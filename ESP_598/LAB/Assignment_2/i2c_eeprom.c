#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/device.h>
#include<linux/types.h>
#include<linux/i2c.h>
#include<asm/uaccess.h>
#include <linux/fs.h>
#include <linux/init.h>
#include<linux/slab.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/list.h>

#define FLASHGETS 1
#define FLASHGETP 7
#define FLASHPUTP 3
#define FLASHPUTS 4
#define FLASHERASE 5
#define MAX_ELEN 512
#define MEM_ERR  514
#define I2C_MAJOR 512
#define I2C_MINORS 256
#define NUM_OF_MINOR_DEV 1
dev_t i2cdev_number = 0;
extern int errno;
int errno;
static DEFINE_SPINLOCK (i2c_dev_list_lock);
struct i2c_dev
{
  struct list_head list;
  struct i2c_adapter *adap;
  struct device *dev;
};
static LIST_HEAD (i2c_dev_list);
static struct i2c_clnt
{
  struct payload
  {
    u16 addr;
    char msg[64];
  } *mesg;
  struct i2c_client *client;
};
static struct class *i2c_dev_class;

static struct i2c_dev *
i2c_dev_get_by_minor (unsigned index)
{
  struct i2c_dev *i2c_dev;

  spin_lock (&i2c_dev_list_lock);
  list_for_each_entry (i2c_dev, &i2c_dev_list, list)
  {
    if (i2c_dev->adap->nr == index)
      goto found;
  }
  i2c_dev = NULL;
found:
  spin_unlock (&i2c_dev_list_lock);
  return i2c_dev;
}

static ssize_t
i2ceeprom_read (struct file *file, char __user * buf, size_t count,
		loff_t * offset)
{
  char *tmp;
  int ret = -1;
  u16 add;
  struct i2c_clnt *clnt = file->private_data;
  if (count > MAX_ELEN)
    {
      count = MAX_ELEN;
    }
  gpio_set_value_cansleep (26, 0);
  tmp = (char *) kmalloc (64, GFP_KERNEL);
  if (tmp == NULL)
    {
      return -ENOMEM;
    }
  add = clnt->mesg->addr;
  ret = i2c_master_send (clnt->client, (char *) &add, 2);
  ret = i2c_master_recv (clnt->client, tmp, 64);	// receiving from client adapter into tmp of size count bytes
  if (ret >= 0)
    {
      ret = copy_to_user (buf, tmp, 64) ? -EFAULT : ret;
    }
  else
    {
      errno = EBUSY;
    }
  kfree (tmp);
  gpio_set_value_cansleep (26, 0);
  return ret;
}

static ssize_t
i2ceeprom_write (struct file *file, const char __user * buf, size_t count,
		 loff_t * offset)
{
  int ret;
  struct i2c_clnt *clnt = file->private_data;
  struct i2c_client *client = clnt->client;
  if (count > MAX_ELEN)
    {
      count = MAX_ELEN;
    }
  gpio_set_value_cansleep (26, 0);
  int t = copy_from_user (clnt->mesg->msg, buf, count);
  if (t)
    {
      return -t;
    }
  if (clnt->mesg->addr < 0 || clnt->mesg->addr > (64 * 512 - 1))
    {
      return -MEM_ERR;
    }
  ret =
    i2c_master_send (client, (char *) clnt->mesg, sizeof (struct payload));
  gpio_set_value_cansleep (26, 0);
  if (ret < 0)
    {
      errno = EBUSY;
    }
  return ret;
}

u16
get_pg_no (struct i2c_clnt * clnt)
{
  u16 addr = clnt->mesg->addr;
  printk (" %d \n", addr);
  return addr;
}

int
set_pg_no (u16 pg_no, struct i2c_clnt *client)
{
  client->mesg->addr = pg_no;
  return pg_no;
}

int
status_flash (struct i2c_client *client)
{
  return i2c_master_send (client, "a", 1);
}

int
erase_flash (struct i2c_client *client, u16 pages)
{
  char mesg[66];
  struct mywrite
  {
    u16 add;
    char mesg[64];
  } mywrite;
  char temp[1];
  temp[0] = 0x00;
  u16 i;
  int ret = -1;
  mesg[0] = 0x61;
  mesg[1] = 0x61;
  for (i = 0; i < 64; i++)
    {
      mywrite.mesg[i] = 97;
    }
  for (i = 0; i < pages; i++)
    {
      mywrite.add = (i * 64);
      while (ret < 0)
	ret = i2c_master_send (client, (char *) &mywrite, 66);
      ret = -1;
    }
  printk ("\n");
  return 0;
}

