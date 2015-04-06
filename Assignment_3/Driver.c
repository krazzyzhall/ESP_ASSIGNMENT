#include<linux/module.h>
#include<linux/kernel.h>
#include<asm/uaccess.h>
#include<linux/init.h>
#include<linux/types.h>
#include<linux/fs.h>
#include<linux/gpio.h>
#include<asm/errno.h>
#include<linux/err.h>
#include<linux/device.h>
#include<linux/slab.h>
#include<linux/ioctl.h>
#include<linux/of_device.h>
#include<linux/spi/spi.h>
#include<linux/uaccess.h>
#include<linux/spi/spidev.h>
#define SPI_MODE_MASK           (SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
                                | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
                                | SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
                                | SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)
//int minors;
#define N_SPI_MINORS                    32      /* ... up to 256 */
static DECLARE_BITMAP(minors, N_SPI_MINORS);
struct spi_master *spi_master;
struct spi_device *spi_device;
struct device *pdev;
char buff[64];
static struct class *spidev_class;
dev_t devno;
int status = 0;
struct spidev_data{
         dev_t                   devt;
         spinlock_t              spi_lock;
         struct spi_device       *spi;
         struct list_head        device_entry;
         /* buffer is NULL unless this device is open (users > 0) */
         struct mutex            buf_lock;
         unsigned                users;
         u8                      *buffer;
         char                    pat[10][8];
};
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");
static void spidev_complete(void *arg) 
{
         complete(arg);
}
static ssize_t spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
         DECLARE_COMPLETION_ONSTACK(done);
         int status;
        message->complete = spidev_complete;
         message->context = &done;
 
         spin_lock_irq(&spidev->spi_lock);
         if (spidev->spi == NULL)
                 status = -ESHUTDOWN;
         else
                 status = spi_async(spidev->spi, message);
         spin_unlock_irq(&spidev->spi_lock);
 
         if (status == 0) {
                 wait_for_completion(&done);
                 status = message->status;
                 if (status == 0)
                         status = message->actual_length;
         }
         return status;
}
static inline ssize_t spidev_sync_write(struct spidev_data *spidev, size_t len)
{
        struct spi_transfer     t = {
                        .tx_buf         = spidev->buffer,
                        .len            = len,
                };
        struct spi_message      m;

        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        return spidev_sync(spidev, &m);
}
static ssize_t spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
printk("this device doesnot read");
return 0;
}
static ssize_t spidev_write(struct file *filp, const char __user *buf,size_t count, loff_t *f_pos){
struct spidev_data      *spidev;
         ssize_t                 status = 0;
         unsigned long           missing;
 
         /* chipselect only toggles at start or end of operation */
         if (count > bufsiz)
                 return -EMSGSIZE;
 
         spidev = filp->private_data;
         mutex_lock(&spidev->buf_lock);
         missing = copy_from_user(spidev->buffer, buf, count);
         if (missing == 0)
                 status = spidev_sync_write(spidev, count);
         else
                 status = -EFAULT;
         mutex_unlock(&spidev->buf_lock);
 
         return status;
}
static long spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
int status=0;
printk("in ioctl function");
return status;
}
//static long spidev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {}
static int spidev_open(struct inode *inode, struct file *filp){
         struct spidev_data      *spidev;
         int                     status = -ENXIO,retval;
 
         mutex_lock(&device_list_lock);
 
         list_for_each_entry(spidev, &device_list, device_entry) {
                 if (spidev->devt == inode->i_rdev) {
                         status = 0;
                         break;
                 }
         }
         if (status == 0) {
                 if (!spidev->buffer) {
                         spidev->buffer = kmalloc(bufsiz, GFP_KERNEL);
                         if (!spidev->buffer) {
                                 dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
                                 status = -ENOMEM;
                         }
                 }
                 if (status == 0) {
                        spidev->users++;
                        spidev->spi->mode |=SPI_MODE_0;//|(spidev->spi->mode & ~SPI_MODE_MASK);
			spidev->spi->mode &= ~SPI_LSB_FIRST;
			spidev->spi->bits_per_word = 8;  
			spidev->spi->max_speed_hz = 10000000;
                        retval = spi_setup(spidev->spi);                    
                        filp->private_data = spidev;
                        nonseekable_open(inode, filp);
                        printk("opened the spi dev");
                 }
         } else
                 pr_debug("spidev: nothing for minor %d\n", iminor(inode));
 
         mutex_unlock(&device_list_lock);
         return status & retval;
}
static int spidev_release(struct inode *inode, struct file *filp){
         struct spidev_data      *spidev;
         int status = 0;
         mutex_lock(&device_list_lock);
         spidev = filp->private_data;
         filp->private_data = NULL;
 
         /* last close? */
         spidev->users--;
         if (!spidev->users) {
                 int   dofree;
                 kfree(spidev->buffer);
                 spidev->buffer = NULL;
                 /* ... after we unbound from the underlying device? */
                 spin_lock_irq(&spidev->spi_lock);
                 dofree = (spidev->spi == NULL);
                 spin_unlock_irq(&spidev->spi_lock);
 
                 if (dofree)
                         kfree(spidev);
         }
         mutex_unlock(&device_list_lock);
 
         return status;
}
static const struct file_operations spidev_fops = {
         .owner =        THIS_MODULE,
         /* REVISIT switch to aio primitives, so that userspace
          * gets more complete API coverage.  It'll simplify things
          * too, except for the locking.
          */
         .write =        spidev_write,
         .read =         spidev_read,
         .unlocked_ioctl = spidev_ioctl,
//       .compat_ioctl = spidev_compat_ioctl,
         .open =         spidev_open,
         .release =      spidev_release,
 };
