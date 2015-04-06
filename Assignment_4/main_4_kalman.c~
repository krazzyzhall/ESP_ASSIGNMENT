#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <sys/poll.h>
#include<linux/i2c-dev.h>
#include<math.h>

#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#define PORT_TIME       13              /* "time" (not available on RedHat) */
#define PORT_APP        9999              /* FTP connection port */
#define MAXBUF          1024

#define dt 0.007
#define sample 39
#define MY_PI 3.14159265359

#define RESTRICT_PITCH 
char raw[14];
float a[3],g[3],gp[3],t,d[3],dp[3],u[3],v[3];
float acc_m[3],ang_vel_m[3],acc_s[3],ang_vel_s[3];
float angle[3],prev_angle[3],pos[3],prev_pos[3];
long long prev;


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
	fd=open("/sys/class/gpio/gpio15/edge",O_RDWR);  //to gpio value to 0 to mux to i2c lines
	write(fd,"rising",7);
	close(fd);
		if(out=ioctl(i2c_fd,I2C_SLAVE_FORCE,0x68)<0) 				//  address set to 0b01101000
		printf(" ioctl sorry %d \n",out);
/*	buf[0]=0x6B;buf[1]=0x80;   //for reset
	if(out=write(i2c_fd,buf,2)<0)
		printf("i2c write for 6B sorry %d reset\n",out);
	
	buf[0]=0x75;		   //for who am i
	out=write(i2c_fd,&buf[0],1);
	read(i2c_fd,&rdbf[0],1);
*/
	buf[0]=0x6B;buf[1]=0x00;   //for writing 0b000000000 to power management register to disable sleep mode if in sleep mode
	
	if(out=write(i2c_fd,buf,2)<0)
	buf[0]=0x6C;buf[1]=0x00;   //for writing 0b000000000 to power management register to disable sleep mode if in sleep mode
	if(out=write(i2c_fd,buf,2)<0)
	{		
		printf("sorry %d",out);
	}
	
	buf[0]=0x19;buf[1]=sample;   //for setti8ng sampling rate to 200 40= 8k/200
	write(i2c_fd,buf,2);
	buf[0]=0x38;buf[1]=0x01;   //for setting data ready enable
	write(i2c_fd,buf,2);/*
	buf[0]=0x1c;buf[1]=0x00;   //for accelerometer configuration
	write(i2c_fd,buf,2);
	buf[0]=0x1b;buf[1]=0x00;   //for gyro configuration
	write(i2c_fd,buf,2);
	buf[0]=0x1A;buf[1]=0x00;   //for configuration
	write(i2c_fd,buf,2);*/
	buf[0]=0x37;buf[1]=0x60;   //for setting open drain
		write(i2c_fd,buf,2);
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

