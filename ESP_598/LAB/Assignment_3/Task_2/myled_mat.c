#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>
#include <linux/uaccess.h>
#include <linux/spi/spidev.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/compat.h>
#include <linux/of.h> 
#include <linux/cdev.h>

#include <linux/kthread.h>  // for threads
#include"led_mat.h"
#include <asm-generic/errno.h>
#include <asm-generic/errno-base.h>

struct kthreadp *work;
static inline int __must_check PTR_ERR_OR_ZERO(__force const void *ptr)
{
        if (IS_ERR(ptr))
                return PTR_ERR(ptr);
        else
                return 0;
}

static DECLARE_BITMAP(minors, N_SPI_MINORS);

/*work queue variables*/
struct spidev_data;
/* function for checking err */
static DECLARE_BITMAP(minors, N_SPI_MINORS);

char buff[64];
static struct class *spidev_class;
static int kill;
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
         struct my_seq_pat       seq;
         int                     ispattern;
         int                     seq_enabled;
	struct task_struct *thread1;
};

static struct task_struct *thread1;

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
static int sequence_run(void *work){
		struct spidev_data *spidev=((struct kthreadp*)work)->spidev;                 
		 int i=0,j;
		char temp[2];
                                           
		 while(!kthread_should_stop()){
                for(i=0;i<spidev->seq.lseq;i+=2){
                          int r=0;
                          if(spidev->seq.seq[i]==0 && spidev->seq.seq[i+1]==0)
				break;
			  mutex_lock(&spidev->buf_lock);
		          r=spidev->seq.seq[i];
                          for(j=0;j<8;j++){   //this for loop 
			  temp[1]=spidev->pat[r][j];
                          temp[0]=j+1;
                             memcpy(spidev->buffer,temp,2);
                             status = spidev_sync_write(spidev,2);
                             }
                            mutex_unlock(&spidev->buf_lock);
                             msleep(spidev->seq.seq[i+1]);
                           }
			set_current_state(TASK_INTERRUPTIBLE);  ///to make the thread interruptible
                         }
return status;
}
static ssize_t spidev_write(struct file *filp, const char __user *buf,size_t count, loff_t *f_pos){
struct spidev_data      *spidev;
         ssize_t                 status = 0;
         /* chipselect only toggles at start or end of operation */
         if (count > bufsiz)
                 return -EMSGSIZE;
        spidev = filp->private_data;
                 if(spidev->ispattern==1){
        		  mutex_lock(&spidev->buf_lock); 
                          status=copy_from_user(spidev->pat,(char **)buf,count);  
                          spidev->ispattern=0;
			  mutex_unlock(&spidev->buf_lock);                            
                 }
                 else if(spidev->seq_enabled==0){
        		  mutex_lock(&spidev->buf_lock); 
                              status=copy_from_user(spidev->buffer,buf,count);
 	        status = spidev_sync_write(spidev, count); 
                 mutex_unlock(&spidev->buf_lock);                            
                 }
printk("led_mat : in write\n");
         return status;
}
static long spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int			err = 0;
	int			retval = 0;
	struct spidev_data	*spidev;
	struct spi_device	*spi;
	u32			tmp;
