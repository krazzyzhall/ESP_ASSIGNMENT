#include "Test_app.h"

pthread_mutex_t lock;
int fd_spi,fd_usonic;
int main(){
	pattern test_seq[10];
	test_seq[0].led[0]=0xFF;test_seq[0].led[1]=0x81;test_seq[0].led[2]=0xBD;test_seq[0].led[3]=0xA5;
	test_seq[0].led[4]=0xA5;test_seq[0].led[5]=0xBD;test_seq[0].led[6]=0x81;test_seq[0].led[7]=0xFF;
	test_seq[1].led[0]=0x00;test_seq[1].led[1]=0x00;test_seq[1].led[2]=0x00;test_seq[1].led[3]=0x00;
	test_seq[1].led[4]=0x00;test_seq[1].led[5]=0x00;test_seq[1].led[6]=0x00;test_seq[1].led[7]=0x00;
	test_seq[2].led[0]=0x00;test_seq[2].led[1]=0x7E;test_seq[2].led[2]=0x42;test_seq[2].led[3]=0x5A;
	test_seq[2].led[4]=0x5A;test_seq[2].led[5]=0x42;test_seq[2].led[6]=0x7E;test_seq[2].led[7]=0x00;
	test_seq[3].led[0]=0x10;test_seq[3].led[1]=0x38;test_seq[3].led[2]=0x38;test_seq[3].led[3]=0x38;
	test_seq[3].led[4]=0x38;test_seq[3].led[5]=0x38;test_seq[3].led[6]=0x7C;test_seq[3].led[7]=0xFE;
	test_seq[4].led[0]=0x7F;test_seq[4].led[1]=0x3E;test_seq[4].led[2]=0x1C;test_seq[4].led[3]=0x1C;
	test_seq[4].led[4]=0x1C;test_seq[4].led[5]=0x1C;test_seq[4].led[6]=0x1C;test_seq[4].led[7]=0x08;
	test_seq[5].led[0]=0x08;test_seq[5].led[1]=0x38;test_seq[5].led[2]=0x78;test_seq[5].led[3]=0xFC;
	test_seq[5].led[4]=0x3E;test_seq[5].led[5]=0x1F;test_seq[5].led[6]=0x0F;test_seq[5].led[7]=0x07;
	test_seq[6].led[0]=0x07;test_seq[6].led[1]=0x0F;test_seq[6].led[2]=0x1F;test_seq[6].led[3]=0x3E;
	test_seq[6].led[4]=0xFC;test_seq[6].led[5]=0x78;test_seq[6].led[6]=0x38;test_seq[6].led[7]=0x08;
	test_seq[7].led[0]=0xFC;test_seq[7].led[1]=0xFE;test_seq[7].led[2]=0x13;test_seq[7].led[3]=0x11;
	test_seq[7].led[4]=0x11;test_seq[7].led[5]=0x13;test_seq[7].led[6]=0xFE;test_seq[7].led[7]=0xFC;
	test_seq[8].led[0]=0x00;test_seq[8].led[1]=0x00;test_seq[8].led[2]=0x00;test_seq[8].led[3]=0x00;
	test_seq[8].led[4]=0x00;test_seq[8].led[5]=0x00;test_seq[8].led[6]=0x00;test_seq[8].led[7]=0x00;
	test_seq[9].led[0]=0xFF;test_seq[9].led[1]=0x01;test_seq[9].led[2]=0xCB;test_seq[9].led[3]=0xE3;
	test_seq[9].led[4]=0xE3;test_seq[9].led[5]=0xCB;test_seq[9].led[6]=0x01;test_seq[9].led[7]=0xFF;

Dist=21;
pthread_t t1,t2;
fd_usonic=open("/dev/pulse_irq",O_RDWR);
fd_spi=open("/dev/myspidev1.0",O_RDWR);
set_spi(fd_spi);
send_pat_set(fd_spi,test_seq);
usleep(1000);
printf(" %d   %d ",fd_spi,fd_usonic);
//motionpicture(fd_spi);
//create  spi write thread and create sequence set thread.
getchar();
pthread_create(&t1,NULL,motionpicture,&fd_spi);
pthread_create(&t2,NULL,(void *)dist_thread,&fd_usonic);
usleep(5000000);
ioctl(fd_spi,KILL_THREAD);
pthread_cancel(t1);
pthread_cancel(t2);
getchar();
close(fd_usonic);
close(fd_spi);
return 0;
}