int caliberate(int *sfd, int gpio_fd){
	int i,j;
	float acc[100][3],ang_vel[100][3];
	printf("please keep the device still as it is trying to caliberate...\n");
	for(i=0;i<100;i++){
		get_mpu_data(sfd,g,a);
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
//		printf("acc_m 0 : %f acc_m 1 : %f acc_m 2 : %f :%f:%f :%f \n",acc_m[0],acc_m[1],acc_m[2],ang_vel_m[0],ang_vel_m[1],ang_vel_m[2]);
//			printf("acc_s 0 : %f acc_s 1 : %f acc_s 2 : %f :%f:%f :%f \n",acc_s[0],acc_s[1],acc_s[2],ang_vel_s[0],ang_vel_s[1],ang_vel_s[2]);
// Q_angle  =  acc_s[0]; //0.001
// Q_gyro   =  ang_vel_s[0];  //0.003
// R_angle  =  acc_s[0];  //0.03
// x_bias	  =  ang_vel_m[0];	
	printf("caliberation done\n");
	return 0;
}

int get_mpu_data(int *sfd,float *g,float *a){
int16_t temp;
char add;
int timeout=100,status;
int nfds = 1,i,fd_edg;
int rc;//gpio_fd,
unsigned int gpio=15;
char buf[2],rdbf;
int out;
int sgpio_fd;
float ac_val=(9.8/16384);
float gy_val=(250.0/32768);
while(1){
	sgpio_fd=open("/sys/class/gpio/gpio15/value", O_RDWR);
	read(sgpio_fd,&rdbf,1);
	close(sgpio_fd);
	//printf("%x  %x \n",rdbf[0],rdbf[1]);
	if(rdbf=='1'){
//	printf(" %f \n",(float)(rdtsc()-prev)/400000);
//	prev=rdtsc();
	add=0x3B;  //starting address of 14 bytes
	status=write(*sfd,&add,1);
	if(status<0){
		printf("writing address failed \n");
		return -1;
	}
	status=read(*sfd,raw,14);
	if(status<0){
		printf("writing address failed \n");
		return -1;
	}
	a[0]=(float)((raw[0]<<8) + raw[1])*(ac_val);
	a[1]=(float)((raw[2]<<8) + raw[3])*(ac_val);
	a[2]=(float)((raw[4]<<8) + raw[5])*(ac_val);
	g[0]=(float)((raw[8]<<8) + raw[9])*(gy_val);
	g[1]=(float)((raw[10]<<8) + raw[11])*(gy_val);
	g[2]=(float)((raw[12]<<8) + raw[13])*(gy_val);
	buf[0]=0x3A;buf[1]=249;  
	out=write(*sfd,&buf[0],1);
	out=read(*sfd,&rdbf,1);
	break;
	}
}
return status;
}
float x_angle=0;
float Q_angle  =  0.001; //0.001
float Q_gyro   =  0.003;  //0.003
float R_angle  =  0.01;  //0.03

float x_bias = 0;
float P_00 = 0, P_01 = 0, P_10 = 0, P_11 = 0;
float  y, S;
float K_0, K_1;

float y_angle=0;
float Qy_angle  =  0.001; //0.001
float Qy_gyro   =  0.003;  //0.003
float Ry_angle  =  0.01;  //0.03

float y_bias = 0;
float Py_00 = 0, Py_01 = 0, Py_10 = 0, Py_11 = 0;
float  yy, Sy;
float Ky_0, Ky_1;

float kalmanCalculate(float newAngle, float newRate,int dir)
{
 if(dir==1){
x_angle += dt * (newRate - x_bias);
   P_00 +=  - dt * (P_10 + P_01) + Q_angle * dt;
   P_01 +=  - dt * P_11;
   P_10 +=  - dt * P_11;
   P_11 +=  + Q_gyro * dt;
   
   y = newAngle - x_angle;
   S = P_00 + R_angle;
   K_0 = P_00 / S;
   K_1 = P_10 / S;
   
   x_angle +=  K_0 * y;
   x_bias  +=  K_1 * y;
   P_00 -= K_0 * P_00;
   P_01 -= K_0 * P_01;
   P_10 -= K_1 * P_00;
   P_11 -= K_1 * P_01;
   
   return x_angle;
}
if(dir==2){
   y_angle += dt * (newRate - y_bias);
   Py_00 +=  - dt * (Py_10 + Py_01) + Qy_angle * dt;
   Py_01 +=  - dt * P_11;
   Py_10 +=  - dt * P_11;
   Py_11 +=  + Qy_gyro * dt;
   
   yy = newAngle - y_angle;
   Sy = Py_00 + Ry_angle;
   Ky_0 = Py_00 / Sy;
   Ky_1 = Py_10 / Sy;
   
   y_angle +=  Ky_0 * y;
   y_bias  +=  Ky_1 * y;
   Py_00 -= Ky_0 * Py_00;
   Py_01 -= Ky_0 * Py_01;
   Py_10 -= Ky_1 * Py_00;
   Py_11 -= Ky_1 * Py_01;
   
   return y_angle;

}/*
	x_angle += dt * (newRate - x_bias);
	P_00 +=   (dt * P_11 - P_10 - P_01 + Q_angle) * dt;
	P_01 -=   dt * P_11;
	P_10 -=   dt * P_11;
	P_11 +=  + Q_gyro * dt;

	y = newAngle - x_angle;
	S = P_00 + R_angle;
	K_0 = P_00 / S;
	K_1 = P_10 / S;

	x_angle +=  K_0 * y;
	x_bias  +=  K_1 * y;
	P_00 -= K_0 * P_00;
	P_01 -= K_0 * P_01;
	P_10 -= K_1 * P_00;
	P_11 -= K_1 * P_01;

	return x_angle;*/
}

double roll;
  double pitch;
int get_angle_kalman(int *sfd,float *angle,float *g){
int status;
#ifdef RESTRICT_PITCH // Eq. 25 and 26
  roll  = atan2f(a[1], a[2]);
  pitch = atan(-a[0] / sqrt(a[1] * a[1] + a[2] * a[2]));
#else // Eq. 28 and 29
  roll  = atan(a[1] / sqrt(a[0] * a[0] + a[2] * a[2]));
  pitch = atan2f(-accX, accZ);
#endif

#ifdef RESTRICT_PITCH
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((roll < -90 && angle[0] > 90) || (roll > 90 && angle[0] < -90)) {
    angle[0]=roll;
  } else
    angle[0]=kalmanCalculate((roll)* 180 / M_PI,g[0],1); // Calculate the angle using a Kalman filter

  if (abs(angle[0]) > 90)
    g[1] = -g[1]; // Invert rate, so it fits the restriced accelerometer reading
  angle[1] = kalmanCalculate((pitch)* 180 / M_PI, g[1], 2);
#else
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((pitch < -90 && angle[1] > 90) || (pitch > 90 && angle[1] < -90)) {
    angle[1]=pitch;
  } else
    angle[1] = kalmanCalculate((pitch)* 180 / M_PI,g[1],2); // Calculate the angle using a Kalman filter

  if (abs(angle[1]) > 90)
    g[0] = -g[0]; // Invert rate, so it fits the restriced accelerometer reading
  angle[0] =  kalmanCalculate((roll)* 180 / M_PI,g[0],1); // Calculate the angle using a Kalman filter
#endif
//angle[0]=kalmanCalculate((atan2f(a[1]-acc_m[1],a[2]-acc_m[2]))* 180 / M_PI,g[0]);
//angle[]=kalmanCalculate((atan2f(a[0]-acc_m[0],a[2]-acc_m[2])* 180 / MY_PI),g[0]);
printf("x angle : %f ",angle[0]);
printf("y angle : %f\n",angle[1]);
return status;

}
float g_1[3],g_2[3];
int get_angle_reg(int *sfd,float *angle){    //for simple integration value
	long long int start,stop;
	get_mpu_data(sfd,g,a);
	if(abs(g[0]-gp[0])>2*ang_vel_s[0])
	angle[0]=0.98*(angle[0]+((g[0]-ang_vel_m[0])*(dt)))+ 0.02*(atan2f((float)a[1], (float)a[2]) * 180 / MY_PI);
	if(abs(g[1]-gp[1])>2*ang_vel_s[1])
	angle[1]=0.98*(angle[1]+((g[1]-ang_vel_m[1])*(dt))) + 0.02*(atan2f((float)a[0], (float)a[2]) * 180 / MY_PI);
	if(abs(g[2]-gp[2])>2*ang_vel_s[2])
	angle[2]=0.98*(angle[2]+((g[2]-ang_vel_m[2])*(dt)))+ 0.02*(atan2f((float)a[1], (float)a[0]) * 180 / MY_PI);
	gp[0]=g[0];
	gp[1]=g[1];
	gp[2]=g[2];
	angle[0]=fmod(angle[0],180);
	angle[1]=fmod(angle[1],180);
	angle[2]=fmod(angle[2],180);
//		stop = start;
}
int make_conn(char *add){
int sockfd,n;
    struct sockaddr_in dest;

    /*---Open socket for streaming---*/
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("Socket");
        exit(errno);
    }

    /*---Initialize server address/port struct---*/
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT_APP);
    if ( inet_aton(add, &dest.sin_addr.s_addr) == 0 )
    {
        perror(add);
        exit(errno);
    }

    /*---Connect to server---*/
    if ( connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 )
    {
        perror("Connect ");
        exit(errno);
    }
	return sockfd;
}
int send_to_app(float *a,float *g,int sockfd){
int n;
 char buffer[MAXBUF];
    /*---Get "Hello?"---*/
    bzero(buffer, MAXBUF);
//	send data to application
	sprintf(buffer,"0.0 0.0 0.0 %f %f %f\n",g[0],g[1],g[2]);  //x y z 
//	sprintf(buffer,"0.0 0.0 0.0 %f %f %f\n",,g[1],g[2]);
	n=write(sockfd,buffer,strlen(buffer));
	if (n < 0){	
		perror("error :  writing failed for socket");
                exit(1);
	}
    printf("%s", buffer);
    /*---Clean up---*/
return 0;
}
long ts,ps=0;
float ga[3],gb[3],gc[3],td;
void read_th(int *fd,int gpio_fd,int sock_fd){
while(1){
	a[0]=0;a[1]=0;a[2]=0;g[0]=0;g[1]=0;g[2]=0;t=0;
	get_mpu_data(fd,g,a);
	g[0]=g[0]-ang_vel_m[0];	
	g[1]=g[1]-ang_vel_m[1];	
	g[2]=g[2]-ang_vel_m[2];
	get_angle_kalman(fd,angle,g);
/*
	get_angle_reg(fd,angle);
	ts=rdtsc();
	td=(float)(ts-ps)/400000000.0;
	ga[0]=M_PI*(angle[0]-prev_angle[0])/(dt*180); //get angular velocity from angle
	ga[1]=M_PI*(angle[1]-prev_angle[1])/(dt*180); //get angular velocity from angle
	ga[2]=M_PI*(angle[2]-prev_angle[2])/(dt*180); //get angular velocity from angle
	gc[0]=ga[0]-gb[0];
	gc[2]=ga[2]-gb[2];
	gc[1]=ga[1]-gb[1];
	send_to_app(a,gc,sock_fd);
	gb[0]=ga[0];
	gb[1]=ga[1];
	gb[2]=ga[2];
	prev_angle[0]=angle[0];
	prev_angle[1]=angle[1];
	prev_angle[2]=angle[2];
	ps=ts;*/
//getchar();
//	get_angle_pos(fd);
//get_mpu_data(fd,g,a);   //file discriptor,gyro mat, acceleration mat,
//	printf("\n x:%f ,y:%f ,z:%f ,al:%f ,be:%f ,ga:%f\n",d[0],d[1],d[2],angle[0],angle[1],angle[2]);
//	printf("\n ax:%f ,ay:%f ,az:%f ,gx:%f ,gy:%f ,gz:%f ,t:%f \n",a[0],a[1],a[2],g[0],g[1],g[2],t);
//	printf("\n ax:%f ,ay:%f ,az:%f ,gx:%f ,gy:%f ,gz:%f ,t:%f \n",a[0]-acc_m[0],a[1]-acc_m[1],a[2]-acc_m[2],d[0],d[1],d[2],t);
//	printf("\n ax:%f ,ay:%f ,az:%f \n",d[0],d[1],d[2]);
	}
}

int main(int argc,char *argv[]){
int fd,i;
char *f_loc="/dev/i2c-0";
pthread_t t1,t2;
angle[0]=0;
fd=open(f_loc,O_RDWR);
setup(fd);
int sock_fd=make_conn(argv[1]);
int gpio_fd=9;
caliberate(&fd,gpio_fd);
getchar();
//prev_angle[0]=0;prev_angle[1]=0;prev_angle[2]=0;
//prev_pos[0]=0;prev_pos[1]=0;prev_pos[2]=0;
//open("/sys/class/gpio/gpio15/value", O_RDONLY);	
//caliberate(&fd,gpio_fd);
read_th(&fd,gpio_fd,sock_fd);
close(fd);
    close(sock_fd);
return 0;
}
