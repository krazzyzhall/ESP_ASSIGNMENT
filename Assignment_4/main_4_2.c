#include<stdio.h>
#include<stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include<pthread.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include<sys/poll.h>
//#include<gpio.h>
int ax,ay,az,gx,gy,gz,t;
int setup(){
	int status,fd;
	char buf[2],rdbf[2];
	fd=open("/sys/class/gpio/export",O_WRONLY);  //to export muxing for setting i2c lines
	write(fd,"29",3);
	close(fd);
	getchar();
	fd=open("/sys/class/gpio/gpio29/direction",O_RDWR);  //to gpio value to 0 to mux to i2c lines
	write(fd,"out",4);
	close(fd);
	return 0;
}
int release(){
	int status,fd;
	char buf[10];
	fd=open("/sys/class/gpio/unexport",O_RDWR);  //to unexport muxing for setting i2c lines
	write(fd,"29",2);
	close(fd);
	return 0;
}
int get_mpu_data(int *sfd){
int fd=*sfd;
char add_l,add_h;
char H_byte,L_byte;
struct pollfd fdset;
int timeout=500,status;
int nfds = 1,i,fd_edg,gpio_fd,rc;
unsigned int gpio=15;
	fd_edg = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
gpio_fd=open("/sys/class/gpio/gpio15/value", O_RDONLY);
        write(fd_edg, "rising", 6);  //make the edge parameter as rising so it will poll for rising edge
	fdset.fd = gpio_fd;
	fdset.events = POLLPRI;
	fdset.revents = 0;
	rc = poll(&fdset, nfds, timeout);      		 
//check for interrupt 
if (rc < 0) {
			printf("\npoll() failed!\n");
		}
if (rc == 0) {
			printf(".");

		}
     		else if (fdset.revents & POLLPRI) {	
//for acceleration x
add_l=0x3C;add_h=0x3B;
status=write(fd,&add_l,1);
status=read(fd,&L_byte,1);
status=write(fd,&add_h,1);
status=read(fd,&L_byte,1);
ax=(int)((H_byte<<8)+L_byte);
//for acceleration y
add_l=0x3E;add_h=0x3D;
status=write(fd,&add_l,1);
status=read(fd,&L_byte,1);
status=write(fd,&add_h,1);
status=read(fd,&L_byte,1);
ay=(int)((H_byte<<8)+L_byte);
//for acceleration z
add_l=0x40;add_h=0x3F;
status=write(fd,&add_l,1);
status=read(fd,&L_byte,1);
status=write(fd,&add_h,1);
status=read(fd,&L_byte,1);
if(status<0){
return -1;
}
az=(int)((H_byte<<8)+L_byte);
//for gyro x
add_l=0x44;add_h=0x43;
status=write(fd,&add_l,1);
status=read(fd,&L_byte,1);
status=write(fd,&add_h,1);
status=read(fd,&L_byte,1);
if(status<0){
return -1;
}
gx=(int)((H_byte<<8)+L_byte);
//for gyro y
add_l=0x46;add_h=0x45;
status=write(fd,&add_l,1);
status=read(fd,&L_byte,1);
status=write(fd,&add_h,1);
status=read(fd,&L_byte,1);
if(status<0){
return -1;
}
gy=(int)((H_byte<<8)+L_byte);
//for gyro z
add_l=0x48;add_h=0x47;
status=write(fd,&add_l,1);
status=read(fd,&L_byte,1);
status=write(fd,&add_h,1);
status=read(fd,&L_byte,1);
if(status<0){
return -1;
}
gz=(int)((H_byte<<8)+L_byte);
//for temp 
add_l=0x42;add_h=0x41;
status=write(fd,&add_l,1);
status=read(fd,&L_byte,1);
status=write(fd,&add_h,1);
status=read(fd,&L_byte,1);
t=(int)((H_byte<<8)+L_byte);
}
close(fd_edg);
close(gpio_fd);
return status;
}
void read_th(int * fd){
while(1){
	get_mpu_data(fd);
	printf("\nax:%d ,ay:%d ,az:%d ,gx:%d ,gy:%d ,gz:%d ,t:%d \n",ax,ay,az,gx,gy,gz,t);
	}
}
int main(){
int fd;
fd=open("/dev/i2c-0",O_RDWR);
pthread_t t1,t2;
setup();

getchar();
pthread_create(&t1,NULL,(void  *)&read_th,&fd);
sleep(10);
pthread_cancel(t1);
release();
return 0;
}
