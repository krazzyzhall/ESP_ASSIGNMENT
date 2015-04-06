#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

typedef struct {
char led[8];
}pattern;
int main(){
int fd,mux,cs,mosi,miso;
pattern p1,p2;
p1.led[0]=64;
p1.led[1]=65;
p1.led[2]=66;
p1.led[3]=67;
p1.led[4]=68;
p1.led[5]=69;
p1.led[6]=70;
p1.led[7]=71;
p2.led[0]=64;
p2.led[1]=63;
p2.led[2]=62;
p2.led[3]=61;
p2.led[4]=60;
p2.led[5]=59;
p2.led[6]=58;
p2.led[7]=57;
//open SPI
char data[2];
int res=0;
fd=open("/dev/spidev1.0",O_RDWR);
if(fd<0){
printf("Sorry no spi for you :(\n");
return -1;
}
data[0]=0x01;
data[1]=0x0C;
res=write(fd,data,2);
printf("res : %d\n",res);
data[0]=0x00;
data[1]=0x09;
res=write(fd,data,2);
printf("res : %d\n",res);
data[0]=0x0f;
data[1]=0x01;
res=write(fd,data,2);
printf("res : %d\n",res);
data[0]=0x00;
data[1]=0x0F;
res=write(fd,data,2);
while(1){
data[0]=p1.led[0];
data[1]=0x1;
write(fd,data,2);
getchar();
data[0]=data[0]+1;
data[1]=data[1]++;
write(fd,data,2);
getchar();
}
close(fd);
//write a pattern to SPI
return 0;
}
