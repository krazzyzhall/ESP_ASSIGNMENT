#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <pthread.h>

#define MY_CPU_FREQ 4000000

static float Dist;

pthread_mutex_t lock;
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

float get_dist(int fd);

float get_speed(int fd);

int get_direction(int fd);


float get_dist(int fd){
float dist=0;
int status;
	write(fd,NULL,1);
	usleep(1000);
	status=read(fd,&measure,1);
	if(status<0){
		return -1;
	}
	else{
		dist=(float)((float)340*measure)/(MY_CPU_FREQ*2);
		return dist;
	}
}

float get_speed(int fd){
float dist[2],speed;
long tbefore=0,tafter=0;
int status;
	tbefore=rdtsc();
	dist[0]=get_dist(fd);
	usleep(10);
	dist[1]=get_dist(fd);
	tafter=rdtsc();
	if(dist[0]<0 || dist[1]<0){
		return -1;
	}
	else{
		if(dist[1]>dist[0]){
		speed=(dist[1]-dist[0])*(MY_CPU_FREQ)/(tafter-tbefore);
		return speed;}
		else{
			speed=(dist[0]-dist[1])*(MY_CPU_FREQ)/(tafter-tbefore);
		return speed;}
	}
}

int get_direction(int fd){
float dist[2];
	dist[0]=get_dist(fd);
	usleep(10);
	dist[1]=get_dist(fd);
	if(dist[1]>=dist[0])
	return 1;
	else
	return -1;
}
