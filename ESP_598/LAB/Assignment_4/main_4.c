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

#define dt 0.008
#define sample 39
#define MY_PI 3.14159265359

#define RESTRICT_PITCH 
char raw[14];
float a[3],g[3],gp[3],t,dp[3],u[3],v[3];
float acc_m[3],ang_vel_m[3],acc_s[3],ang_vel_s[3];
float angle[3],prev_angle[3],pos[3],prev_pos[3];
long long prev;


static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

float **mat_mult(float **a,int ax,int ay,float **b,int bx,int by){
int i,j,k;
	float	**ans=(float **)malloc(ax*sizeof(float *));
	printf("\n seg fault after mult  \n");
	for(i=0;i<ax;i++){
	ans[i]=malloc(by*sizeof(float));
}
	printf("\n seg fault after malloct ,%f %f %f \n",ans[0][4],ans[1][0],ans[2][0]);

	if(ay!=bx){
		printf("matrices cannot be multiplied\n");
	}
	else{

		for( i=0; i<ax; i++){
			for(j=0; j<by; j++){
				for(k=0; k < ay; k++){
						printf("\n seg fault after malloct ,%d %d %d  %f %f %f\n",i,j,k,*(*(b+k)+j));
						getchar();

				//	ans[i][j]+= a[i][k]*b[k][j];
				}
			}
		}
	printf("\n seg fault in elset  \n");
	}
return ans;
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
	write(i2c_fd,buf,2);
	buf[0]=0x1c;buf[1]=0x00;   //for accelerometer configuration
	write(i2c_fd,buf,2);
	buf[0]=0x1b;buf[1]=0x00;   //for gyro configuration
	write(i2c_fd,buf,2);
	buf[0]=0x1A;buf[1]=0x00;   //for configuration
	write(i2c_fd,buf,2);
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

int caliberate(int *sfd){
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
		printf("acc_m 0 : %f acc_m 1 : %f acc_m 2 : %f :%f:%f :%f \n",acc_m[0],acc_m[1],acc_m[2],ang_vel_m[0],ang_vel_m[1],ang_vel_m[2]);

	printf("caliberation done\n press any key to continue....\n");
getchar();
	return 0;
}

int get_mpu_data(int *sfd,float *g,float *a){
int16_t temp;
char add;
int timeout=100,status;
int nfds = 1,i,fd_edg;
int rc;
unsigned int gpio=15;
char buf[2],rdbf;
int out;
int sgpio_fd;
float ac_val=(9.8/16384.0);
float gy_val=(250.0/32768);
while(1){
	sgpio_fd=open("/sys/class/gpio/gpio15/value", O_RDWR);
	read(sgpio_fd,&rdbf,1);
	close(sgpio_fd);
	if(rdbf=='1'){
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
	a[2]=(float)((raw[4]<<8) + raw[5])*(ac_val);  //there is an error in my MPU 14.
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

struct kalmanpar{
    float Q_angle;         // Process noise variance for the accelerometer
    float Q_bias;          // Process noise variance for the gyro bias
    float R_measure;       // Measurement noise variance - this is actually the variance of the measurement noise

    float angle; // The angle calculated by the Kalman filter - part of the 2x1 state vector
    float bias; // The gyro bias calculated by the Kalman filter - part of the 2x1 state vector
    float rate; // Unbiased rate calculated from the rate and the calculated bias - you have to call getAngle to update the rate

    float P[2][2]; // Error covariance matrix - This is a 2x2 matrix
    float K[2]; // Kalman gain - This is a 2x1 vector
    float y; // Angle difference
    float S; // Estimate error
    float newAngle,newRate;
}kx,ky;

float kalmanCalculate(struct kalmanpar *k)
{
   k->angle += dt * (k->newRate - k->bias);
   k->P[0][0] +=  - dt * (k->P[1][0] + k->P[0][1]) + k->Q_angle * dt;
   k->P[0][1] +=  - dt * k->P[1][1];
   k->P[1][0] +=  - dt * k->P[1][1];
   k->P[1][1] +=  + k->Q_bias * dt;
   
   k->y = k->newAngle - k->angle;
   k->S = k->P[0][0] + k->R_measure;
   k->K[0] = k->P[0][0] / k->S;
   k->K[1] = k->P[1][0] / k->S;
   
   k->angle +=  k->K[0] * k->y;
   k->bias  +=  k->K[1] * k->y;
   k->P[0][0] -= k->K[0] * k->P[0][0];
   k->P[0][1] -= k->K[0] * k->P[0][1];
   k->P[1][0] -= k->K[1] * k->P[0][0];
   k->P[1][1] -= k->K[1] * k->P[0][1];
   
   return 0;

}

float roll;
float pitch,ap[3];
float get_angle_pos(float *g,float *a,float *d){
float acc;
	if( abs(a[0]) >(1/20)*acc_s[0] ){
	acc  = -(a[0]);		
	v[0] =(acc*dt);
	d[0]    = d[0] + v[0]*dt;
	}

	if( abs(a[1]) >(1/20)*acc_s[1] ){
	acc  = -(a[1]);		
	v[1] = (acc*dt);
	d[1] = d[1] + v[1]*dt;
	ap[1] =a[1];
	}

	if( abs(a[2]) >(1/20)*acc_s[2] ){
	acc  = -(a[2]);		
	v[2] =(acc*dt);
	d[2]    = d[2] + v[2]*dt;
	ap[2]=a[2];
	}

	
}

int get_angle_kalman(int *sfd,float *angle,float *g,float *a){
int status;
static float tgp=0;
kx.Q_angle=0.001; // Process noise variance for the accelerometer
kx.Q_bias=0.001; // Process noise variance for the gyro bias
kx.R_measure=0.01; // Measurement noise variance - this is actually the variance of the measurement noise

ky.Q_angle=0.001; // Process noise variance for the accelerometer
ky.Q_bias=0.001; // Process noise variance for the gyro bias
ky.R_measure=0.01; // Measurement noise variance - this is actually the variance of the measurement noise

#ifdef RESTRICT_PITCH // Eq. 25 and 26
/*for calculating value of x_angle*/
if(a[2]>=0){
	roll  = atan2f(a[1], sqrt(a[0] * a[0] + a[2] * a[2]))* 180 / M_PI;  
}
else{
	roll  = atan2f(a[1],-sqrt(a[0] * a[0] + a[2] * a[2]))* 180 / M_PI;  
}
	pitch = atan(-a[0] / sqrt(a[1] * a[1] + a[2] * a[2]))* 180 / M_PI;//}
#else // Eq. 28 and 29
  roll  = atan(a[1] / sqrt(a[0] * a[0] + a[2] * a[2]))* 180 / M_PI;
  pitch = atan2f(-a[0], a[2])* 180 / M_PI;
#endif

#ifdef RESTRICT_PITCH
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
  if ((roll < -90 && angle[0] > 90) || (roll > 90 && angle[0] < -90)) {
    angle[0]=roll;
    kx.angle=roll;
  } else{
	kx.newAngle=roll;
	kx.newRate=g[0];
    kalmanCalculate(&kx); // Calculate the angle using a Kalman filter
	angle[0]=kx.angle;
}
  if (abs(angle[0]) > 90)
    g[1] = -g[1]; // Invert rate, so it fits the restriced accelerometer reading
	ky.newAngle=(pitch);
	kx.newRate=g[1];
	kalmanCalculate(&ky);
	angle[1]=ky.angle;
#else
  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
 if ((pitch < -90 && angle[1] > 90) || (pitch > 90 && angle[1] < -90)) {
    angle[1]=pitch;
        ky.angle=pitch;
  } else{
ky.newAngle=(pitch);
	kx.newRate=g[1];
	kalmanCalculate(&ky);
	angle[1]=ky.angle;
}
  if (abs(angle[1]) > 90)
    g[0] = -g[0]; // Invert rate, so it fits the restriced accelerometer reading
	kx.newAngle=roll;
	kx.newRate=g[0];
    kalmanCalculate(&kx); // Calculate the angle using a Kalman filter
	angle[0]=kx.angle;
#endif
	if(abs(g[2]-gp[2])>(1/5)*ang_vel_s[2]){
	angle[2]=(angle[2]+((g[2]-ang_vel_m[2])*(dt)));
tgp=g[2];
	angle[2]=fmod(angle[2],180);}
return status;

}
float g_1[3],g_2[3];
int get_angle_reg(int *sfd,float *angle,float *g,float *a){    //for simple integration value
		roll  = atan2f(a[1], sqrt(a[0] * a[0] + a[2] * a[2]))* 180 / M_PI;  
	pitch = atan(-a[0] / sqrt(a[1] * a[1] + a[2] * a[2]))* 180 / M_PI;//}
	if(abs(g[0]-gp[0])>ang_vel_s[0])
{	angle[0]=0.98*(angle[0]+((g[0]-ang_vel_m[0])*(dt)))+ 0.02*(roll);
		gp[0]=g[0];
}
	if(abs(g[1]-gp[1])>ang_vel_s[1])
{	angle[1]=0.98*(angle[1]+((g[1]-ang_vel_m[1])*(dt))) + 0.02*(pitch);
	gp[1]=g[1];
}	if(abs(g[2]-gp[2])>ang_vel_s[2])
{	angle[2]=0.98*(angle[2]+((g[2]-ang_vel_m[2])*(dt)));//+ 0.02*(atan2f((float)a[1], (float)a[0]) * 180 / MY_PI);
	gp[2]=g[2];
}	angle[0]=fmod(angle[0],180);
	angle[1]=fmod(angle[1],180);
	angle[2]=fmod(angle[2],180);
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
/*
function sends processed data collected from MPU to the 3D application
*/
int send_to_app(float *g,int sockfd){
int n;
 char buffer[MAXBUF];

    bzero(buffer, MAXBUF);
//	send data to application
	sprintf(buffer,"%2.2f %2.2f %2.2f %2.2f %2.2f %2.2f\n",0.0,0.0,0.0,g[0],g[1],g[2]);  //x y z 
	n=write(sockfd,buffer,strlen(buffer));
	if (n < 0){	
		perror("error :  writing failed for socket");
                exit(1);
	}
//   printf("%s", buffer);
    /*---Clean up---*/
return 0;
}

float ga[3];

void read_th(int *fd,int sock_fd,char func){
float avg[3],ang[3];
int i;
while(1){
	a[0]=0;a[1]=0;a[2]=0;g[0]=0;g[1]=0;g[2]=0;t=0;
	get_mpu_data(fd,g,a);
	g[0]=g[0]-ang_vel_m[0];	
	g[1]=g[1]-ang_vel_m[1];	
	g[2]=g[2]-ang_vel_m[2];
	if(func=='2')
	get_angle_kalman(fd,angle,g,a);
	else
	get_angle_reg(fd,angle,g,a);       // function that calculates angles using gyro values
	ga[0]=M_PI*(angle[0]-prev_angle[0])/(dt*180); //get angular velocity from angle
	ga[1]=M_PI*(angle[1]-prev_angle[1])/(dt*180); //get angular velocity from angle
	ga[2]=M_PI*(angle[2]-prev_angle[2])/(dt*180); //get angular velocity from angle
	float rad_ang[3];
	rad_ang[0]=angle[0]*M_PI/180;	rad_ang[1]=angle[1]*M_PI/180;	rad_ang[2]=angle[2]*M_PI;
	send_to_app(ga,sock_fd);
	prev_angle[0]=angle[0];
	prev_angle[1]=angle[1];
	prev_angle[2]=angle[2];
	}
}

int main(int argc,char *argv[]){
int fd,i;
if(argc !=3){
printf(" command format: main_4 \"<IPADDRESS>\" <function number 2 or 1> 2 for kalman filter result 1 or anything else for integration based\n");
exit(1);
}
char *f_loc="/dev/i2c-0";   //name of i2c dev file
pthread_t t1,t2;
angle[0]=0;
fd=open(f_loc,O_RDWR);
setup(fd);
int sock_fd=make_conn(argv[1]);       //this function call created a socket for tcp transaction.
int gpio_fd=9;
caliberate(&fd);
read_th(&fd,sock_fd,*argv[2]);
close(fd);
    close(sock_fd);
release();
return 0;
}
