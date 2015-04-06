/* ----------------------------------------------- DRIVER Squeue --------------------------------------------------
 
	Assignment -1 ----- Message Passing Driver for Interprocess Communication.
 
 ----------------------------------------------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include "message.h"
#include<linux/init.h>
#define DEVICE_NAME                 "Squeue"  // device name to be created and registered
#define NUM_OF_MINOR_DEV              5
typedef struct ipc_msg ipc_msg;
/*
   structure for message passing between the processes
*/

/* per device structure */
static DEFINE_MUTEX(my_mutex);
struct Squeue {
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/
	ipc_msg *msg[10];			/* buffer for struct of massages to be passed*/
	int current_write_pointer;      //head pointer
        int current_read_pointer;       //tail pointer
    //    unsigned long cumulative_enqueue_time;  //to know accumulated enqueue-dequeue time
} *Squeue_devp,*Squeue_devp1,*Squeue_devp2,*Squeue_devp3;

static dev_t Squeue_dev_number;      /* Allotted device number */
struct class *Squeue_class;          /* Tie with the device model */
static struct device *Squeue_device; 
static unsigned long init_jiffies;   //to know the initial time when the drive was loaded.
/*
* Open Squeue driver
*/
int Squeue_open(struct inode *inode, struct file *file)
{
	struct Squeue *Squeue_dvp;
         int minor=iminor(inode);
	// Get the per-device structure that contains this cdev 
	Squeue_dvp = container_of(inode->i_cdev, struct Squeue, cdev);

	// Easy access to cmos_devp from rest of the entry points 
	Squeue_dvp->current_write_pointer = 0;
	Squeue_dvp->current_read_pointer = -1;
	file->private_data = Squeue_dvp;
        if(minor==0){
	      printk("\n Squeue is openning in device with read: %d write :%d\n",Squeue_dvp->current_read_pointer,Squeue_dvp->current_write_pointer);
	}
        else{
            printk("Squeue in opening Out device with minor as :%d :  read: %d write :%d\n",minor,Squeue_dvp->current_read_pointer,Squeue_dvp->current_write_pointer);
}
        return 0;
}
//-------------------------------------done-------------------------------------------------------
/*
 * Release Squeue driver
 */
int Squeue_release(struct inode *inode, struct file *file)
{
	//struct gmem_dev *gmem_devp = file->private_data;
	
	printk("\n releaseing Squeue driver\n");
	
	return 0;
}

/*
 * Write to Squeue driver
 */
ssize_t Squeue_driver_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{
  //       printk("\nwriting on Squeue Driver\n"); 
ipc_msg *temp;
temp=(ipc_msg*)kmalloc(sizeof(ipc_msg), GFP_KERNEL);
        int check=0,retval;
	struct Squeue *Squeue_devp = file->private_data;
        int cwrite=(Squeue_devp->current_write_pointer) - 10*(Squeue_devp->current_write_pointer/10);
        int cread=(Squeue_devp->current_read_pointer) - 10*(Squeue_devp->current_read_pointer/10);
        if((Squeue_devp->current_write_pointer-Squeue_devp->current_read_pointer>0) &&
           (Squeue_devp->current_write_pointer-Squeue_devp->current_read_pointer<=10) ){
            if(cwrite-cread!=0){
                   mutex_lock(&my_mutex);
                   Squeue_devp->msg[cwrite]=temp;
                   check=copy_from_user(Squeue_devp->msg[cwrite],(ipc_msg*)buf,count);
                   Squeue_devp->msg[cwrite]->enqueue_time=jiffies;
                   Squeue_devp->current_write_pointer++;
//                   printk("in cwrite-cread with check :  %d, major: %d,minor:%d, i_rdev:%d count : %d\n",check,imajor(file->f_inode),iminor(file->f_inode),file->f_inode->i_rdev,count);
	           printk("Writing -- done with write pointer at %d \n",Squeue_devp->current_write_pointer);
                   mutex_unlock(&my_mutex);
                   retval=count;
            }
           else
             retval=-1;
        }
        else retval=-2;	
  //     printk("recieved:\n %s \n",Squeue_devp->msg[cwrite].message); 
//         kfree(temp);
	return sizeof(ipc_msg);
}
/*
 * Read to Squeue driver
 */
