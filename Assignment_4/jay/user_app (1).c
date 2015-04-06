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
#include <linux/spi/spidev.h>
#include <poll.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <math.h>

#define Sensor_Address 	0x68
#define Power_Mng1	0x6B
#define Power_Mng2	0x6C
#define Sample_Rate	0x19
#define Interrupt_Set	0x38

/* Gyro Axis Data */
#define GyroX1		0x43 	// X- axis MSB
#define GyroX2		0x44 	// X- axis LSB
#define GyroY1		0x45	// Y- axis MSB
#define GyroY2		0x46	// Y- axis LSB
#define GyroZ1		0x47	// Z- axis MSB
#define GyroZ2		0x48	// Z- axis LSB

/* Accelerometer Axis Data */
#define AcceX1		0x3B	// X- axis MSB
#define AcceX2		0x3C	// X- axis LSB
#define AcceY1		0x3D	// Y- axis MSB
#define AcceY2		0x3E	// Y- axis LSB
#define AcceZ1		0x3F	// Z- axis MSB
#define AcceZ2		0x40	// Z- axis LSB

#define g_value 	16384
#define d_value		32768

#define CPU_CLock	400000000

/* Time Stamp Counter function */
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

int Register_Read(int fd, uint8_t Address);			// Reads Register from given address
int Register_Write(int fd, uint8_t Address, uint8_t Data);	// Writes Data to Given Register Address
int Set_Up(int fd_i2c);						// Sets Up GPIOs
int Read_MP650(int fd_i2c, float *Gyro_Array, float *Acce_Array);
int Calibration(int fd_i2c);					//
int Poll_Fuction(int fd_i2c);						//Waitsfor an event i.e. update of data in register

float Standard_DeviationX = 0;
float Standard_DeviationY = 0;
float Standard_DeviationZ = 0;
float Standard_DeviationAX = 0;
float Standard_DeviationAY = 0;
float Standard_DeviationAZ = 0;
float GyroRefXm  = 0;
float GyroRefYm  = 0;
float GyroRefZm  = 0;
float AcceRefXm  = 0;
float AcceRefYm  = 0;
float AcceRefZm  = 0;

uint64_t   	Time_Stamp_curr;
uint64_t   	Time_Stamp_prev;
float   	Total_Time;

float reference_angleX;
float reference_angleY;
float reference_angleZ;

int main()
{
	int status;
	int fd_i2c;
	float Gyro_Array[3];
	float Gyro_New[3];
	float Acce_Array[3];
	float Acce_New[3];

	float AngleX = 0;
	float SumX   = 0;
	float AngleY = 0;
	float SumY   = 0;
	float AngleZ = 0;
	float SumZ   = 0;

	fd_i2c = open("/dev/i2c-0",O_RDWR);
	printf("%d\n",fd_i2c);

	if(status < 0){
		printf("failed to Write Slave Address\n");
	}

	Set_Up(fd_i2c);
	
	/* Releasing Sensor from sleep mode */
	Register_Write(fd_i2c,Power_Mng1,0x00);
	Register_Write(fd_i2c,Power_Mng2,0x00);

	/* Setting Sampling Rate */
	Register_Write(fd_i2c,Sample_Rate,39);

	Register_Write(fd_i2c,0x37,0x60);
	Register_Write(fd_i2c,Interrupt_Set,0x01);


	/* Calibration */
	Calibration(fd_i2c);

	Gyro_New[0]   = 0; 
	Gyro_New[1]   = 0;
	Gyro_New[2]   = 0;
	Acce_New[0]   = 0; 
	Acce_New[1]   = 0;
	Acce_New[2]   = 0;
	Time_Stamp_prev = 0;

	while(1)
	{	
		
		Poll_Fuction(fd_i2c);

		Read_MP650(fd_i2c, Gyro_Array, Acce_Array);

		Gyro_Array[0] = Gyro_Array[0] - GyroRefXm;
		Gyro_Array[1] = Gyro_Array[1] - GyroRefYm;
		Gyro_Array[2] = Gyro_Array[2] - GyroRefZm;

		if(abs(Gyro_Array[2]) > Standard_DeviationZ){
			Gyro_New[2] = Gyro_Array[2];
		}
		else{
			Gyro_New[2] = 0;
		}

		if(abs(Gyro_Array[1]) > Standard_DeviationY){
			Gyro_New[1] = Gyro_Array[1];
		}
		else{
			Gyro_New[1] = 0;
		}

		if(abs(Gyro_Array[0]) > Standard_DeviationX){
			Gyro_New[0] = Gyro_Array[0];
		}
		else{
			Gyro_New[0] = 0;
		}

		SumX = SumX + Gyro_New[0];
		SumY = SumY + Gyro_New[1];
		SumZ = SumZ + Gyro_New[2];

		AngleX = SumX/200;
		AngleY = SumY/200;
		AngleZ = SumZ/200;

		
		//Acce_Array[0] =  AcceRefXm*cos(reference_angleX-AngleY);
		//Acce_Array[1] =  AcceRefYm*cos(reference_angleY-AngleZ);
		//Acce_Array[2] =  AcceRefZm*cos(reference_angleZ-AngleX);
	
		//printf("AngleX- %f \n", AngleX);
		//printf("AngleY- %f \n", AngleY);
		//printf("AngleZ- %f \n", AngleZ);
		//printf("Gyro Z - %f Gyro_New Z- %f Angle- %f \n", Gyro_Array[2], Gyro_New[2], AngleZ);//Gyro_New[2]
		//printf("AngleX- %f AngleY- %f AngleZ- %f \n",AngleX, AngleY, AngleZ);
		printf("Acee X- %f Y- %f Z- %f \n",Acce_Array[0], Acce_Array[1], Acce_Array[2]);
	}
	close(fd_i2c);
	return 0;
	
}

