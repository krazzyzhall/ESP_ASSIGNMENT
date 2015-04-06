#include "Application.h"

int main(){
sequence test_seq[12];

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
test_seq[10].led[0]=0x00;test_seq[10].led[1]=0x00;test_seq[10].led[2]=0x00;test_seq[10].led[3]=0x00;
test_seq[10].led[4]=0x00;test_seq[10].led[5]=0x00;test_seq[10].led[6]=0x00;test_seq[10].led[7]=0x00;
test_seq[11].led[0]=0xFF;test_seq[11].led[1]=0x1 ;test_seq[11].led[2]=0xCB;test_seq[11].led[3]=0xE3;
test_seq[11].led[4]=0xE3;test_seq[11].led[5]=0xCB;test_seq[11].led[6]=0x01;test_seq[11].led[7]=0xFF;
test_seq[0].stime=300;test_seq[1].stime=300;test_seq[2].stime=300;test_seq[3].stime=300;test_seq[4].stime=300;
test_seq[5].stime=300;test_seq[6].stime=300;test_seq[7].stime=300;test_seq[8].stime=300;test_seq[9].stime=300;
test_seq[10].stime=300;test_seq[11].stime=300;

char *spi_loc="/dev/spidev1.0";

pthread_t t1,t3,t2;
set_spi(spi_loc);   //set spi param 
set_gpio_usonic();
struct myparam *par = (struct myparam *)malloc(sizeof(struct myparam));
par->floc=spi_loc;par->buf=&test_seq;
par->count=12;
Dist=1;
printf("creating thread \n");
pthread_create( &t3, NULL, (void*)read_val,&fd);
pthread_create( &t2, NULL, (void*)itr_seq,par);
usleep(10000000);
usleep(10000000);
pthread_cancel(t2);
pthread_cancel(t3);
return 0;
}
