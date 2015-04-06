#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "led_mat.h"

int fd;
static int kill; 
char pat1[10][8];
struct my_seq_pat *myseq;
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
pattern test_seq[10];
test_seq[0].led[0]=0xFF;test_seq[0].led[1]=0x3 ;test_seq[0].led[2]=0x6 ;test_seq[0].led[3]=0xC ;
test_seq[0].led[4]=0x18;test_seq[0].led[5]=0x30;test_seq[0].led[6]=0x60;test_seq[0].led[7]=0xFF;
test_seq[1].led[0]=0xFC;test_seq[1].led[1]=0xFE;test_seq[1].led[2]=0x13;test_seq[1].led[3]=0x11;
test_seq[1].led[4]=0x11;test_seq[1].led[5]=0x13;test_seq[1].led[6]=0xFE;test_seq[1].led[7]=0xFC;
test_seq[2].led[0]=0xFF;test_seq[2].led[1]=0x3 ;test_seq[2].led[2]=0x6 ;test_seq[2].led[3]=0xC ;
test_seq[2].led[4]=0x18;test_seq[2].led[5]=0x30;test_seq[2].led[6]=0x60;test_seq[2].led[7]=0xFF;
test_seq[3].led[0]=0xFC;test_seq[3].led[1]=0xFE;test_seq[3].led[2]=0x13;test_seq[3].led[3]=0x11;
test_seq[3].led[4]=0x11;test_seq[3].led[5]=0x13;test_seq[3].led[6]=0xFE;test_seq[3].led[7]=0xFC;
test_seq[4].led[0]=0xFF;test_seq[4].led[1]=0x3 ;test_seq[4].led[2]=0x6 ;test_seq[4].led[3]=0xC ;
test_seq[4].led[4]=0x18;test_seq[4].led[5]=0x30;test_seq[4].led[6]=0x60;test_seq[4].led[7]=0xFF;
test_seq[5].led[0]=0xFC;test_seq[5].led[1]=0xFE;test_seq[5].led[2]=0x13;test_seq[5].led[3]=0x11;
test_seq[5].led[4]=0x11;test_seq[5].led[5]=0x13;test_seq[5].led[6]=0xFE;test_seq[5].led[7]=0xFC;
test_seq[6].led[0]=0xFF;test_seq[6].led[1]=0x3 ;test_seq[6].led[2]=0x6 ;test_seq[6].led[3]=0xC ;
test_seq[6].led[4]=0x18;test_seq[6].led[5]=0x30;test_seq[6].led[6]=0x60;test_seq[6].led[7]=0xFF;
test_seq[7].led[0]=0xFC;test_seq[7].led[1]=0xFE;test_seq[7].led[2]=0x13;test_seq[7].led[3]=0x11;
test_seq[7].led[4]=0x11;test_seq[7].led[5]=0x13;test_seq[7].led[6]=0xFE;test_seq[7].led[7]=0xFC;
test_seq[8].led[0]=0x00;test_seq[8].led[1]=0x00;test_seq[8].led[2]=0x00;test_seq[8].led[3]=0x00;
test_seq[8].led[4]=0x00;test_seq[8].led[5]=0x00;test_seq[8].led[6]=0x00;test_seq[8].led[7]=0x00;
test_seq[9].led[0]=0xFF;test_seq[9].led[1]=0x1 ;test_seq[9].led[2]=0xCB;test_seq[9].led[3]=0xE3;
test_seq[9].led[4]=0xE3;test_seq[9].led[5]=0xCB;test_seq[9].led[6]=0x01;test_seq[9].led[7]=0xFF;
/*test_seq[10].led[0]=0x00;test_seq[10].led[1]=0x00;test_seq[10].led[2]=0x00;test_seq[10].led[3]=0x00;
test_seq[10].led[4]=0x00;test_seq[10].led[5]=0x00;test_seq[10].led[6]=0x00;test_seq[10].led[7]=0x00;
test_seq[11].led[0]=0xFF;test_seq[11].led[1]=0x1 ;test_seq[11].led[2]=0xCB;test_seq[11].led[3]=0xE3;
test_seq[11].led[4]=0xE3;test_seq[11].led[5]=0xCB;test_seq[11].led[6]=0x01;test_seq[11].led[7]=0xFF;*/
myseq=malloc(sizeof(struct my_seq_pat));
myseq->seq=malloc(6*sizeof(int));
myseq->lseq=6;
myseq->seq[0]=0;myseq->seq[1]=250;myseq->seq[2]=1;myseq->seq[3]=250;myseq->seq[4]=0;myseq->seq[5]=0;
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
printf("my_spi_dev setup done \n");
ioctl(fd,ISPATTERN,(unsigned long)1);   // is pattern
res=write(fd,test_seq,80);
ioctl(fd,SEQ_ENABLE,(unsigned long)1);   ///to enable sequence mode
ioctl(fd,INS_SEQ,myseq);   //to put in sequence and create a thread that will run a thread
//res=write(fd,pat1[2],8);
//send_pat(fd,pat1[9],8);
//usleep(250000);
//send_pat(fd,pat1[9],8);
usleep(10000000);
myseq->seq[0]=9;myseq->seq[1]=250;myseq->seq[2]=0;myseq->seq[3]=250;myseq->seq[4]=0;myseq->seq[5]=0;
ioctl(fd,INS_SEQ,myseq);   //to put in sequence and create a thread that will run a thread
usleep(5000000);
ioctl(fd,KILL_THREAD);
//res=read(fd,in,2);
printf("res : %d \n",res);
close(fd);
getchar();
//write a pattern to SPI
return 0;
}