static int spidev_probe(struct spi_device *spi) {
struct spidev_data *spidev;
int status;
unsigned long minor;
spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
         if (!spidev)
                 return -ENOMEM;
 
         /* Initialize the driver data */
         spidev->spi = spi;
         spin_lock_init(&spidev->spi_lock);
         mutex_init(&spidev->buf_lock);
     INIT_LIST_HEAD(&spidev->device_entry); 
         /* If we can allocate a minor number, hook up this device.
          * Reusing minors is fine so long as udev or mdev is working.
          */
         mutex_lock(&device_list_lock);
         minor = find_first_zero_bit(minors, N_SPI_MINORS);
         if (minor < N_SPI_MINORS) {
                 struct device *dev;
 
                 spidev->devt = MKDEV(devno, minor);
                 dev = device_create(spidev_class, &spi->dev, spidev->devt,
                                     spidev, "myspidev%d.%d",
                                     spi->master->bus_num, spi->chip_select);
                 status = -1;//PTR_ERR_OR_ZERO(dev);
         } else {
                 dev_dbg(&spi->dev, "no minor number available!\n");
                 status = -ENODEV;
         }
if (status == 0) {
                 set_bit(minor, minors);
                 list_add(&spidev->device_entry, &device_list);
         }
         mutex_unlock(&device_list_lock);
 
         if (status == 0)
                 spi_set_drvdata(spi, spidev);
         else
                 kfree(spidev);
 
         return status;
}
static int spidev_remove(struct spi_device *spi) {
struct spidev_data      *spidev = spi_get_drvdata(spi); 
         /* make sure ops on existing fds can abort cleanly */
         spin_lock_irq(&spidev->spi_lock);
         spidev->spi = NULL;
         spin_unlock_irq(&spidev->spi_lock);
 
         /* prevent new opens */
         mutex_lock(&device_list_lock);
         list_del(&spidev->device_entry);
         device_destroy(spidev_class, spidev->devt);
         clear_bit(MINOR(spidev->devt), minors);
         if (spidev->users == 0)
                 kfree(spidev);
         mutex_unlock(&device_list_lock);
 
         return 0;
}/*
static const struct of_device_id spidev_dt_ids[] = {
         { .compatible = "rohm,dh2228fv" },
         {},
 };
MODULE_DEVICE_TABLE(of, spidev_dt_ids);
static struct spi_driver spidev_spi_driver = {
         .driver = {
                 .name =         "spidev",
                 .owner =        THIS_MODULE,
                 .of_match_table = of_match_ptr(spidev_dt_ids),
         },
         .probe =        spidev_probe,
         .remove =       spidev_remove,
 
         /* NOTE:  suspend/resume methods are not necessary here.
          * We don't do anything except pass the requests to/from
          * the underlying controller.  The refrigerator handles
          * most issues; the controller driver handles the rest.
          
 };*/
int __init SPI_init(void){
int status;
//set GPIO pins to appropriate value
//set SPI MUX to appropriate value 
//
gpio_request(42,"CS");
gpio_request(43,"MOSI");
  gpio_export(42,true);
  gpio_export(43,true);
gpio_request(55,"SCLK");
  gpio_export(55,true);
gpio_direction_output(42,1); 
gpio_set_value_cansleep(42,0);
gpio_direction_output(43,1); 
gpio_set_value_cansleep(43,0);
gpio_direction_output(55,1); 
gpio_set_value_cansleep(55,0);
/*if(alloc_chrdev_region (&devno,0,5,"myspidev")<0){
printk("Failed to open SPI Device file");
return -1;
}
spidev_class = class_create(THIS_MODULE, "spidev");
 status = spi_register_driver(&spidev_spi_driver);
         if (status < 0) {
                 class_destroy(spidev_class);
                 unregister_chrdev(devno, spidev_spi_driver.driver.name);
         }
*/         return status;
//spi_master = spi_busnum_to_master(SPI_BUS);
}
void __exit SPI_exit(void){
gpio_direction_output(42,0); 
gpio_set_value_cansleep(42,1);
gpio_direction_output(43,0); 
gpio_set_value_cansleep(43,1);
gpio_direction_output(55,0); 
gpio_set_value_cansleep(55,1);
gpio_free(42);
gpio_free(43); 
gpio_free(54);
gpio_free(55); 
//spi_unregister_driver(&spidev_spi_driver);
//         class_destroy(spidev_class);
      //   unregister_chrdev(devno, spidev_spi_driver.driver.name);
}
module_init(SPI_init);
module_exit(SPI_exit);
MODULE_AUTHOR("Shivanshu Shukla");
MODULE_DESCRIPTION("this module is for Assignment 3 of ESP 598");
MODULE_LICENSE("GPL");
