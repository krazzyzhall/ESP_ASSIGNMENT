#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <syscall.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sched.h>
#include <linux/input.h>

void my_pthread_create(pthread_t *t,void *func(),void *arg); /*
my thread function to register threads.
*/

long cnt;
int scope,no_t;
pthread_t my_id1,my_id2;
cpu_set_t cpuset,cpuset2;
struct sigaction actset;
pthread_mutex_t mut;
static sigset_t   set,set2;
sem_t sem;
void sighandler_threads(int sig);
void sighandler(int sig){
	printf("\ncaught signal in running thread inturrupted by signal %d had output count as  %ld in thread %u \n ",sig,cnt,(unsigned int)syscall(SYS_gettid));
}
void recv_thread(){   //thread which will catch the dummy SIGUSR1 signal instead of SIGIO
	sigaddset(&set2, SIGUSR1);
	signal(SIGUSR1,sighandler); 				// Registering signal handler
	pthread_sigmask(SIG_UNBLOCK, &set2, NULL); 		  	// Masking signals
while(1){
int r=rand()%50;
sleep((r+1)*1000000);
}
//will display who received 

}
pthread_t t1,t2,t3,t4,t5;
pthread_attr_t attr;
pthread_t *pth[10];
int *policy;
void my_signal_pthread(__sighandler_t func){
	sigaddset(&set2, SIGUSR1);
	signal(SIGUSR1,func); 				// Registering signal handler
}
void my_pthread_create(pthread_t *t,void *func(),void *arg){
static int curr=0;
pth[curr++]=t;
pthread_create(t,NULL,func,arg);
no_t=curr;
}
void sighandler_threads(int sig){
int i;
	for(i=0;i<no_t;i++){
	pthread_kill(*pth[i],SIGUSR1);
}
}
int main(int argc, char **argv){
int sltime,fdmouse;
struct input_event ls;
sigemptyset(&set);
sigemptyset(&set2);
	
        sigaddset(&set, SIGIO);
	signal(SIGIO,sighandler_threads); 				// Registering signal handler
	my_signal_pthread(sighandler);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL); 		  	// Masking signals
my_pthread_create(&t1,(void*)&recv_thread,NULL);
my_pthread_create(&t2,(void*)&recv_thread,NULL);
my_pthread_create(&t3,(void*)&recv_thread,NULL);
	fdmouse = open("/dev/input/event17",O_RDWR|O_NONBLOCK|O_NOCTTY);
fcntl(fdmouse,F_SETOWN,getpid());
fcntl(fdmouse, F_SETFL, FASYNC); 
while(1)
{
	read(fdmouse,&ls,sizeof(struct input_event));
	if(ls.code == 272){
//		printf("this is the click of left \n");	//		longjmp(env,1);
	}
	else if(ls.code == 273){
//		printf("this is the click of right \n");	//		longjmp(env,1);
	}
}
return 0;
}