ssize_t Squeue_driver_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
         ssize_t retval=0;
         printk("reading from Squeue Driver");
	int cread,cwrite,res;
	struct Squeue *Squeue_devp = file->private_data;               
 //       printk("read %d and write %d minor : %d \n",Squeue_devp->current_read_pointer,Squeue_devp->current_write_pointer,iminor(file->f_inode));
        if( Squeue_devp->current_read_pointer+1 < Squeue_devp->current_write_pointer ){    
        mutex_lock(&my_mutex);
         Squeue_devp->current_read_pointer++;
   //      printk(", major: %d,minor:%d , file->f_inode->i_rdev :%d\n :incrementing pointer with count : %d",imajor(file->f_inode),iminor(file->f_inode),file->f_inode->i_rdev,count);
        //printk("\ncread: %d       :     cwrite :   %d\n",cread,cwrite);
        cread = (Squeue_devp->current_read_pointer) - 10*(Squeue_devp->current_read_pointer/10);
        cwrite = (Squeue_devp->current_write_pointer) - 10*(Squeue_devp->current_write_pointer/10);
     //   printk("\ncread: %d       :     cwrite :   %d\n",cread,cwrite);
        /* 
         * Actually put the data into the buffer 
         */ 
if(Squeue_devp->msg[cread]==NULL){
            mutex_unlock(&my_mutex);
             return -100;}
        Squeue_devp->msg[cread]->accumulated_queing_time+=jiffies-Squeue_devp->msg[cread]->enqueue_time;
        res=copy_to_user(buf,(char *)Squeue_devp->msg[cread],count);
        printk("Reading -- at %d %s ret : res %d\n",cread,Squeue_devp->msg[cread]->message,res);
        kfree(Squeue_devp->msg[cread]);
            if(res<0){
              retval=-3;
              }
              else{
              retval=count;
              retval=0;
              }
        /* 
         * Most read functions return the number of bytes put into the buffer
         */
         mutex_unlock(&my_mutex);
         }
         else {
         printk("unsucessfull read from Squeue Driver because of :\n %d and %d count: %d\n",Squeue_devp->current_read_pointer,Squeue_devp->current_write_pointer,count);
           retval=-2;
           
         } 
        printk("retval:%d\n",retval);
	return retval;

}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations Squeue_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= Squeue_open,        /* Open method */
    .release	        = Squeue_release,     /* Release method */
    .write		= Squeue_driver_write,       /* Write method */
    .read		= Squeue_driver_read,        /* Read method */
};

/*
 * Driver Initialization
 */
//=======================================un_done======================================================
int __init Squeue_driver_init(void)
{
	int ret=0;

        mutex_init(&my_mutex);
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&Squeue_dev_number, 0, NUM_OF_MINOR_DEV, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	Squeue_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
        Squeue_devp = kmalloc(sizeof(struct Squeue), GFP_KERNEL);
        Squeue_devp1 = kmalloc(sizeof(struct Squeue), GFP_KERNEL);
        Squeue_devp2 = kmalloc(sizeof(struct Squeue), GFP_KERNEL);
        Squeue_devp3 = kmalloc(sizeof(struct Squeue), GFP_KERNEL);
	if (!Squeue_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}	
	/* Enter name of Device to Cdev*/
	sprintf(Squeue_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&Squeue_devp->cdev, &Squeue_fops);
	cdev_init(&Squeue_devp1->cdev, &Squeue_fops);
	cdev_init(&Squeue_devp2->cdev, &Squeue_fops);
	cdev_init(&Squeue_devp3->cdev, &Squeue_fops);
	Squeue_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&Squeue_devp->cdev, MKDEV(MAJOR(Squeue_dev_number),0), NUM_OF_MINOR_DEV);
	ret = cdev_add(&Squeue_devp1->cdev, MKDEV(MAJOR(Squeue_dev_number),1), NUM_OF_MINOR_DEV);
	ret = cdev_add(&Squeue_devp2->cdev, MKDEV(MAJOR(Squeue_dev_number),2), NUM_OF_MINOR_DEV);
	ret = cdev_add(&Squeue_devp3->cdev, MKDEV(MAJOR(Squeue_dev_number),3), NUM_OF_MINOR_DEV);
	//ret = cdev_add(&Squeue_devp->cdev, MKDEV(MAJOR(Squeue_dev_number),), NUM_OF_MINOR_DEV);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}
//         Squeue_setup_cdev(Squeue_devp,0);
	/* Send uevents to udev, so it'll create /dev nodes */
	Squeue_device = device_create(Squeue_class, NULL, MKDEV(MAJOR(Squeue_dev_number), 0), NULL,"bus_in_q");		
	Squeue_device = device_create(Squeue_class, NULL, MKDEV(MAJOR(Squeue_dev_number), 1), NULL,"bus_out_q1");		
	Squeue_device = device_create(Squeue_class, NULL, MKDEV(MAJOR(Squeue_dev_number), 2), NULL,"bus_out_q2");		
	Squeue_device = device_create(Squeue_class, NULL, MKDEV(MAJOR(Squeue_dev_number), 3), NULL,"bus_out_q3");		
	
	init_jiffies=(jiffies-INITIAL_JIFFIES);  //since on some systems jiffies is a very huge uninitialized value at boot and saved.
	printk("Hi computer, this machine has been on for %lu seconds\n",init_jiffies);
	printk("Squeue driver initialized.\n");
	return 0;
}
/* Driver Exit */
void __exit Squeue_driver_exit(void)
{
	// device_remove_file(gmem_dev_device, &dev_attr_xxx);
	/* Release the major number */
	unregister_chrdev_region((Squeue_dev_number), 1);

	/* Destroy devices*/
        
	device_destroy (Squeue_class, MKDEV(MAJOR(Squeue_dev_number), 0));
	device_destroy (Squeue_class, MKDEV(MAJOR(Squeue_dev_number), 1));
	device_destroy (Squeue_class, MKDEV(MAJOR(Squeue_dev_number), 2));
	device_destroy (Squeue_class, MKDEV(MAJOR(Squeue_dev_number), 3));
        //class_distroy(Squeue_class);
	cdev_del(&Squeue_devp->cdev);
	kfree(Squeue_devp);
	
	/* Destroy driver_class */
	class_destroy(Squeue_class);

	printk("Squeue driver removed.\n");
}

module_init(Squeue_driver_init);
module_exit(Squeue_driver_exit);
MODULE_LICENSE("GPL v2");
