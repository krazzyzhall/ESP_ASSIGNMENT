#include "Test_app.h"
int *seq;
pattern;
pthread_mutex_t lock;
int fd_spi,fd_usonic;
int main(){
fd_usonic=open("/dev/pulse_irq",O_RDWR);
//create  spi write thread and create sequence set thread.
//
close(fd_usonic);
close(fd_spi);
return 0;
}
