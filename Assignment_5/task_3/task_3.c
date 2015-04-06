#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sched.h>
#include <linux/input.h>
long cnt;
int scope;
pthread_t my_id1,my_id2;
cpu_set_t cpuset,cpuset2;
struct sigaction actset;
pthread_mutex_t mut;
static sigset_t   set;
sem_t sem;
void sighandler(int sig){
	printf("\ncaught signal in running thread inturrupted by signal %d had output count as  %ld in thread %ld \n ",sig,cnt,pthread_self());
//	exit(1);
}
void recv_thread(){
	pthread_sigmask(SIG_UNBLOCK, &set, NULL); 		  	// Masking signals
while(1){
printf("in thread No. %ld and waiting for a  signal\n",pthread_self());
sem_wait(&sem);
int r=rand()%50;
usleep((r+1)*1000);
printf("thread : %ld caught the signal\n",pthread_self());
}
//will display who received 

}
pthread_t t1,t2,t3,t4,t5;
pthread_attr_t attr;
//struct siched_param *param,parm;
int *policy;
int main(int argc, char **argv){
int sltime,fdmouse;
struct input_event ls;
sigemptyset(&set);
        sigaddset(&set, SIGIO);
	signal(SIGIO,sighandler); 				// Registering signal handler
//	sigaction() 
	pthread_sigmask(SIG_BLOCK, &set, NULL); 		  	// Masking signals
pthread_create(&t1,NULL,(void*)&recv_thread,NULL);
pthread_create(&t2,NULL,(void*)&recv_thread,NULL);
pthread_create(&t3,NULL,(void*)&recv_thread,NULL);
	fdmouse = open("/dev/input/event17",O_RDWR|O_NONBLOCK|O_NOCTTY);
fcntl(fdmouse,F_SETOWN,getpid());
fcntl(fdmouse, F_SETFL, FASYNC); 
while(1)
{
	read(fdmouse,&ls,sizeof(struct input_event));
	if(ls.code == 272){
		printf("this is the click of left \n");	//		longjmp(env,1);
	}
	else if(ls.code == 273){
		printf("this is the click of right \n");	//		longjmp(env,1);
	}
}
return 0;
}
