#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<linux/types.h>
#include<linux/i2c-dev.h>
#include<sys/poll.h>
#include<math.h>
/*
struct kalman{
	
}*/

float a[3],g[3],t;
float acc_m[3],ang_vel_m[3],acc_s[3],ang_vel_s[3];
float angle[3],prev_angle[3],pos[3],prev_pos[3];
static pthread_mutex_t lock;

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

int setup(int i2c_fd){
	int status,fd,out;
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
	if(out=ioctl(i2c_fd,I2C_SLAVE_FORCE,0x68)<0) 				//  address set to 0b01101000
		printf(" ioctl sorry %d \n",out);
	buf[0]=0x6B;buf[1]=0x80;   //for reset
	if(out=write(i2c_fd,buf,2)<0)
		printf("i2c write for 6B sorry %d reset\n",out);
	
//	write(fd,&buf[0],1);
//	out=read(fd,&rdbf[0],1);
	buf[0]=0x75;		   //for who am i
	out=write(i2c_fd,&buf[0],1);
	read(i2c_fd,&rdbf[0],1);

	buf[0]=0x6B;buf[1]=0x00;   //for writing 0b000000000 to power management register to disable sleep mode if in sleep mode
//		printf("i2c_fd %d %d ",i2c_fd);
	
	if(out=write(i2c_fd,buf,2)<0)
//	write(i2c_fd,&buf[0],1);
//	out=read(i2c_fd,&rdbf[0],1);
	buf[0]=0x6C;buf[1]=0x00;   //for writing 0b000000000 to power management register to disable sleep mode if in sleep mode
	if(out=write(i2c_fd,buf,2)<0)
	{		
		printf("sorry %d",out);
	}
	
//	write(i2c_fd,&buf[0],1);
//	out=read(i2c_fd,&rdbf[0],1);
	buf[0]=0x19;buf[1]=39;   //for setting sampling rate to 200 40= 8k/200
	write(i2c_fd,buf,2);
	out=write(i2c_fd,&buf[0],1);
	out=read(i2c_fd,&rdbf[0],1);
	printf("%d  %d \n",rdbf[0],out);/*
	buf[0]=0x38;buf[1]=0x01;   //for setting data ready enable
	write(i2c_fd,buf,2);
	out=write(i2c_fd,&buf[0],1);
	out=read(i2c_fd,&rdbf[0],1);
	buf[0]=0x1c;buf[1]=0x00;   //for accelerometer configuration
	write(i2c_fd,buf,2);
	write(i2c_fd,&buf[0],1);
	read(i2c_fd,&rdbf[0],1);
	buf[0]=0x1b;buf[1]=0x00;   //for gyro configuration
	write(i2c_fd,buf,2);
	write(i2c_fd,&buf[0],1);
	read(i2c_fd,&rdbf[0],1);
	buf[0]=0x1A;buf[1]=0x00;   //for configuration
	write(i2c_fd,buf,2);
	write(i2c_fd,&buf[0],1);
	read(i2c_fd,&rdbf[0],1);	
	buf[0]=0x23;buf[1]=0xF8;   //for configuration
	write(i2c_fd,buf,2);
	write(i2c_fd,&buf[0],1);
	read(i2c_fd,&rdbf[0],1);	*/
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
int16_t read_reg(int fd,char add_l,char add_h){
	char H_byte,L_byte;
	int status;
	status=write(fd,&add_l,1);
	if(status<0){
		printf("writing address failed \n");
		return -1;
	}
	status=read(fd,&L_byte,1);
	if(status<0){
	printf("reading address failed \n");
		return -1;
	}
	status=write(fd,&add_h,1);
	if(status<0){
	printf("writing address failed \n");
		return 0xfffffffff;
	}
	status=read(fd,&H_byte,1);
	if(status<0){
	printf("reading address failed \n");
		exit(-1);
	}
	return ((H_byte<<8)+L_byte);
}

int get_gyro(int fd){
	int16_t temp;
	char add_l,add_h;
	//for gyro x
	add_l=0x44;add_h=0x43;
	temp=read_reg(fd,add_l,add_h);
	g[0]=((float)250*(temp))/(32768);
	//for gyro y
	add_l=0x46;add_h=0x45;
	temp=read_reg(fd,add_l,add_h);
	g[1]=((float)250*(temp))/(32768);
	//for gyro z
	add_l=0x48;add_h=0x47;
	temp=read_reg(fd,add_l,add_h);
	g[2]=((float)250*(temp))/(32768);
	return 0;
}

int get_acc(int fd){
	int16_t temp;
	char add_l,add_h;
	add_l=0x3C;add_h=0x3B;
	temp=read_reg(fd,add_l,add_h);
	a[0]=((float)(temp))/(16384);
	//for acceleration y
	add_l=0x3E;add_h=0x3D;	
	temp=read_reg(fd,add_l,add_h);
	a[1]=((float)(temp))/(16384);
	//for acceleration z
	add_l=0x40;add_h=0x3F;
	temp=read_reg(fd,add_l,add_h);
	a[2]=((float)(temp))/(16384);
	return 0;
}

int get_mpu_data(int *sfd){
int fd=*sfd;
int16_t temp;
char add_l,add_h;
char H_byte,L_byte;
struct pollfd fdset;
int timeout=10,status;
int nfds = 1,i,fd_edg,gpio_fd,rc;
unsigned int gpio=15;
	gpio_fd=open("/sys/class/gpio/gpio15/value", O_RDONLY);	
	fd_edg = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
	write(fd_edg, "rising", 7);  //make the edge parameter as rising so it will poll for rising edge
	close(fd_edg);
	fdset.fd = gpio_fd;
	fdset.events = POLLPRI;
	fdset.revents = 0;
	rc = poll(&fdset, 1, timeout);
	status=write(fd,&add_l,1);      		 
	status=read(fd,&L_byte,1);
	//status=write(fd,&add_l,1);
	//status=read(fd,&L_byte,1);
	//check for interrupt 
if (rc < 0) {
			printf("\npoll() failed!\n");}
if (rc == 0) {
			printf(".");
		}
     		else if (fdset.revents & POLLPRI) {	
get_acc(fd);
get_gyro(fd);
}
close(gpio_fd);
return status;
}

int caliberate(int *sfd){
//	int fd=*sfd;
	int i,j;
	float acc[100][3],ang_vel[100][3];
	printf("please keep the device still as it is trying to caliberate...\n");
//	get_mpu_data(sfd);
	//collect 100 samples of acceleration and gyro
	for(i=0;i<100;i++){
		get_mpu_data(sfd);
		acc[i][0]=a[0];
		acc[i][1]=a[1];
		acc[i][2]=a[2];
		ang_vel[i][0]=g[0];
		ang_vel[i][1]=g[1];
		ang_vel[i][2]=g[2];
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
//			printf("acc_s 0 : %f acc_s 1 : %f acc_s 2 : %f :%f:%f :%f \n",acc_s[0],acc_s[1],acc_s[2],ang_vel_s[0],ang_vel_s[1],ang_vel_s[2]);
//getchar();
	printf("caliberation done\n");
	return 0;
}
int get_angle_kal(int *sfd){
	long long int start,stop;
	start= rdtsc();
	get_mpu_data(sfd);
	stop = rdtsc();
	/*angle[0]=prev_angle[0]+((float)((g[0]-ang_vel_m[0])*(stop-start)/(400000000)));
	angle[1]=prev_angle[1]+((float)((g[1]-ang_vel_m[1])*(stop-start)/(400000000)));
	angle[2]=prev_angle[2]+((float)((g[2]-ang_vel_m[2])*(stop-start)/(400000000)));/*
	angle[0]=prev_angle[0]+((float)((g[0]-ang_vel_m[0])*(stop-start)/(400000)));
	angle[1]=prev_angle[1]+((float)((g[1]-ang_vel_m[1])*(stop-start)/(400000)));
	angle[2]=prev_angle[2]+((float)((g[2]-ang_vel_m[2])*(stop-start)/(400000)));*/
	prev_angle[0]=(angle[0]);
	prev_angle[1]=(angle[1]);
	prev_angle[2]=(angle[2]);
	
/*	prev_angle[0]=(angle[0]+prev_angle[0])/2;
	prev_angle[1]=(angle[1]+prev_angle[1])/2;
	prev_angle[2]=(angle[2]+prev_angle[2])/2;
*/}
int get_angle_reg(int *sfd){
	long long int start,stop;
//	start= rdtsc();
	get_mpu_data(sfd);
//	stop = rdtsc();
	angle[0]=prev_angle[0]+((float)((g[0]-ang_vel_m[0])/(200)));
	angle[1]=prev_angle[1]+((float)((g[1]-ang_vel_m[1])/(200)));
	angle[2]=prev_angle[2]+((float)((g[2]-ang_vel_m[2])/(200)));/*
	angle[0]=prev_angle[0]+((float)((g[0]-ang_vel_m[0])*(stop-start)/4000000));
	angle[1]=prev_angle[1]+((float)((g[1]-ang_vel_m[1])*(stop-start)/4000000));
	angle[2]=prev_angle[2]+((float)((g[2]-ang_vel_m[2])*(stop-start)/4000000));
	*/prev_angle[0]=angle[0];
	prev_angle[1]=angle[1];
	prev_angle[2]=angle[2];
}
void read_th(int *fd){
while(1){
//	a[0]=0;a[1]=0;a[2]=0;g[0]=0;g[1]=0;g[2]=0;t=0;
//	pthread_mutex_lock(&lock);
//	get_mpu_data(fd);
	get_angle_reg(fd);
//	pthread_mutex_unlock(&lock);	
//	usleep(300000);
	printf("\n angx:%f ,ay:%f ,az:%f ,gx:%f ,gy:%f ,gz:%f\n",angle[0],angle[1],angle[2],pos[0],pos[1],pos[2]);
//	printf("\n ax:%f ,ay:%f ,az:%f ,gx:%f ,gy:%f ,gz:%f ,t:%f \n",a[0],a[1],a[2],g[0],g[1],g[2],t);
	}
}

int main(){
int fd;
char *f_loc="/dev/i2c-0";
pthread_t t1,t2;
if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
fd=open(f_loc,O_RDWR);
setup(fd);
//ioctl(fd,I2C_SLAVE_FORCE,0x68);
prev_angle[0]=0;prev_angle[1]=0;prev_angle[2]=0;
prev_pos[0]=0;prev_pos[1]=0;prev_pos[2]=0;
caliberate(&fd);
//getchar();
//pthread_create(&t1,NULL,(void  *)&read_th,"/dev/i2c-0");
//pthread_create(&t1,NULL,(void  *)&read_th,&fd);
read_th(&fd);
//getchar();
//sleep(5);
//pthread_cancel(t1);
close(fd);
//release();
pthread_mutex_destroy(&lock);
return 0;
}
