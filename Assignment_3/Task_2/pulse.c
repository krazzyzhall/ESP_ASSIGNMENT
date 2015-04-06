#include"pulse.h"


static int devno;
static struct class *pulse_class;
static my_pulse_struct *pulse_struct;

static irqreturn_t pulse_handler(int irq, void *dev_id, struct pt_regs *regs){
if(pulse_struct->read_rising==1){
  irq_set_irq_type(pulse_struct->irq_15,IRQF_TRIGGER_FALLING);
  pulse_struct->read_falling=1;
  pulse_struct->read_rising=0;
  pulse_struct->rising=rdtsc();
  return IRQ_HANDLED;
}
if(pulse_struct->read_falling==1){
  pulse_struct->read_falling=0;
  pulse_struct->ready_read=1;
  pulse_struct->falling=rdtsc();
  return IRQ_HANDLED;
}
pulse_struct->write_ready=1;
return IRQ_NONE;
}

int pulse_open(struct inode *inode, struct file *file)
{
int status =0,retval;
unsigned long flags;
	        pulse_struct->write_ready=1;	
	        pulse_struct->read_falling=0;
	        pulse_struct->read_rising=0;
	        pulse_struct->ready_read=0;
        pulse_struct->irq_15=gpio_to_irq(15);
 	printk("retval : in if before open  irq number %d \n",pulse_struct->irq_15);
        //enable_irq(pulse_struct->irq_15);
        udelay(11);
	flags = IRQF_TRIGGER_RISING;
        if((retval=request_irq(pulse_struct->irq_15,pulse_handler,flags,"pulse",pulse_struct->dev_id))==0){	
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
	if(pulse_struct->write_ready==1){
	        pulse_struct->read_rising=1;
	        gpio_set_value_cansleep(14,1);
	        udelay(11);
	        gpio_set_value_cansleep(14,0);
	 	irq_set_irq_type(pulse_struct->irq_15,IRQF_TRIGGER_RISING);
		pulse_struct->write_ready=0;
		return status;
	}
	else{
		printk("\npulse driver: write busy\n");
	return -status;
	}
}
ssize_t pulse_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
int status =0;
long dist=0;
if(pulse_struct->ready_read==1){
  pulse_struct->diff=pulse_struct->falling-pulse_struct->rising;
pulse_struct->ready_read=0;
pulse_struct->write_ready=1;
copy_to_user(buf,&pulse_struct->diff,sizeof(pulse_struct->diff));
return 1;
}
else{return -1;}
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