int Poll_Fuction(int fd_i2c){

	int ret;
	int fdedge;
	struct pollfd fds;
	int dummy_read;
	int fdvalue;
	char *buf;
	char echo;	
	while(1){
		fdvalue = open("/sys/class/gpio/gpio15/value",O_RDWR);
		read(fdvalue, &echo, 1);
		close(fdvalue);
		//printf("%c\n",echo);
		if(echo == '1' ){
			//printf("Posedge Received\n");
			/*
			Time_Stamp_curr = rdtsc();
			Total_Time 	= ((float)(Time_Stamp_curr - Time_Stamp_prev))/CPU_CLock;
			Time_Stamp_prev = Time_Stamp_curr;
			printf("%f\n",Total_Time);
			*/
			Register_Read(fd_i2c,0x3A);
			break;
		}
	}
/*
		fdedge = open("/sys/class/gpio/gpio15/edge", O_WRONLY);
		write(fdedge, "none", 5);
		close(fdedge);
		fdvalue = open("/sys/class/gpio/gpio15/value", O_WRONLY);

		fds.fd 		= fdvalue;
		fds.events 	= POLLPRI;
		fds.revents 	= 0;

		ret = poll(&fds, 1, 1000);
		dummy_read = read(fdvalue,buf,1);
		if (ret >= 0) {
			if (fds.revents & POLLPRI) {
				Time_Stamp_curr = rdtsc();
				Total_Time 	= ((float)(Time_Stamp_curr - Time_Stamp_prev))/CPU_CLock;
				Time_Stamp_prev = Time_Stamp_curr;
				printf("%f\n",Total_Time);
				break;	
			}
		}else{
			printf("Poll Failed\n");
		}
		close(fdvalue);
*/
}

