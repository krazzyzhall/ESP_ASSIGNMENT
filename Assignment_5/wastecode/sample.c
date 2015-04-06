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
char buf[3];
	while(1)
	{//	printf("before read \n");
		if(setjmp(jumpto1)==0 || setjmp(jumpto1)==1){
		printf("in if abeforre read\n");	
		read(fdmouse,buf,3);
		if(buf[0] & 0x1){
			printf("Left Click making cnt=0\n");
			printf("count value : %lu\n",cnt);
			pthread_mutex_lock(&lock);
			cnt=0;
			stop=1;
			pthread_mutex_unlock(&lock);
			longjmp(jumpto2,1);
			printf("after long jump\n");
		}
		if(buf[0] & 0x2){
			stop=1;
			printf("Right Click \n");
//			exit(0);
		}
		}
//		else{
//			printf("not in setjmp\n");
//		}
	}
}
void *compute(){

while(1){
	if(setjmp(jumpto2)==0){
		//while(){
//		cnt++;
		}

	else{
	printf("cnt : %lu \n",cnt);
	cnt=0;
	longjmp(jumpto1,0);
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
int dummy;
long long int time,prev;
stop=0;
pthread_create(&p1,NULL,poll_clicks,&fdmouse);
while(1){
	dummy=setjmp(jumpto2);

		printf("dummy %d \n",dummy);
	if(dummy==0 || dummy==1){
		printf("do something %d \n",dummy);
		time=rdtsc()-prev;
		printf("time elapsed in between clicks :%f \n",(time/400000));
		prev=rdtsc();
	longjmp(jumpto1,1);
	while(1);
		}
	else{
	printf("cnt : %lu  %d %d\n",cnt,dummy,time/400000);
	cnt=0;
	longjmp(jumpto1,1);
	}
}
prinf("after a while \n");
	close(fdmouse);
	return 0;	
}

