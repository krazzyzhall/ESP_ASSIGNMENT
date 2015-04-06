#include "Application.h"
/*#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <pthread.h>
#define POLL_TIMEOUT 100
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
*/
int set_gpio_usonic(){
int gp,fd;
fd = open("/sys/class/gpio/export", O_WRONLY);
        if(fd<0){
        printf("failed\n");
        }
        write(fd, "14", 3);  //for Echo
        write(fd, "0", 2);  //for Echo
        write(fd, "30", 3);  //for Echo
        write(fd, "31", 3);  //for Echo
        write(fd, "15", 3);  //for Trig
  close(fd);
fd = open("/sys/class/gpio/gpio0/direction", O_WRONLY);
        write(fd, "out", 4);  //for Echo
  close(fd);
fd = open("/sys/class/gpio/gpio14/direction", O_WRONLY);
        write(fd, "out", 4);  //for Echo
  close(fd);
fd = open("/sys/class/gpio/gpio30/direction", O_WRONLY);
        write(fd, "out", 4);  //for Echo
  close(fd);
fd = open("/sys/class/gpio/gpio31/direction", O_WRONLY);
        write(fd, "out", 4);  //for Echo
  close(fd);
fd = open("/sys/class/gpio/gpio15/direction", O_WRONLY);
        write(fd, "in", 3);  //for Echo
        close(fd);
fd = open("/sys/class/gpio/gpio0/value", O_WRONLY);
        write(fd, "0", 2);  //for Echo
  close(fd);
fd = open("/sys/class/gpio/gpio14/value", O_WRONLY);
        write(fd, "0", 2);  //for Echo
  close(fd);
//fd = open("/sys/class/gpio/gpio15/value", O_RDWR);
 //       write(fd, "", 2);  //for Echo
 // close(fd);
fd = open("/sys/class/gpio/gpio30/value", O_WRONLY);
        write(fd, "0", 2);  //for Echo
  close(fd);
fd = open("/sys/class/gpio/gpio31/value", O_WRONLY);
        write(fd, "0", 2);  //for Echo
  close(fd);
fd = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
        write(fd, "rising", 7);  //for Echo
  close(fd);
return 0;
}
int send_echo(){
int fd;
//printf("in echo send\n ha\n");
fd = open("/sys/class/gpio/gpio14/value", O_WRONLY);
        write(fd, "1", 2);  //for Echo
        usleep(20);
        write(fd, "0", 2);  //for Echo
  close(fd);
return 0;
}
int receive_edge(int fd){
int dist=0;
long long ret_time=0,falltime=0,risetime=0;
struct pollfd fdset[2];
	int nfds = 2,i;
	int gpio_fd, timeout, rc;
	char *buf[100];
	unsigned int gpio=15; 
	int len;
	int flag=0;
fd = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
        write(fd, "rising", 7);  //for Echo
  close(fd);
fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;
      gpio_fd=open("/sys/class/gpio/gpio15/value", O_RDONLY);
//read(gpio_fd,buf[0],1);
//read(gpio_fd,&buf[0],1);
//for(i=0;i<8;i++){
//usleep(400);
//}
		fdset[1].fd = gpio_fd;
		fdset[1].events = POLLPRI;
		rc = poll(fdset, nfds, timeout);      
		if (rc < 0) {
			printf("\npoll() failed!\n");
			return -1;
		}
      
		if (rc == 0) {
			printf(".");
		}
            
		else if (fdset[1].revents & POLLPRI) {
printf("000    dont know what to do \n%d  %d %d %d ",fd,fdset[1].events,fdset[1].revents,fdset[1].revents & POLLPRI);
pthread_mutex_lock(&lock);
			flag=1;
			risetime=rdtsc(); //len = read(fdset[1].fd, buf, MAX_BUF);
                        fd = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
        		write(fd, "falling", 8);  //for Echo
  			close(fd);
                        rc = poll(fdset, nfds, timeout);
				if(rc<0){printf("\npoll() failed!\n");
			return -1;}
			if (rc == 0) {
			printf(".");fd = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
        write(fd, "rising", 7);  //for Echo
  close(fd);
}
			else if(fdset[1].revents & POLLPRI) { falltime=rdtsc(); //len = read(fdset[1].fd, buf, MAX_BUF);
		//	printf("\npoll() GPIO %d interrupt occurred\n", gpio);
			if(falltime>risetime && falltime-risetime <=0.0235*MY_CPU_FREQ){
ret_time=falltime-risetime;
Dist=(float)((float)340*ret_time)/(MY_CPU_FREQ*2);
//usleep(10000);
}
pthread_mutex_unlock(&lock); 
close(gpio_fd);
printf("time   val : %f \n",Dist);

}

		}

		if (fdset[0].revents & POLLIN) {
			(void)read(fdset[0].fd, buf, 1);
			printf("\npoll() stdin read 0x%2.2X\n", (unsigned int) buf[0]);
		}

		fflush(stdout);
return dist;
}
typedef struct {
char led[8];
}pattern;
void sendint(){
while(1){
send_echo();
usleep(30000);
}
}
void rcv_int(int *fd){
while(1){
//send_echo();
receive_edge(*fd);
usleep(120000);
}
}
char buf[4];
void read_val(int *fd){
unsigned long rising,falling;
while(1){
pthread_mutex_lock(&lock);
*fd=open("/sys/class/gpio/gpio15/value",O_RDONLY);
read(*fd,buf,2);
close(*fd);
*fd=open("/sys/class/gpio/gpio15/value",O_RDONLY);
read(*fd,buf+2,2);
close(*fd);
pthread_mutex_unlock(&lock); 
if(buf[0]=='0' && buf[2]=='1'){
//printf("in rising\n");
rising=rdtsc();
}
if(buf[0]=='1' && buf[2]=='0'){
falling=rdtsc();
//printf("in falling\n");
}
if(buf[0]=='0' && buf[2]=='0'){
rising=0;
falling=0;
}
if(falling>rising && falling-rising<0.0235*MY_CPU_FREQ){
ret_time=falling-rising;
Dist=(float)((float)340*ret_time)/(MY_CPU_FREQ*2);
printf("time   val : %f \n",Dist);

}
//printf("read val : %s \n",buf);
//usleep(10000);
}
}
int main(){
char p1[8],p2[8],buf[2];
p1[0]=0x00;
p1[1]=0x00;
p1[2]=0x02;
p1[3]=0x83;
p1[4]=0x7F;
p1[5]=0x24;
p1[6]=0x22;
p1[7]=0x62;
p2[0]=0x00;
p2[1]=0x00;
p2[2]=0x02;
p2[3]=0x03;
p2[4]=0x7F;
p2[5]=0xC2;
p2[6]=0x32;
p2[7]=0x16;
pthread_t t1,t2,t3;
if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
set_gpio_usonic();
int fd=open("/sys/class/gpio/gpio15/value",O_RDWR);
pthread_create( &t1, NULL, (void*)sendint,NULL);
pthread_create( &t2, NULL, (void*)rcv_int,&fd);
//pthread_create( &t3, NULL, (void*)read_val,&fd);
usleep(10000000);
usleep(10000000);
//pthread_cancel(t1);
pthread_cancel(t2);
//pthread_cancel(t3);
return 0;
}