/* Read Gyroscope values */
int Read_MP650(int fd_i2c, float *Gyro_Array, float *Acce_Array)
{
	uint16_t fin_val;
	int GyroX;
	int GyroY;
	int GyroZ;
	uint8_t reg_out[14];
	uint8_t add;
	int status;
	int AcceX;
	int AcceY;
	int AcceZ;
		
	add = AcceX1;

	status = write(fd_i2c, &add, 1);
	if(status < 0 ){
		printf("Address Write Failed\n");
	}

	status = read (fd_i2c, reg_out, 14);
	if(status < 0 ){
		printf("Data Read Failed\n");
	}

	/* Reading Accelometer registers */
	fin_val = reg_out[0];
	fin_val = (fin_val << 8) + reg_out[1];
	AcceX = (short int)fin_val;
	*Acce_Array = ((float)AcceX)/g_value;

	fin_val = reg_out[2];
	fin_val = (fin_val << 8) + reg_out[3];
	AcceY = (short int)fin_val;
	*(Acce_Array+1) = ((float)AcceY)/g_value;

	fin_val = reg_out[4];
	fin_val = (fin_val << 8) + reg_out[5];
	AcceZ = (short int)fin_val;
	*(Acce_Array+2) = ((float)AcceZ)/g_value;

	/* Reading Gyroscope registers */
	fin_val = reg_out[8];
	fin_val = (fin_val << 8) + reg_out[9];
	GyroX = (short int)fin_val;

	*Gyro_Array = (((float)GyroX)*250/d_value) ;
	fin_val = 0;

	fin_val = reg_out[10];
	fin_val = (fin_val << 8) + reg_out[11];
	GyroY = (short int)fin_val;
	*(Gyro_Array+1) = (((float)GyroY)*250/d_value);

	fin_val = reg_out[12];
	fin_val = (fin_val << 8) + reg_out[13];
	GyroZ = (short int)fin_val;
	*(Gyro_Array+2) = (((float)GyroZ)*250/d_value);
}

/* Calibration */
int Calibration(int fd_i2c)
{
	int i;
	float GyroRefX[200];
	float GyroRefY[200];
	float GyroRefZ[200];

	float AcceRefX[200];
	float AcceRefY[200];
	float AcceRefZ[200];

	float Gyro_Array[3];
	float Acce_Array[3];

	printf("Calibrating...\n");

	sleep(1);

	/* Storing Values in buffer as well as calculating mean*/
	for(i = 0; i < 200; i++){
		Poll_Fuction(fd_i2c);
		Read_MP650(fd_i2c,Gyro_Array,Acce_Array);
		GyroRefXm   = (*Gyro_Array + i*GyroRefXm)/(i+1);
		GyroRefX[i] = *Gyro_Array;

		GyroRefYm = (*(Gyro_Array+1) + i*GyroRefYm)/(i+1);
		GyroRefY[i] = *(Gyro_Array + 1);
		
		GyroRefZm = (*(Gyro_Array+2) + i*GyroRefZm)/(i+1);
		GyroRefZ[i] = *(Gyro_Array + 2);	

		AcceRefXm = (*(Acce_Array) + i*AcceRefXm)/(i+1);
		AcceRefX[i] = *(Acce_Array );

		AcceRefYm = (*(Acce_Array+1) + i*AcceRefYm)/(i+1);
		AcceRefY[i] = *(Acce_Array + 1);

		AcceRefZm = (*(Acce_Array+2) + i*AcceRefZm)/(i+1);
		AcceRefZ[i] = *(Acce_Array + 2);
	}

	/* Calculating Standard Deviation */
	Standard_DeviationX = 0;
	Standard_DeviationY = 0;
	Standard_DeviationZ = 0;
	
	Standard_DeviationAX = 0;
	Standard_DeviationAY = 0;
	Standard_DeviationAZ = 0;

	for(i = 0; i < 200 ; i++){
		Standard_DeviationX = (((GyroRefX[i] - GyroRefXm)*(GyroRefX[i] - GyroRefXm))  + i*Standard_DeviationX)/(i+1);
		Standard_DeviationY = (((GyroRefY[i] - GyroRefYm)*(GyroRefY[i] - GyroRefYm))  + i*Standard_DeviationY)/(i+1);
		Standard_DeviationZ = (((GyroRefZ[i] - GyroRefZm)*(GyroRefZ[i] - GyroRefZm))  + i*Standard_DeviationZ)/(i+1);

		Standard_DeviationAX = (((AcceRefX[i] - AcceRefXm)*(AcceRefX[i] - AcceRefXm))  + i*Standard_DeviationAX)/(i+1);
		Standard_DeviationAY = (((AcceRefY[i] - AcceRefYm)*(AcceRefY[i] - AcceRefYm))  + i*Standard_DeviationAY)/(i+1);
		Standard_DeviationAZ = (((AcceRefZ[i] - AcceRefZm)*(AcceRefZ[i] - AcceRefZm))  + i*Standard_DeviationAZ)/(i+1);
	}
	
	Standard_DeviationX =  pow(Standard_DeviationX, 0.5);
	Standard_DeviationY =  pow(Standard_DeviationY, 0.5);
	Standard_DeviationZ =  pow(Standard_DeviationZ, 0.5);
	Standard_DeviationAX = pow(Standard_DeviationAX, 0.5);
	Standard_DeviationAY = pow(Standard_DeviationAY, 0.5);
	Standard_DeviationAZ = pow(Standard_DeviationAZ, 0.5);

	printf("Gyro-Ref X- %f Y- %f Z- %f \n",GyroRefXm, GyroRefYm, GyroRefZm);

	printf("Standard DeviationX : %f\n",Standard_DeviationX);
	printf("Standard DeviationY : %f\n",Standard_DeviationY);
	printf("Standard DeviationZ : %f\n",Standard_DeviationZ);

	printf("Standard DeviationAX : %f\n",Standard_DeviationAX);
	printf("Standard DeviationAY : %f\n",Standard_DeviationAY);
	printf("Standard DeviationAZ : %f\n",Standard_DeviationAZ);

	if(AcceRefXm > 1){
		AcceRefXm = 1;
	}

	if(AcceRefYm > 1){
		AcceRefYm = 1;
	}

	if(AcceRefZm > 1){
		AcceRefZm = 1;
	}

	if(AcceRefXm < -1){
		AcceRefXm = -1;
	}

	if(AcceRefYm < -1){
		AcceRefYm = -1;
	}

	if(AcceRefZm < -1){
		AcceRefZm = -1;
	}


	printf("Acce-Ref X- %f Y- %f Z- %f \n",AcceRefXm, AcceRefYm, AcceRefZm);

	reference_angleX = acosf(AcceRefXm)*180/3.14;
	reference_angleY = acosf(AcceRefYm)*180/3.14;
	reference_angleZ = acosf(AcceRefZm)*180/3.14;

	printf("Initial Orientation X : %f\n",reference_angleX);
	printf("Initial Orientation Y : %f\n",reference_angleY);
	printf("Initial Orientation Z : %f\n",reference_angleZ);
	
}