int status=0;
spidev = filp->private_data;
switch(cmd){
case ISPATTERN:
spidev->ispattern=arg;
       printk("ispattern : %lu \n ",arg);
//set is_patternset
break;
case INS_SEQ:
  //     printk("in case 2 : %lu \n ",arg);
   work->spidev=spidev;
   work->count=0;
   work->status=status;
if(thread1==0){
thread1 = kthread_create(sequence_run,(void *)work,"write_thread");
    if((thread1 && spidev->seq_enabled==1))
        {
	int *tmp=kmalloc(((struct my_seq_pat*)arg)->lseq*4,GFP_KERNEL);
	status= memcpy(&(spidev->seq),(struct my_seq_pat *)arg,sizeof(struct my_seq_pat));	        
        copy_from_user(tmp,((struct my_seq_pat *)arg)->seq,((struct my_seq_pat *)arg)->lseq*4);
        spidev->seq.seq=tmp;
        spidev->seq.lseq=((struct my_seq_pat*)arg)->lseq;	
        wake_up_process(thread1);
        }
}
else{
	int *tmp=kmalloc(((struct my_seq_pat*)arg)->lseq*4,GFP_KERNEL);
	status= memcpy(&(spidev->seq),(struct my_seq_pat *)arg,sizeof(struct my_seq_pat));	        
        copy_from_user(tmp,((struct my_seq_pat *)arg)->seq,((struct my_seq_pat *)arg)->lseq*4);
        spidev->seq.seq=tmp;
        spidev->seq.lseq=((struct my_seq_pat*)arg)->lseq;	
  }
 //set sequence
break;

case SEQ_ENABLE:
spidev->seq_enabled=arg;
       printk("seq_enabled : %lu \n ",arg);
//set sequence
break;

case KILL_THREAD: //this case is for destroying a write thread
kill=1;
 status=kthread_stop(thread1);
 if(!status)
  printk(KERN_INFO "Thread stopped\n");
thread1=NULL;
break;
default:
printk("this command  is not supported \n");
status=-1;
break;

}
/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
switch(cmd){
	case SPI_IOC_WR_MODE:
		if (cmd == SPI_IOC_WR_MODE)
			retval = __get_user(tmp, (u8 __user *)arg);
		else
			retval = __get_user(tmp, (u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			tmp |= spi->mode & ~SPI_MODE_MASK;
			spi->mode = (u16)tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "spi mode %x\n", tmp);
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u32	save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "%csb first\n",
						tmp ? 'l' : 'm');
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->bits_per_word;

			spi->bits_per_word = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:
		retval = __get_user(tmp, (__u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->max_speed_hz;

			spi->max_speed_hz = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->max_speed_hz = save;
			else
				dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
		}
		break;
default:
printk("this command  is not supported \n");
status=-1;
break;
}
spi_dev_put(spi);
return status;
}
static int spidev_open(struct inode *inode, struct file *filp){
         struct spidev_data      *spidev;
         int                     status = -ENXIO;
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
			work=(struct kthreadp *)kmalloc(sizeof(struct kthreadp),GFP_KERNEL);
                        spidev->users++;
                        spidev->ispattern = 0;
                        spidev->seq_enabled = 0;
			thread1=0;
                        filp->private_data = spidev;
                        nonseekable_open(inode, filp);
                        printk("opened the spi dev");
                 }
         } else
                 pr_debug("spidev: nothing for minor %d\n", iminor(inode));
 
         mutex_unlock(&device_list_lock);
         return status;
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
                 spin_lock_irq(&spidev->spi_lock);
                 dofree = (spidev->spi == NULL);
		spin_unlock_irq(&spidev->spi_lock);
		if(thread1!=NULL){
			status=kthread_stop(thread1);
		 if(!status)
  			printk(KERN_INFO "Thread stopped\n");}               
		 printk("led_mat : in close\n"); 
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
 
         spidev->spi = spi;
         spin_lock_init(&spidev->spi_lock);
         mutex_init(&spidev->buf_lock);
     INIT_LIST_HEAD(&spidev->device_entry);
        mutex_lock(&device_list_lock);
         minor = find_first_zero_bit(minors, N_SPI_MINORS);
         if (minor < N_SPI_MINORS) {
                 struct device *dev;
 
                 spidev->devt = MKDEV(SPI_MAJOR, minor);
                 dev = device_create(spidev_class, &spi->dev, spidev->devt,
                                     spidev, "myspidev%d.%d",
                                     spi->master->bus_num, spi->chip_select);
                 status =PTR_ERR_OR_ZERO(dev);
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
// printk("spidev: reached the final spot");
         return status;
}

static int spidev_remove(struct spi_device *spi) {
struct spidev_data      *spidev = spi_get_drvdata(spi); 
         spin_lock_irq(&spidev->spi_lock);
         spidev->spi = NULL;
         spin_unlock_irq(&spidev->spi_lock);
 
         mutex_lock(&device_list_lock);
         list_del(&spidev->device_entry);
         device_destroy(spidev_class, spidev->devt);
         clear_bit(MINOR(spidev->devt), minors);
         if (spidev->users == 0)
                 kfree(spidev);
         mutex_unlock(&device_list_lock);
 
         return 0;
}
static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "rohm,dh2228fv" },
	{},
};

MODULE_DEVICE_TABLE(of, spidev_dt_ids);

static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spidev",
		.owner =	THIS_MODULE,
		.of_match_table = of_match_ptr(spidev_dt_ids),
	},
	.probe =	spidev_probe,   //will be called when spi device is registered
	.remove =	spidev_remove,  // will be called when spi is unregistered
};

static int __init SPI_init(void){
int status=0;
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	gpio_request(42,"CS");  //request gpio for muxing
	gpio_request(43,"MOSI"); //request gpio for muxing
	gpio_export(42,true);  //export gpio for muxing
	gpio_export(43,true);  //export gpio for muxing
	gpio_request(55,"SCLK"); //request gpio for muxing
	gpio_export(55,true);  //export gpio for muxing
	gpio_direction_output(42,1); 
	gpio_set_value_cansleep(42,0);
	gpio_direction_output(43,1); 
	gpio_set_value_cansleep(43,0);
	gpio_direction_output(55,1); 
	gpio_set_value_cansleep(55,0); 
	if( register_chrdev(SPI_MAJOR,"myspi", &spidev_fops)<0){
		printk("Failed to open SPI Device file");
		return -1;
	}
	printk("Major number : %d\n",SPI_MAJOR);
	spidev_class = class_create(THIS_MODULE, "myspidev");
	status = spi_register_driver(&spidev_spi_driver);
         if (status < 0) {
                 class_destroy(spidev_class);
                 unregister_chrdev(SPI_MAJOR, spidev_spi_driver.driver.name);
                 printk("unable to register SPI driver");
         }
//	printk("status = %d",status);
         return status;
}

static void __exit SPI_exit(void){
gpio_free(42);
gpio_free(43); 
gpio_free(55); 
        spi_unregister_driver(&spidev_spi_driver);
        class_destroy(spidev_class);
        unregister_chrdev(SPI_MAJOR, spidev_spi_driver.driver.name);
	printk("SPI_EXIT DONE \n");
}
module_init(SPI_init);
module_exit(SPI_exit);
MODULE_AUTHOR("Shivanshu Shukla");
MODULE_DESCRIPTION("this module is for Assignment 3 of ESP 598");
MODULE_LICENSE("GPL");
