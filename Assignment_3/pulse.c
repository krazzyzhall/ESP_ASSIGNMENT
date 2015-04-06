#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
//#define MY_CPU_FREQ 400000
typedef struct {
int trig;
unsigned long echo;
unsigned long long rising;
unsigned long long falling;
long diff;
int read_val;
int ready;
int read_falling;
int read_rising;
int speed;
int direction;
int irq_15;
void *dev_id;
int samples[10];
struct cdev my_cdev;
}my_pulse_struct;
static int devno;
static struct class *pulse_class;
static my_pulse_struct *pulse_struct;
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

static irqreturn_t pulse_handler(int irq, void *dev_id, struct pt_regs *regs){
//if(gpio_get_value(15)==0){
//return IRQ_NONE;
//}
if(pulse_struct->read_rising==1 && pulse_struct->read_val==0){
if(gpio_get_value(15)==0){
return IRQ_NONE;
}
//printk("servicing forr irq number : %d in rising edge about to be changed to falling  %d  \n",irq,irqd_get_trigger_type(irq_get_irq_data(irq)));
pulse_struct->rising=rdtsc();
//pulse_struct->diff+=rdtsc();
printk("\n / %d : %lu \n",gpio_get_value(15),pulse_struct->rising);
         pulse_struct->read_falling=1;
	pulse_struct->read_rising=0;
        irq_set_irq_type(pulse_struct->irq_15,IRQF_TRIGGER_FALLING);
return IRQ_HANDLED;
}
else if(pulse_struct->read_falling==1){
//printk("servicing forr irq number : %d in falling Edge about to be changed to rising  %d  \n",irq,irqd_get_trigger_type(irq_get_irq_data(irq)));
printk("\n \\ %d : %lu \n",gpio_get_value(15),pulse_struct->falling);
pulse_struct->falling=rdtsc();
pulse_struct->diff=pulse_struct->falling - pulse_struct->rising;
//pulse_struct->diff-=rdtsc();
pulse_struct->read_falling=0;
pulse_struct->read_val=1;
return IRQ_HANDLED;
//printk("\n \\ %d : %lu \n",gpio_get_value(15),pulse_struct->falling);
// irq_set_irq_type(pulse_struct->irq_15,IRQF_TRIGGER_RISING);
}
return IRQ_NONE;
}

int pulse_open(struct inode *inode, struct file *file)
{
int status =0,retval;
unsigned long flags;
	pulse_struct->trig=0;
	pulse_struct->rising=-1;
	pulse_struct->falling=-1;
        pulse_struct->read_val=0;
        pulse_struct->read_rising=0;
        pulse_struct->read_falling=0;
        pulse_struct->irq_15=gpio_to_irq(15);
 	printk("retval : in if before open \n");
       // udelay(11);
       //  pulse_struct->irq_15=gpio_to_irq(15);
        //enable_irq(pulse_struct->irq_15);
        udelay(11);
	flags = IRQF_TRIGGER_RISING;
        if((retval=request_irq(pulse_struct->irq_15,pulse_handler,flags,"pulse",pulse_struct->dev_id))==0){	
 	printk("retval : in if \n");
       //  irq_set_irq_type(pulse_struct->irq_15,IRQF_TRIGGER_RISING);
 	printk("retval : %d \n",retval);
          }
	printk("pulse driver: in open\n");
	file->private_data=pulse_struct;
return status;
}

int pulse_release(struct inode *inode, struct file *file)
{

int status =0;
	pulse_struct->trig=0;
        pulse_struct->echo=0;
free_irq(pulse_struct->irq_15,pulse_struct->dev_id);
	printk("pulse driver: in release\n");
return status;
}

ssize_t pulse_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{
int status =1;
        pulse_struct->read_rising=1;
	//disable_irq(((my_pulse_struct*)file->private_data)->irq_15);
        gpio_set_value_cansleep(14,1);
        udelay(11);
        gpio_set_value_cansleep(14,0);
       // udelay(20);
	//enable_irq(((my_pulse_struct*)file->private_data)->irq_15);
 irq_set_irq_type(pulse_struct->irq_15,IRQF_TRIGGER_RISING);
	printk("pulse driver: in write\n");
return status;
}
ssize_t pulse_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
int status =0;
long dist=0;
if(pulse_struct->read_val==1){
pulse_struct->read_val=0;
my_pulse_struct *p_st=file->private_data;
	printk("pulse driver: in read\n");
printk("  %lu  :  %lu \n",pulse_struct->falling,pulse_struct->rising);
dist=pulse_struct->diff;//((pulse_struct->falling - pulse_struct->rising));///(MY_CPU_FREQ*2);//(p_st->falling - p_st->rising);
printk("  dist  %lu  \n",dist);
//copy_to_user(buf,&dist,sizeof(unsigned long long));
if(status<0 || (p_st->falling<0||(p_st->rising)<0)){
	printk("pulse driver: in read failure \n");

return -1;
}
else{
p_st->falling=-1;
p_st->rising=-1;
printk(" before copy to y  %lu \n",dist);
status=copy_to_user(buf,&dist,sizeof(long));
	printk("pulse driver: in read success\n");
return status;
}}
else 
return -1;
}
static struct file_operations my_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= pulse_open,        /* Open method */
    .release	        = pulse_release,     /* Release method */
    .write		= pulse_write,       /* Write method */
    .read		= pulse_read,        /* Read method */
};
static int __init pulse_init(void) {
int err;
 devno = MKDEV(300, 0);
printk("setting gpio 14 and 15 to appropriate value");
gpio_request(30,"MUX_TRIG");  //request gpio for muxing trigger
gpio_request(31,"MUX_ECHO"); //request gpio for muxing  echo
gpio_request(14,"Trig");  //request gpio 14 for triggering
gpio_request(15,"ECHO"); //request gpio 15 for echo
  gpio_export(30,true);  //export gpio for muxing
  gpio_export(31,true);  //export gpio for muxing  
  gpio_export(14,true);  //export gpio for muxing
  gpio_export(15,true);  //export gpio for muxing
gpio_direction_output(30,1); 
gpio_direction_output(31,1); 
gpio_direction_output(14,1); 
gpio_direction_input(15); 
gpio_set_value_cansleep(30,0);
gpio_set_value_cansleep(31,0);
//gpio_set_value_cansleep(14,0);
//gpio_set_value_cansleep(15,0);
pulse_struct=(my_pulse_struct *)kmalloc(sizeof(my_pulse_struct),GFP_KERNEL);
cdev_init(&pulse_struct->my_cdev,&my_fops);
pulse_class = class_create(THIS_MODULE, "pulse");
//cdev add
err = cdev_add (&pulse_struct->my_cdev, devno, 1);
 device_create(pulse_class, NULL, MKDEV(MAJOR(devno), 0), NULL,"pulse_irq");		
//device create
  return 0;
}
static void __exit pulse_exit(void){
printk("exiting pulse module");
  gpio_free(30);
  gpio_free(31);
  gpio_free(14);
  gpio_free(15);
unregister_chrdev_region((devno), 1);
device_destroy (pulse_class, MKDEV(MAJOR(devno), 0));
cdev_del(&pulse_struct->my_cdev);
kfree(pulse_struct);
class_destroy(pulse_class);
//printk set  output of goi to off
printk("exiting pulse module");
}
module_init(pulse_init);
module_exit(pulse_exit);
MODULE_LICENSE("Dual BSD/GPL");
