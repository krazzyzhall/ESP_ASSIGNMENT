#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>   //for usleep function
#include "message.h"
#define BD_slp_time  1000
#define rcv_slp_time 1000
#define snd_slp_time 1000
typedef struct ipc_msg ipc_msg;
// function prototype 
void bus_deamon();        //bus-deamon :- for routing messages to other queues
int sender( long *my_id);  // sender :- sending messages to a specific queue
int receiver(long *my_id); //receiver :- one which receives all the messages from a fixed queue
int get_outfd();
int link_me(long pt);
long get_msg_id();
//global variables 
static int out_fd_arr[3];  // static variable to store fd for all the out files
static long gmsg_id=0;     // global message id variable
pthread_mutex_t lock;      //lock variable for mutual exclusion
static int send_fd=0;      // fd for the queue where all senders will send the messages
static int available;      //to check if any out device is available
static long receiver_id[3]; // stores the receiver id
