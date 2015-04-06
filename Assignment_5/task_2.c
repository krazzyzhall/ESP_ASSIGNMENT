#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <sched.h>
long cnt;
int scope;
pthread_t my_id1,my_id2;
cpu_set_t cpuset,cpuset2;
struct sigaction act_open,act_open2;
pthread_mutex_t mut;
static sigset_t  set;
sem_t sem;
void sighandler_1(int sig){
	printf("\ncaught signal in running thread inturrupted by signal %d had output count as  %ld in thread %ld \n ",sig,cnt,my_id1);
//	exit(1);
}
void sighandler_2(int sig){
	printf("\ncaught signal in high priority thread inturrupted by signal %d had output count as  %ld in thread %ld\n",sig,cnt,my_id1);
//	exit(1);
}
void sighandler_3(int sig){
	printf("\ncaught signal in thread waiting for semaphore,inturrupted by signal %d had output count as  %ld\n",sig,my_id1);
if(sig==SIGINT){
	printf("this is the end \n");
	exit(1);}
}
void sighandler_4(int sig){
	printf("\ncaught signal in high priority thread with delay,inturrupted by signal %d had output count as  %ld\n",sig,pthread_self());
//	exit(1);
}
void cont_thread(){
//	setupt signal to be captured ;
	signal(SIGUSR1,sighandler_1);
//act_open.sa_handler=;
//pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
//sigfillset(&act_open.sa_mask);
//act_open.sa_handler=sighandler_1;
//sigaction(SIGUSR1,&act_open,0);
struct sched_param parm;
int pol=SCHED_FIFO;
parm.sched_priority = 20;
pthread_setschedparam(pthread_self(),SCHED_FIFO, &parm);
pthread_getschedparam(pthread_self(),&pol , &parm);
my_id1=pthread_self();
//pthread_getaffinity_np(pthread_self(),sizeof(cpu_set_t), &cpuset);
		printf("low priority thread : %ld\n",pthread_self());
	while(1){
//pthread_mutex_lock(&mut);
		cnt++;
//		printf("lp thread %d \n",parm.sched_priority);
//pthread_mutex_unlock(&mut);
	}
}
void highp_thread(){
struct sched_param parm;
int pol=SCHED_FIFO;
parm.sched_priority =  30 ;//sched_get_priority_max(SCHED_FIFO)-10;
pthread_setschedparam(pthread_self(),SCHED_FIFO, &parm);
pthread_getschedparam(pthread_self(),&pol , &parm);
my_id1=pthread_self();

		printf("high priority thread : %ld\n",pthread_self());
//		printf("hp thread %d %d\n",sched_get_priority_min(SCHED_FIFO),sched_get_priority_max(SCHED_FIFO),);
	while(1){
//pthread_mutex_lock(&mut);
		cnt--;
//		printf("hp thread priority : %ld\n",pthread_self());
//pthread_mutex_unlock(&mut);

	}
}
void semwait_thread_1(){
//	signal(SIGUSR1,sighandler_3);
printf("in thread 1 which is waiting for semaphore to get released\n");
my_id1=pthread_self();
		printf("sem wait thread : %ld\n",pthread_self());
sem_wait (&sem);
printf("after sem_wait\n");
sem_post(&sem);
}
void semsig_thread(){
printf("in thread 2 which is waiting for semaphore to get released\n");
my_id1=pthread_self();
		printf("sem sig thread : %ld\n",pthread_self());
sem_wait (&sem);
sem_post(&sem);
printf("end of thread \n");}
void sleep_thread(){
//signal(SIGUSR1,sighandler_4);
pthread_sigmask(SIG_UNBLOCK,&set,NULL);
while(1){
	printf("before I go to sleep\n");
/*	while(1){
		//sleep 
		cnt++;
	}*/
//	int retval=usleep(10000000);
	sleep(10);
	printf("after sleep in %ld  with usleep returning %s \n",pthread_self(),strerror(errno));
	}
}
pthread_t t1,t2,t3,t4,t5;
pthread_attr_t attr;
//sigset_t set;
//struct siched_param *param,parm;
int *policy;
int main(int argc, char **argv){
int sltime;
printf("%ld\n",pthread_self());
struct sched_param parm;
 struct sigaction action;
int pol=SCHED_FIFO;
	if(argc!=2){
		printf("format needs to be ./task_2 <thread number>\n where thread number being the kind of result you want to see:\n'1' : for normal running thread\n'2' : for high priority thread \n'3' : for semaphore waiting thread \n'4' : for thread with sleep \n");
		exit(0);
	}
if(pthread_mutex_init(&mut,NULL)!=0){
printf("lock init failed \n");
}
 CPU_ZERO(&cpuset2);
CPU_SET(0, &cpuset2);
pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset2);
cnt=0;
parm.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
pthread_setschedparam(pthread_self(),SCHED_FIFO, &parm);
 CPU_ZERO(&cpuset);
CPU_SET(0, &cpuset);
//	signal(SIGUSR1,sighandler_1);	
printf("this is second task to show signal handling how it works and what happens in different conditions  %c\n",*argv[1]);
if(*argv[1]== '1'){
	signal(SIGUSR1,sighandler_1);	
pthread_create(&t1,NULL,(void *)&cont_thread,NULL);
//pthread_setaffinity_np(t1, sizeof(cpu_set_t), &cpuset);
}
else if(*argv[1]=='2'){
	signal(SIGUSR1,sighandler_1);	
//parm.sched_priority = sched_get_priority_min (SCHED_FIFO); /* next-to-highest priority */ 
pthread_create(&t1,NULL,(void *)&cont_thread,NULL);
sleep(1);
//pthread_setaffinity_np(t1, sizeof(cpu_set_t), &cpuset);
pthread_create(&t2,NULL,(void *)&highp_thread,NULL);
}
else if(*argv[1]=='3'){
 //struct sigaction action;
    action.sa_handler = &sighandler_3; // your function
    action.sa_flags   = SA_RESTART; // this flag means that system tries to resume interrupted system calls
	sigfillset(&action.sa_mask);
    sigaction(SIGUSR1, &action, NULL);
pthread_create(&t1,NULL,(void *)&semwait_thread_1,NULL);
//pthread_create(&t2,NULL,(void *)&semsig_thread,NULL);
}
else if(*argv[1]=='4'){
	signal(SIGUSR1,sighandler_4);
	sigemptyset(&set);
	sigaddset(&set,SIGUSR1);
pthread_create(&t1,NULL,(void *)&sleep_thread,NULL);
pthread_sigmask(SIG_BLOCK,&set,NULL);

	while(1){
		sltime=rand()%1000;
		usleep((sltime+1)*1000);
		pthread_kill(t1,SIGUSR1);
		}
	}

while(1){
	sltime=rand()%1000;
	printf(" %d \n",sltime);
	usleep((sltime+1)*1000);
//	pthread_kill(t1,SIGUSR1);
	printf("pois %d \n",raise(SIGUSR1));
}
//if(*argv[1]=='1'){
if(*argv[1]== '1'){
pthread_join(t1,NULL);
}//cnt++;
else if(*argv[1]=='2'){
pthread_join(t1,NULL);
pthread_join(t2,NULL);
}
else if(*argv[1]=='3'){
pthread_join(t1,NULL);
}
else if(*argv[1]=='4'){
pthread_join(t1,NULL);
}
return 0;
}
