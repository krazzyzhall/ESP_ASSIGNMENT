#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <pthread.h>
#define POLL_TIMEOUT 50
#define MY_CPU_FREQ 400000000
static unsigned long ret_time;
static float Dist;
pthread_mutex_t lock;
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
struct pollfd *fdset;
int main(){
unsigned long long measure;
float Dist;
int j=0;
int fd=open("/dev/pulse_irq",O_RDWR);
long val=100000;
while(j<10){
j++;
write(fd,NULL,1);
usleep(val);
}
close(fd);
return 0;
}
