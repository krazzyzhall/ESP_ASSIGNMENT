#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include<pthread.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <poll.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/input.h>


int main()
{
	int fdmouse;
	struct input_event ls;
	fdmouse = open("/dev/event2",O_RDWR);

	while(1)
	{	read(fdmouse,&ls,sizeof(struct input_event));
		if(ls.code == 272){
			printf("Left Click\n");
		}
		if(ls.code == 273){
			printf("Right Click \n");
		}
		
	}
	close(fdmouse);
	return 0;
	
}

