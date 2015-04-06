#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include<pthread.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include<sys/poll.h>
#include<math.h>
float ax,ay,az,gx,gy,gz,t;
static pthread_mutex_t lock;
int setup(){
	int status,fd;
	char buf[2],rdbf[2];
	fd=open("/sys/class/gpio/export",O_WRONLY);  //to export muxing for setting i2c lines
	write(fd,"29",2);
	write(fd,"15",2);
	write(fd,"30",2);
	close(fd);
	fd=open("/sys/class/gpio/gpio29/direction",O_RDWR);  //to gpio value to 0 to mux to i2c lines
	write(fd,"out",4);
	close(fd);
	fd=open("/sys/class/gpio/gpio15/direction",O_RDWR);  //to gpio value to 0 to mux to i2c lines
	write(fd,"in",2);
	close(fd);
	fd=open("/sys/class/gpio/gpio30/direction",O_RDWR);  //to gpio value to 0 to mux to i2c lines
	write(fd,"out",3);
	close(fd);
	fd=open("/sys/class/gpio/gpio29/value",O_RDWR);  //to gpio value to 0 to mux to i2c lines
	write(fd,"0",2);
	close(fd);
	fd=open("/sys/class/gpio/gpio30/value",O_RDWR);  //to gpio value to 0 to mux to i2c lines
	write(fd,"0",1);
	close(fd);
	fd=open("/dev/i2c-0",O_RDWR);  //setting up i2c to start reading and writing on i2c bus
		int out;		
	if(out=ioctl(fd,I2C_SLAVE_FORCE,0x68)<0) 				//  address set to 0b01101000
		printf(" ioctl sorry %d",out);
	buf[0]=0x75;buf[1]=0xF0;   //for who am i
	out=write(fd,&buf[0],1);
	read(fd,&rdbf[0],1);
	buf[0]=0x6B;buf[1]=0x00;   //for writing 0b000000000 to power management register to disable sleep mode if in sleep mode
	if(out=write(fd,buf,2)<0)
		printf("sorry %d",out);
	
	write(fd,&buf[0],1);
	out=read(fd,&rdbf[0],1);
	buf[0]=0x6C;buf[1]=0x00;   //for writing 0b000000000 to power management register to disable sleep mode if in sleep mode
	if(out=write(fd,buf,2)<0)
		printf("sorry %d",out);
	
	write(fd,&buf[0],1);
	out=read(fd,&rdbf[0],1);
	buf[0]=0x19;buf[1]=40;   //for setting sampling rate to 200 40= 8k/200
	write(fd,buf,2);
	out=write(fd,&buf[0],1);
	out=read(fd,&rdbf[0],1);
	buf[0]=0x38;buf[1]=0x01;   //for setting data ready enable
	write(fd,buf,2);
	out=write(fd,&buf[0],1);
	out=read(fd,&rdbf[0],1);
	buf[0]=0x1c;buf[1]=0x00;   //for accelerometer configuration
	write(fd,buf,2);
	write(fd,&buf[0],1);
	read(fd,&rdbf[0],1);
	buf[0]=0x1b;buf[1]=0x00;   //for gyro configuration
	write(fd,buf,2);
	write(fd,&buf[0],1);
	read(fd,&rdbf[0],1);
	buf[0]=0x1A;buf[1]=0x00;   //for configuration
	write(fd,buf,2);
	write(fd,&buf[0],1);
	read(fd,&rdbf[0],1);	
	buf[0]=0x23;buf[1]=0xF8;   //for configuration
	write(fd,buf,2);
	write(fd,&buf[0],1);
	read(fd,&rdbf[0],1);	
	close(fd);
		//setup gpio 15 for interrupt
	return 0;
}
int release(){
	int status,fd;
	char buf[10];
	fd=open("/sys/class/gpio/unexport",O_WRONLY);  //to unexport muxing for setting i2c lines
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
//fd=open(f_loc,O_RDWR);
//ioctl(fd,I2C_SLAVE_FORCE,0x68);
	rc = poll(&fdset, nfds, timeout);      		 
//check for interrupt 
if (rc < 0) {
			printf("\npoll() failed!\n");
if (rc == 0) {
		}
			printf(".");

		}
     		else if (fdset.revents & POLLPRI) {	
//for acceleration x
char data[6];
add_l=0x3C;add_h=0x3B;
status=write(fd,&add_l,1);
if(status<0){
return -1;
}
status=read(fd,data,6);
if(status<0){
return -1;
}
//status=write(fd,&add_h,1);
//status=read(fd,&H_byte,1);
ax=((float)((data[0]<<8)+data[1]))/(16384);
ay=((float)((data[2]<<8)+data[3]))/(16384);
az=((float)((data[4]<<8)+data[5]))/(16384);
//for gyro x
add_l=0x44;add_h=0x43;
status=write(fd,&add_l,1);
if(status<0){
return -1;
}
status=read(fd,data,6);
//status=write(fd,&add_h,1);
//status=read(fd,&H_byte,1);
if(status<0){
return -1;
}
gx=((float)250*((data[0]<<8)+data[1]))/(2*16384);
gy=((float)250*((data[2]<<8)+data[3]))/(2*16384);
gz=((float)250*((data[4]<<8)+data[5]))/(2*16384);
}
//close(fd);
close(fd_edg);
close(gpio_fd);
return status;
}
int caliberate(int *sfd){
	int fd=*sfd;
	int i,j;
	float acc[100][3],ang_vel[100][3];
	float acc_m[3],ang_vel_m[3],acc_s[3],ang_vel_s[3];
	printf("please keep the device still as it is trying to caliberate...\n");
	get_mpu_data(&fd);
	//collect 100 samples of acceleration and gyro
	for(i=0;i<100;i++){
		get_mpu_data(&fd);
		acc[i][0]=ax;
		acc[i][1]=ay;
		acc[i][2]=az;
		ang_vel[i][0]=gx;
		ang_vel[i][1]=gy;
		ang_vel[i][2]=gz;
	}
	//get the mean
	acc_m[0]=0;acc_m[1]=0;acc_m[2]=0;ang_vel_m[0]=0;ang_vel_m[1]=0;ang_vel_m[2]=0;
	for(i=0;i<100;i++){
		acc_m[0]+=acc[i][0];
		acc_m[1]+=acc[i][1];
		acc_m[2]+=acc[i][2];
		ang_vel_m[0]+=ang_vel[i][0];
		ang_vel_m[1]+=ang_vel[i][1];
		ang_vel_m[2]+=ang_vel[i][2];
	}
		acc_m[0]=acc_m[0]/100;
		acc_m[1]=acc_m[1]/100;
		acc_m[2]=acc_m[2]/100;
		ang_vel_m[0]=ang_vel_m[0]/100;
		ang_vel_m[1]=ang_vel_m[1]/100;
		ang_vel_m[2]=ang_vel_m[2]/100;
	//get std dev
	acc_s[0]=0;acc_s[1]=0;acc_s[2]=0;ang_vel_s[0]=0;ang_vel_s[1]=0;ang_vel_s[2]=0;
	for(i=0;i<100;i++){
		acc_s[0]+=(acc[i][0]-acc_m[0])*(acc[i][0]-acc_m[0]);
		acc_s[1]+=(acc[i][1]-acc_m[1])*(acc[i][1]-acc_m[1]);
		acc_s[2]+=(acc[i][2]-acc_m[2])*(acc[i][2]-acc_m[2]);
		ang_vel_s[0]+=(ang_vel[i][0]-ang_vel_m[0])*(ang_vel[i][0]-ang_vel_m[0]);
		ang_vel_s[1]+=(ang_vel[i][1]-ang_vel_m[1])*(ang_vel[i][1]-ang_vel_m[1]);
		ang_vel_s[2]+=(ang_vel[i][2]-ang_vel_m[2])*(ang_vel[i][2]-ang_vel_m[2]);
	}
		acc_s[0]=sqrt(acc_s[0]/100);
		acc_s[1]=sqrt(acc_s[1]/100);
		acc_s[2]=sqrt(acc_s[2]/100);
		ang_vel_s[0]=sqrt(ang_vel_s[0]/100);
		ang_vel_s[1]=sqrt(ang_vel_s[1]/100);
		ang_vel_s[2]=sqrt(ang_vel_s[2]/100);
		printf("acc_m 0 : %f acc_m 1 : %f acc_m 2 : %f :%f:%f :%f \n",acc_m[0],acc_m[1],acc_m[2],ang_vel_m[0],ang_vel_m[1],ang_vel_m[2]);
			printf("acc_s 0 : %f acc_s 1 : %f acc_s 2 : %f :%f:%f :%f \n",acc_s[0],acc_s[1],acc_s[2],ang_vel_s[0],ang_vel_s[1],ang_vel_s[2]);
getchar();
	printf("caliberation done\n");
	return 0;
}
float get_angular_velocity(int *sfd){
	int fd=*sfd;
	get_mpu_data(&fd);
	
}
void read_th(int *fd){
while(1){
	ax=0;ay=0;az=0;gx=0;gy=0;gz=0;t=0;
	pthread_mutex_lock(&lock);
	get_mpu_data(fd);
	pthread_mutex_unlock(&lock);	
	usleep(100);
	printf("\n ax:%f ,ay:%f ,az:%f ,gx:%f ,gy:%f ,gz:%f ,t:%f \n",ax,ay,az,gx,gy,gz,t);
	}
}
int main(){
int fd;
char *f_loc="/dev/i2c-0";
pthread_t t1,t2;
setup();
if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
fd=open(f_loc,O_RDWR);
ioctl(fd,I2C_SLAVE_FORCE,0x68);
caliberate(&fd);
//pthread_create(&t1,NULL,(void  *)&read_th,"/dev/i2c-0");
//pthread_create(&t1,NULL,(void  *)&read_th,&fd);
getchar();
sleep(5);
close(fd);
//pthread_cancel(t1);
release();
pthread_mutex_destroy(&lock);
return 0;
}
