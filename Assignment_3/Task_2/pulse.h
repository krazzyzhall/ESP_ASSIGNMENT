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

typedef struct {
int trig;
unsigned long echo;
unsigned long long rising;
unsigned long long falling;
long diff;
int write_ready;
int ready_read;
int read_falling;
int read_rising;
int irq_15;
void *dev_id;
int samples[10];
struct cdev my_cdev;
}my_pulse_struct;


static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

