#include <stdio.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>   //for usleep function
void myopen(void){
 int fd=open("/dev/i2c-flash",O_RDWR);
 printf("%d\n",fd);
}
int main(){
pthread_t t1,t2,t3;
pthread_create( &t1, NULL, (void*)myopen,NULL);
pthread_create( &t2, NULL, (void*)myopen,NULL);
pthread_create( &t3, NULL, (void*)myopen,NULL);

pthread_join(t3,NULL);
pthread_join(t2,NULL);
pthread_join(t1,NULL);
return 0;
}