/* Function to write single register */
int Register_Write(int fd, uint8_t Address, uint8_t Data){
	int status; 
	uint8_t Data_Write[2];
	
	Data_Write[0] = Address;
	Data_Write[1] = Data;
	status = write(fd, Data_Write, 2);

	if(status < 0 ){
		printf("Address Write Failed\n");
	}
}

/* Function to Read single register */
int Register_Read(int fd, uint8_t Address){
	uint8_t reg_out;
	int status; 
	uint8_t add;
	
	add = Address;
	status = write(fd, &add, 1);
	if(status < 0 ){
		printf("Address Write Failed\n");
	}

	status = read (fd, &reg_out, 1);
	if(status < 0 ){
		printf("Data Read Failed\n");
	}

	return reg_out;
}

/* Setting Up GPIOs */
int Set_Up(int fd_i2c){
	int fd;
	int gpio;
	int status;

	/* Writing Slave Address */
	status = ioctl(fd_i2c, I2C_SLAVE, Sensor_Address);

	/* Setting GPIO 29 */	
	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, "29", 3);
	close(fd);

	fd = open("/sys/class/gpio/gpio29/direction",O_RDWR);
	write(fd, "out", 4); 
	close(fd);

	fd = open("/sys/class/gpio/gpio29/value",O_RDWR);
	write(fd, "0", 2); 

	/* Setting GPIO 15 */ //Interrupt
	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, "15", 3);
	close(fd);

	fd = open("/sys/class/gpio/gpio15/direction",O_RDWR);
	write(fd, "in", 3); 
	
	/* Setting GPIO 30 */ 
	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, "30", 3);
	close(fd);

	fd = open("/sys/class/gpio/gpio30/direction",O_RDWR);
	write(fd, "out", 4); 
	close(fd);

	fd = open("/sys/class/gpio/gpio30/value",O_RDWR);
	write(fd, "0", 2); 
	close(fd);

	close(fd);
	
}
