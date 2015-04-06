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

#include <asm-generic/errno.h>
#include <asm-generic/errno-base.h>
#include"led_mat.h"

#define N_SPI_MINORS                    32      /* ... up to 256 */
#define SPI_MAJOR 153

