#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/poll.h>
#include <linux/spi/spidev.h>
#include <pthread.h>

#define POLL_TIMEOUT 500
#define MY_CPU_FREQ 400000000
#define TEST_SIZE 10

static unsigned long ret_time;
static float Dist;
pthread_mutex_t lock;
struct pollfd *fdset;

int fd;

struct myparam{
char *floc;
void *buf;
int count;
};

typedef struct{
char led[8];
int stime;
} sequence;

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
/*
spi setup functions
*/
int set_spi();
/**
spi send function which will write a single pattern to spi bus
**/
int send_pat(int fd,char *buf,int count);
/**
gpio setup for ultrasonic sensor
**/
int set_gpio_usonic();
/**
sends a trigger on gpio 14 
**/
int send_echo();
/**
consistantly sends trigger on gpio 14
**/
void read_val(int *fd);
/**
will send pattern on 
**/
int send_seq(int fd,sequence *buf,int count);
/**
this function is to iterate sequence
**/
void itr_seq(void *param);


int set_spi(char *floc){
int fd;
char data[2];
int speed,bitperword,msb,res,i;
bitperword=8;
speed=10000000;
msb=0;
fd=open(floc,O_RDWR);
printf("%d \n",fd);
ioctl(fd,SPI_IOC_WR_LSB_FIRST,&msb);
ioctl(fd,SPI_MODE_0);
ioctl(fd,SPI_IOC_WR_BITS_PER_WORD,&bitperword);
ioctl(fd,SPI_IOC_WR_MAX_SPEED_HZ,&speed);
data[1]=0x00;        //to remove from mode
data[0]=0x0F;
res=write(fd,data,2);
printf("res : %d\n",res);
data[1]=0x01;        //to set to normal mode
data[0]=0x0C;
res=write(fd,data,2);
printf("res : %d\n",res);
data[1]=0x00;       // to set to decode
data[0]=0x09;
res=write(fd,data,2);
printf("res : %d\n",res);
data[1]=0x07;      //scan limit
data[0]=0x0B;
res=write(fd,data,2);
printf("res : %d\n",res);
data[1]=0x08;
data[0]=0x0A;
res=write(fd,data,2);
for(i=0;i<8;i++){
data[0]=i;
data[1]=0x00;
res=write(fd,data,2);}
close(fd);
printf("spi_setting_done\n");
return 0;
}

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
        write(fd, "out", 4); 
  close(fd);
fd = open("/sys/class/gpio/gpio14/direction", O_WRONLY);
        write(fd, "out", 4);  //for trig 
  close(fd);
fd = open("/sys/class/gpio/gpio30/direction", O_WRONLY);
        write(fd, "out", 4);  //for trig mux
  close(fd);
fd = open("/sys/class/gpio/gpio31/direction", O_WRONLY);
        write(fd, "out", 4);  //for echo mux
  close(fd);
fd = open("/sys/class/gpio/gpio15/direction", O_WRONLY);
        write(fd, "in", 3);  //for Echo
        close(fd);
fd = open("/sys/class/gpio/gpio0/value", O_WRONLY);
        write(fd, "0", 2);  
  close(fd);
fd = open("/sys/class/gpio/gpio14/value", O_WRONLY);
        write(fd, "0", 2);  //for trig
  close(fd);
fd = open("/sys/class/gpio/gpio30/value", O_WRONLY);
        write(fd, "0", 2);  //for trig mux
  close(fd);
fd = open("/sys/class/gpio/gpio31/value", O_WRONLY);
        write(fd, "0", 2);  //for Echo mus
  close(fd);
fd = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
        write(fd, "rising", 7);  //for Echo
  close(fd);
printf("ultrasonic gpio set \n");
return 0;
}


void read_val(int *sfd){
int dist=0;
long long ret_time=0,falltime=0,risetime=0;
struct pollfd fdset;
	int nfds = 2,i,fd,fd_edg;
	int gpio_fd, timeout, rc;
	char *buf[100];
	unsigned int gpio=15; 
	int len;
	timeout=POLL_TIMEOUT;
	int flag=0;
	                        fd_edg = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
printf("starting to read Distance\n");
while(1){
	fd = open("/sys/class/gpio/gpio14/value", O_WRONLY);
	write(fd, "1", 1);  //on time for trigger
        usleep(11);
        write(fd, "0", 1);  // switch off trigger
	close(fd);
        write(fd_edg, "rising", 6);  //make the edge parameter as rising so it will poll for rising edge
      gpio_fd=open("/sys/class/gpio/gpio15/value", O_RDONLY);
		fdset.fd = gpio_fd;
		fdset.events = POLLPRI;
		fdset.revents = 0;
		rc = poll(&fdset, nfds, timeout);      
		read(gpio_fd,buf[0],1);

		if (rc < 0) {
			printf("\npoll() failed!\n");
		}
      
		if (rc == 0) {
			printf(".");

		}
     		else if (fdset.revents & POLLPRI) {
			pthread_mutex_lock(&lock); //lock for atomicity
			flag=1;
			risetime=rdtsc(); // record the rise time after an edge is recieved
        		write(fd_edg, "falling", 8);  //change edge to falling 
                        rc = poll(&fdset, nfds, timeout);  //poll for falling edge
			read(gpio_fd,buf[0],1);            //dummy read
			if(rc<0){printf("\npoll() failed!\n");
				}
			if (rc == 0) {
				printf(".");
			}
			else if(fdset.revents & POLLPRI) { 
				falltime=rdtsc();          //record time to read falling edge time
					ret_time=falltime-risetime;    //get total time elapsed
					Dist=(float)((float)340*ret_time)/(MY_CPU_FREQ*2);  //calculate distance
	//				printf("Dist : %f \n",Dist);
			}pthread_mutex_unlock(&lock);   //release lock
		}
close(gpio_fd);
usleep(400000);
}
}

int send_seq(int fd,sequence *buf,int count){
int i;
	for (i=0;i<count;i++){
	if(buf[i].led!=NULL){
		pthread_mutex_lock(&lock);
		send_pat(fd,buf[i].led,8);
		pthread_mutex_unlock(&lock);
		usleep((1000*buf[i].stime)/((Dist)+1)); 
		}
	else
		break;
	}
}
int send_pat(int fd,char *buf,int count){    //writes a pattern byte by byte 
int i;
char data[2]; 
	for(i=0;i<count;i++){
		data[0]=i+1;
		data[1]=buf[i];
		write(fd,data,2);
	}
	return 0;
}
void itr_seq(void * pparam){
struct myparam *param=(struct myparam*)pparam;
char *floc=param->floc;
int fd=open(floc,O_WRONLY);
sequence *p1; 
p1=param->buf;
int count=param->count;
	while(1){  //repeat
		send_seq(fd,p1,count);  //write seq on led display
	}
}


