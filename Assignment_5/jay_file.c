#define _GNU_SOURCE

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <linux/input.h>
#include <setjmp.h>
#include <sched.h>


void Event_Detect();		// Function to detect Mouse event
int count;

/* Signal Handler for SIGUSR1 and SIGUSR2 */
void my_signal_handler(int sig)
{
	printf("Signal Received \n");
}

sigset_t set;
pthread_t Main_Process_id;


int main(){
	
	int i; 
	int s;

	cpu_set_t cpuset;
	pthread_t thread1;

	sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
	signal(SIGUSR1,my_signal_handler); 				// Registering signal handler 

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);

	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setaffinity_np(thread1, sizeof(cpu_set_t), &cpuset);

 	pthread_create(&thread1, NULL,(void*)Event_Detect, NULL); 	// Creating Mouse detect 
	pthread_sigmask(SIG_BLOCK, &set, NULL); 		  	// Masking signals

	sleep(1);

	for(i=0;i<100;i++){
		pthread_kill(thread1,SIGUSR1);
		printf("Signal Sent\n");
		usleep(10000);
	 }
	
	sleep(1);
	pthread_cancel(thread1);

	return 0;

}

/* Thread for detecting mouse event */
void Event_Detect(){
	int s;
	count = 0;

	s = pthread_sigmask(SIG_UNBLOCK, &set, NULL); // Masking signals

	while(1)
	{	
		sleep(10);
		printf("Interrupted\n");
	}

	pthread_exit(0);
}
