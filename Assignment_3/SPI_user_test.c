#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#define ISPATTERN      101
#define SEQ_ENABLE     103
#define KILL_THREAD    104
#define INS_SEQ        109

int fd;
static int kill; 
char pat1[10][8];
struct my_seq_pat{
int *seq;
int lseq;
} *myseq;
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
ioctl(fd,ISPATTERN,(unsigned long)0);
ioctl(fd,SEQ_ENABLE,(unsigned long)0);
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
printf("at the end of SET_SPI");
return 0;
}
int send_pat(int fd,char *buf,int count){
int i;
char data[2];
for(i=0;i<count;i++){
data[0]=i+1;
data[1]=buf[i];//0xff;
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
pat1[1][0]='a';
pat1[1][1]='a';
pat1[1][2]='a';
pat1[1][3]='a';
pat1[1][4]='a';
pat1[1][5]='a';
pat1[1][6]='a';
pat1[1][7]='a';
pat1[9][0]=0x00;
pat1[9][1]=0x00;
pat1[9][2]=0x02;
pat1[9][3]=0x03;
pat1[9][4]=0x7F;
pat1[9][5]=0xC2;
pat1[9][6]=0x32;
pat1[9][7]=0x16;
pat1[3][0]='1';pat1[3][1]='2';pat1[3][1]='3';pat1[3][1]='4';pat1[3][1]='5';pat1[3][1]='6';pat1[3][1]='7';pat1[3][1]='8';//0x00,0x00,0x00,0x00,0x00,0x00,0x00;
pat1[4][0]='1';pat1[4][1]='2';pat1[4][1]='4';pat1[4][1]='4';pat1[4][1]='5';pat1[4][1]='6';pat1[4][1]='7';pat1[4][1]='8';//0x00,0x00,0x00,0x00,0x00,0x00,0x00;
pat1[5][0]='1';pat1[5][1]='2';pat1[5][1]='5';pat1[5][1]='4';pat1[5][1]='5';pat1[5][1]='6';pat1[5][1]='7';pat1[5][1]='8';//0x00,0x00,0x00,0x00,0x00,0x00,0x00;
pat1[6][0]='1';pat1[6][1]='2';pat1[6][1]='6';pat1[6][1]='4';pat1[6][1]='5';pat1[6][1]='6';pat1[6][1]='7';pat1[6][1]='8';//0x00,0x00,0x00,0x00,0x00,0x00,0x00;
pat1[7][0]='1';pat1[7][1]='2';pat1[7][1]='7';pat1[7][1]='4';pat1[7][1]='5';pat1[7][1]='6';pat1[7][1]='7';pat1[7][1]='8';//0x00,0x00,0x00,0x00,0x00,0x00,0x00;
pat1[8][0]='1';pat1[8][1]='2';pat1[8][1]='8';pat1[8][1]='4';pat1[8][1]='5';pat1[8][1]='6';pat1[8][1]='7';pat1[3][1]='8';//0x00,0x00,0x00,0x00,0x00,0x00,0x00;
pat1[2][0]='a';
pat1[2][1]='a';
pat1[2][2]='a';
pat1[2][3]='a';
pat1[2][4]='a';
pat1[2][5]='a';
pat1[2][6]='a';
pat1[2][7]='a';
myseq=malloc(sizeof(struct my_seq_pat));
myseq->seq=malloc(6*sizeof(int));
myseq->lseq=6;
myseq->seq[0]=9;myseq->seq[1]=250;myseq->seq[2]=2;myseq->seq[3]=250;myseq->seq[4]=0;myseq->seq[5]=0;
//open SPI
char data[2],in[2];
int res=0,i;
fd=open("/dev/myspidev1.0",O_RDWR);
//fd=open("/dev/spidev1.0",O_RDWR);
if(fd<0){
printf("Sorry no spi for you :(\n");
return -1;
}
set_spi();
ioctl(fd,ISPATTERN,(unsigned long)1);   // is pattern
res=write(fd,pat1,80);
ioctl(fd,SEQ_ENABLE,(unsigned long)1);   ///to enable sequence mode
ioctl(fd,INS_SEQ,myseq);   //to put in sequence and create a thread that will run a thread
//res=write(fd,pat1[2],8);
//send_pat(fd,pat1[9],8);
//usleep(250000);
//send_pat(fd,pat1[9],8);
//usleep(250000);
usleep(5000000);
ioctl(fd,KILL_THREAD);
//res=read(fd,in,2);
printf("res : %d \n",res);
close(fd);
getchar();
//write a pattern to SPI
return 0;
}
