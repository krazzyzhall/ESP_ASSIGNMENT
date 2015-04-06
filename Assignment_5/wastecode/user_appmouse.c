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
#include <setjmp.h>
jmp_buf jumper,jumpto1,jumpto2;
long int cnt,stop;
pthread_t p1,p2;
static pthread_mutex_t lock;
void poll_clicks(int *fd){
int fdmouse=*fd;
char buf[3];
	while(1)
	{	printf("before read \n");
		read(fdmouse,buf,3);
		if(buf[0] & 0x2){
			printf("Left Click making cnt=0\n");
			printf("count value : %lu\n",cnt);
			cnt=0;
			stop=1;
//			longjmp(jumpto2,2);
		}
		if(buf[0] & 0x1==1){
			stop=1;
			printf("Right Click \n");
		}
	}
}
//static pthread_mutex_t lock;
int main()
{
	int fdmouse;
	struct input_event ls;
	char buf[3];
	fdmouse = open("/dev/input/mouse1",O_RDONLY);
cnt=0;
stop=0;
if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return -1;
    }
pthread_create(&p1,NULL,poll_clicks,(void *)&fdmouse);
//pthread_cancel(p1);
	close(fdmouse);
	return 0;	
}

