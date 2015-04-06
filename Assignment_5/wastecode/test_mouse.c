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
no_clickl=0;
	{
		dummy=setjmp(jumpto1);   // saves all the registers that are on stack for restoring 
		if(dummy==0 || dummy==1){
		buf[0]=0;
		while(1){
		read(fdmouse,buf,3);    //reads click value
		if(buf[0] & 0x02){
			no_clickl=0;
			no_clickr++;
			if(no_clickr==2){
			no_clickr=0;
			longjmp(jumpto2,1);}
			else
				continue;
		}
		else if(buf[0] & 0x01){
			no_clickl++;
			no_clickr=0;	
			printf("left Click: exiting \n");
			close(fdmouse);
			exit(0);
		}}
		}
		else{
			printf("not in setjmp\n");
		}
	}
}


int main(int argc,char *argv[]){
	int fdmouse;
	struct input_event ls;
	char buf[3];
	fdmouse = open("/dev/input/mice",O_RDONLY);
cnt=0;
int dummy=0;
long long int time,prev;
stop=0;
prev=rdtsc();
printf("This is task one which shows setjmp and longjmp functionality. \n In this task i will basically calculate time difference between 2 double right clicks. \n i have not timed double clicks so no matter how far are the 2 clicks time will be displayed after 2 clicks.\n double rignt click will display time difference between clicks which are in multiples of 2 \n left click will end the program \n");
pthread_create(&p1,NULL,poll_clicks,&fdmouse);
while(1){
	dummy=setjmp(jumpto2);

//		printf("dummy %d \n",dummy);
	if(dummy==0 || dummy==1){
//		printf("do something %d \n",dummy);
		time=rdtsc()-prev;
		printf("time elapsed in between clicks :%lld miliseconds\n",(time/2300000));
		prev=rdtsc();
	longjmp(jumpto1,1);
//	while(1);
		}
}

prinf("after a while \n");
	close(fdmouse);    //to close mouse fd
	return 0;	
}

