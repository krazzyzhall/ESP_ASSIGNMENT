#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
int fd;
int set_spi(){
char data[2];
int speed,bitperword,msb,res,i;
bitperword=8;
speed=10000000;
msb=0;
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
return 0;
}
int send_pat(int fd,char *buf,int count){
int i;
char data[2];
for(i=0;i<count;i++){
data[0]=i+1;
data[1]=buf[i];
write(fd,data,2);}
return 0;
}
/*
get_dist(){
//use gpio15
int gp
fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "42", 3);
	close(fd);
}*/
typedef struct {
char led[8];
}pattern;
int main(){
char p1[8],p2[8];
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
//open SPI
char data[2],in[2];
int res=0,i;
fd=open("/dev/spidev1.0",O_RDWR);
if(fd<0){
printf("Sorry no spi for you :(\n");
return -1;
}
set_spi();
while(1){
send_pat(fd,p1,8);
usleep(250000);
send_pat(fd,p2,8);
usleep(250000);
}
res=read(fd,in,2);
printf("res : %d in: %0x2x, %0x2x \n",res,in[0],in[1]);
getchar();
close(fd);
//write a pattern to SPI
return 0;
}