static long
i2ceeprom_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
  struct i2c_clnt *clnt = file->private_data;
  int res = 0;
  gpio_set_value_cansleep (26, 1);
  switch (cmd)
    {

    case FLASHGETS:
      return i2c_master_send (clnt->client, "a", 1);
    case FLASHGETP:
      res = get_pg_no (clnt);
      if ((res) < 0)
	{
	  return -1;
	}
      return res;
    case FLASHPUTP:
      res = set_pg_no (((u16) arg << 6), clnt);
      if (res < 0)
	{
	  return -1;
	}
      file->private_data = clnt;
      return res;
    case FLASHERASE:
      erase_flash (clnt->client, 512);
      break;
    default:
      return -ENOTTY;
    }
  gpio_set_value_cansleep (26, 0);
  return 0;
}

static int
i2ceeprom_open (struct inode *inode, struct file *file)
{
  unsigned int minor = iminor (inode);
  struct i2c_clnt *pack;
  struct i2c_adapter *adap;
  struct i2c_dev *i2c_dev;
/*-----------gpio --------------------*/
  gpio_request (29, "mux-i2c");
  gpio_request (26, "busy indicator");
  gpio_export (29, true);
  gpio_export (26, true);
  gpio_direction_output (26, 0);
  gpio_direction_output (29, 0);
  printk ("i2c in open \n");
  gpio_set_value_cansleep (29, 0);
  gpio_set_value_cansleep (26, 0);
/*-----------gpio --------------------*/
  i2c_dev = i2c_dev_get_by_minor (minor);
  if (!i2c_dev)
    return -ENODEV;
  adap = i2c_get_adapter (i2c_dev->adap->nr);
  if (!adap)
    return -ENODEV;
  pack = (struct i2c_clnt *) kzalloc (sizeof (*pack), GFP_KERNEL);
  if (!pack)
    {
      i2c_put_adapter (adap);
      return -ENOMEM;
    }
  pack->mesg = kzalloc (sizeof (struct payload), GFP_KERNEL);
  pack->client =
    (struct i2c_client *) kzalloc (sizeof (struct i2c_client), GFP_KERNEL);
  if (!pack->client)
    {
      i2c_put_adapter (adap);
      return -ENOMEM;
    }
  snprintf (pack->client->name, I2C_NAME_SIZE, "i2c-dev %d", adap->nr);
  pack->client->addr = 0x54;
  pack->mesg->addr = 95;
  pack->client->adapter = adap;
  file->private_data = pack;
  printk ("i2c device opened  \n");
  return 0;
}

static int
i2ceeprom_release (struct inode *inode, struct file *file)
{
  struct i2c_clnt *client = file->private_data;
  gpio_direction_output (29, 0);
  gpio_set_value_cansleep (29, 1);
  gpio_free (29);
  gpio_free (26);
  i2c_put_adapter (client->client->adapter);
  kfree (client);
  file->private_data = NULL;
  return 0;
}

static const struct file_operations i2cdev_fops = {
  .owner = THIS_MODULE,
  .read = i2ceeprom_read,
  .write = i2ceeprom_write,
  .unlocked_ioctl = i2ceeprom_ioctl,
  .open = i2ceeprom_open,
  .release = i2ceeprom_release,
};

/*------module init and exit----------------------------*/
static struct i2c_dev *
get_free_i2c_dev (struct i2c_adapter *adap)
{
  struct i2c_dev *i2c_dev;

  if (adap->nr >= I2C_MINORS)
    {
      printk (KERN_ERR "i2c-dev: Out of device minors (%d)\n", adap->nr);
      return ERR_PTR (-ENODEV);
    }

  i2c_dev = kzalloc (sizeof (*i2c_dev), GFP_KERNEL);
  if (!i2c_dev)
    return ERR_PTR (-ENOMEM);
  i2c_dev->adap = adap;

  spin_lock (&i2c_dev_list_lock);
  list_add_tail (&i2c_dev->list, &i2c_dev_list);
  spin_unlock (&i2c_dev_list_lock);
  return i2c_dev;
}

