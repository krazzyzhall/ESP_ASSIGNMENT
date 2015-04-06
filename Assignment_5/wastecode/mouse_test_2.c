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
#include <poll.h>
#include <linux/input.h>
#include <setjmp.h>
jmp_buf jumper,jumpto1,jumpto2;
long int cnt,stop;
pthread_t p1,p2;
static pthread_mutex_t lock;
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

void *poll_clicks(int *fd){
int fdmouse=*fd;
int dummy,no_clickl,no_clickr;
char buf[3];
no_clickr=0;
//prev=0;
	while(1)
	{//	printf("before read \n");
		dummy=setjmp(jumpto1);   // saves all the registers that are on stack for restoring 
		if(dummy==0 || dummy==1){
		buf[0]=0;
		read(fdmouse,buf,3);    //reads click value
		printf("in if abeforre read ,%x %x %x\n",buf[0],buf[1],buf[2]);	
		if(buf[0] & 0x02){
			printf("Left Click making cnt=0\n");
			printf("count value : %ld \n",cnt);
//			longjmp(jumpto2,1);
			printf("after long jump\n");
		}
		else if(buf[0] & 0x01){
			printf("Right Click \n");
			close(fdmouse);
			exit(0);
		}
//		}
//		else{
//			printf("not in setjmp\n");
		}
	}
}
//static pthread_mutex_t lock;
int main()
{
	int fdmouse;
	struct input_event ls;
	char buf[3];
	fdmouse = open("/dev/input/mice",O_RDONLY);
cnt=0;
int dummy=0;
long long int time,prev;
stop=0;
prev=0;
pthread_create(&p1,NULL,poll_clicks,&fdmouse);
while(1);
printf("after a while \n");
	close(fdmouse);
	return 0;	
}