static void
return_i2c_dev (struct i2c_dev *i2c_dev)
{
  spin_lock (&i2c_dev_list_lock);
  list_del (&i2c_dev->list);
  spin_unlock (&i2c_dev_list_lock);
  kfree (i2c_dev);
}

static int
i2cdev_attach_adapter (struct device *dev, void *dummy)
{
  struct i2c_adapter *adap;
  struct i2c_dev *i2c_dev;
  int res;

  if (dev->type != &i2c_adapter_type)
    return 0;
  adap = to_i2c_adapter (dev);
  i2c_dev = get_free_i2c_dev (adap);
  if (IS_ERR (i2c_dev))
    return PTR_ERR (i2c_dev);
  i2c_dev->dev =
    device_create (i2c_dev_class, NULL, MKDEV (I2C_MAJOR, adap->nr), NULL,
		   "i2c-flash");
  if (IS_ERR (i2c_dev->dev))
    {
      printk ("error creating device \n");
      res = PTR_ERR (i2c_dev->dev);
      return res;
    }
  printk ("i2c-dev: adapter [%s] registered as minor %d\n", adap->name,
	  adap->nr);
  return 0;
}

static int
i2cdev_detach_adapter (struct device *dev, void *dummy)
{
  struct i2c_adapter *adap;
  struct i2c_dev *i2c_dev = NULL;
  if (dev->type != &i2c_adapter_type)
    return 0;
  adap = to_i2c_adapter (dev);
  i2c_dev = i2c_dev_get_by_minor (adap->nr);
  if (!i2c_dev)			/* attach_adapter must have failed */
    return 0;
  return_i2c_dev (i2c_dev);
  device_destroy (i2c_dev_class, MKDEV (I2C_MAJOR, adap->nr));
  return 0;
}

static int
i2cdev_notifier_call (struct notifier_block *nb, unsigned long action,
		      void *data)
{
  struct device *dev = data;
  switch (action)
    {
    case BUS_NOTIFY_ADD_DEVICE:
      return i2cdev_attach_adapter (dev, NULL);
    case BUS_NOTIFY_DEL_DEVICE:
      return i2cdev_detach_adapter (dev, NULL);
    }
  return 0;
}

static struct notifier_block i2cdev_notifier = {
  .notifier_call = i2cdev_notifier_call,
};

static int __init
i2c_eeprom_init (void)
{
  int res;
  printk (KERN_INFO "i2c /dev entries driver \n");
  res = register_chrdev (I2C_MAJOR, "i2c-flash", &i2cdev_fops);
  printk (KERN_INFO "i2c:reg_chardev %d\n", res);
  if (res < 0)
    {
      printk (KERN_INFO "i2c could not register the flash \n");
      return -1;
    }
  i2cdev_number = res;
  i2c_dev_class = class_create (THIS_MODULE, "i2ceeprom");
  if (IS_ERR (i2c_dev_class))
    {
      res = PTR_ERR (i2c_dev_class);
      unregister_chrdev (I2C_MAJOR, "i2c-flash");
      printk ("unregistering in init \n");
    }
  res = bus_register_notifier (&i2c_bus_type, &i2cdev_notifier);
  if (res)
    {
      class_destroy (i2c_dev_class);
      printk (KERN_INFO "i2c distroying class \n");
    }
  i2c_for_each_dev (NULL, i2cdev_attach_adapter);
  return 0;
}

static void __exit
i2c_eeprom_exit (void)
{
  gpio_free (29);
  gpio_free (26);
  bus_unregister_notifier (&i2c_bus_type, &i2cdev_notifier);
  i2c_for_each_dev (NULL, i2cdev_detach_adapter);
  class_destroy (i2c_dev_class);
  unregister_chrdev (I2C_MAJOR, "i2c-flash");
  printk (KERN_INFO "i2c /dev exit driver \n");
}

MODULE_AUTHOR ("Shivanshu Shukla");
MODULE_DESCRIPTION ("modult to interface a 512x64 bytes eeprom");
MODULE_LICENSE ("GPL");
module_init (i2c_eeprom_init);
module_exit (i2c_eeprom_exit);
