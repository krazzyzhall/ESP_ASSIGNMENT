/*
this piece of code has 3 main functions :
 1) Spawning the Bus Daemon thread
 2) Spawning the sending threads 
 3) spawning the recieving threads
*/
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
void bus_deamon();        //function prototype
int sender( long *my_id); 
int receiver(long *my_id);
static int out_fd_arr[3];
static long gmsg_id=0;
static pthread_mutex_t lock;
static int send_fd=0;
static int available;   //to check if any out device is available
static long receiver_id[3];
int get_outfd(){
   return send_fd;
}
int link_me(long pt){
  if(available<3){
    receiver_id[available]=pt;
     return available++;
     }
   else
     return -1;
}
long get_msg_id(){
      return gmsg_id++;
}
/*
bus_deamon- for routing the messages from one queue to another
*/
void bus_deamon(){
ipc_msg *msg=NULL;     
msg=(ipc_msg*) malloc(sizeof(ipc_msg));  // to save message from the in queue
if(msg==NULL){
printf("msg not initiated \n");
}
int in_fd=0,out_fd=0,temp_fd=0,ran;    //variables required for FD's
long temp_dest;
int rerr=-1,werr=-1;
while(1){
       msg=(ipc_msg*)malloc(sizeof(ipc_msg));  // to save message from the in queue
rerr=-1;werr=-1;
        while( rerr < 0){
        rerr=read(send_fd,msg,sizeof(ipc_msg));   //read from driver and save it in msg
                 ran=(rand()%10) + 1;
                usleep(ran*BD_slp_time);   //will sleep if fails to read
                }
            temp_dest=msg->destination_id;
             if     (temp_dest==receiver_id[0] && out_fd_arr[0]>0) { temp_fd=out_fd_arr[0]; }  //
             else if(temp_dest==receiver_id[1] && out_fd_arr[1]>0) { temp_fd=out_fd_arr[1]; }
             else if(temp_dest==receiver_id[2] && out_fd_arr[2]>0) { temp_fd=out_fd_arr[2]; }
             else   { printf("in else of lookup  %ld\n",temp_dest); continue; }
            while(werr<0){
            ran=rand()%10;
            werr=write(temp_fd,msg,sizeof(ipc_msg));     // writing message to destination queue
            usleep((ran+1)*BD_slp_time);
            }
            free(msg);
        }
return;
}
/*
sender_thread for sending the message
*/
int sender(long *pmy_id){
//initialize
long my_id=*pmy_id;
ipc_msg *snd_msg;  //message structure declaration
int dest,ran;
int we=-1;
snd_msg=(ipc_msg*)malloc(sizeof(ipc_msg));
long rcv_id[3];
int in_fd=0;
while(in_fd<=0){
in_fd=get_outfd();
}
while(1){   //sender will not start sending till all the recievers are setup 
   if(receiver_id[0]>0 && receiver_id[1]>0 && receiver_id[2]>0){
 //pthread_mutex_lock(&lock);
       rcv_id[0]=receiver_id[0];
       rcv_id[1]=receiver_id[1];
       rcv_id[2]=receiver_id[2];
//pthread_mutex_unlock(&lock);
       break;
   }
usleep(1000);
}
while(1){

   dest=rand();
   dest=(dest%3);
   snd_msg->destination_id=rcv_id[dest];   //random destination 
   snd_msg->source_id=my_id;
   snd_msg->accumulated_queing_time=0;          //accumulated time is 0;
   snd_msg->enqueue_time=0;
 pthread_mutex_lock(&lock);
snd_msg->message_id=get_msg_id();    //get global message id 
   sprintf(snd_msg->message,"message id: %ld  this message is from %ld sender thread",snd_msg->message_id,my_id);
pthread_mutex_unlock(&lock);
we=-1;
 ran=rand()%10;
   while(we<0){
       
           we=write(in_fd,snd_msg,(unsigned long)sizeof(ipc_msg));
           if(we<0)printf("waiting for the queue to get empty");
           usleep(snd_slp_time);
    }
   
   usleep((ran+1)*snd_slp_time);
}
return 0;
}
/*
receiver() - for receiving and displaying the message
*/
int receiver(long *pmy_id){
int re,ran;
long my_id=*pmy_id;
pthread_mutex_lock(&lock);
int myout_fd=out_fd_arr[link_me(my_id)];     //will link the reciever thread to the bus deamon and return q
pthread_mutex_unlock(&lock); 
ipc_msg *rcv_msg=(ipc_msg*)malloc(sizeof(ipc_msg));
while(1){

re=-1;
ran=(rand()%10) +1;
rcv_msg=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));
while(re < 0){
re=read(myout_fd,rcv_msg,sizeof(struct ipc_msg));}
printf(" \n reciever %ld: %s from %ld,with total_enqueue_time as :%lu.\n",my_id,rcv_msg->message,rcv_msg->source_id,rcv_msg->accumulated_queing_time);  
free(rcv_msg);
usleep(ran*rcv_slp_time);
}
return 0;
}
int main(){
//struct rtc_time t=(struct rtc_time)malloc(sizeof(struct rtc_time));
available=0;
struct tm * time_info;
struct ipc_msg *msg;
struct ipc_msg *inp;
out_fd_arr[0]=0;
out_fd_arr[1]=0;
out_fd_arr[2]=0;
receiver_id[0]=0;
receiver_id[1]=0;
receiver_id[2]=0;
msg=(ipc_msg*)malloc(sizeof(ipc_msg));
inp=(ipc_msg*)malloc(sizeof(ipc_msg));
pthread_t bus_dmn,sender_th[3];
pthread_t rcv_th[3];

//if (pthread_mutex_init(&lock, NULL) != 0)
if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
//bus_deamon
//printf("in main:\n");
int in_fd=0,out_fd=0;
in_fd=open("/dev/bus_in_q",O_RDWR);
send_fd=in_fd;
//printf("in_fd : %d\n",in_fd);
out_fd=open("/dev/bus_out_q1",O_RDWR);
out_fd_arr[0]=out_fd;
//printf("outfd  1: %d\n",out_fd);
out_fd=open("/dev/bus_out_q2",O_RDWR);
out_fd_arr[1]=out_fd;
//printf("outfd  2: %d\n",out_fd);
out_fd=open("/dev/bus_out_q3",O_RDWR);
out_fd_arr[2]=out_fd;
//printf("outfd  3: %d\n",out_fd);
if(send_fd<0 || out_fd_arr[0]<0 || out_fd_arr[1]<0 || out_fd_arr[2]<0){
     printf("unable to open device \n");
return -1;
}
long rcv_id[3];
rcv_id[0]=10;
rcv_id[1]=11;
rcv_id[2]=12;
long snd_id[3];
snd_id[0]=20;
snd_id[1]=21;
snd_id[2]=22;
pthread_create( &bus_dmn, NULL, (void*)bus_deamon,NULL);
pthread_create( &rcv_th[0], NULL, (void*)receiver,&(rcv_id[0]));
pthread_create( &rcv_th[1], NULL, (void*)receiver,&(rcv_id[1]));
pthread_create( &rcv_th[2], NULL, (void*)receiver,&(rcv_id[2]));
pthread_create( &sender_th[0], NULL, (void*)sender,&(snd_id[0]));
pthread_create( &sender_th[1], NULL, (void*)sender,&(snd_id[1]));
pthread_create( &sender_th[2], NULL, (void*)sender,&(snd_id[2]));
sleep(10);
printf(" after creating thread \n");
pthread_cancel(sender_th[0]);
pthread_cancel(sender_th[1]);
pthread_cancel(sender_th[2]);
sleep(2);
pthread_cancel(bus_dmn);
sleep(1);
pthread_cancel(rcv_th[0]);
pthread_cancel(rcv_th[1]);
pthread_cancel(rcv_th[2]);
printf("gmsg-id : %ld \n",gmsg_id);
//pthread_join(bus_dmn,NULL);
//pthread_join(receiver_id[0],NULL);
//pthread_join(receiver_id[1],NULL);
//pthread_join(receiver_id[2],NULL);
//pthread_join(sender_th[0],NULL);
//pthread_join(sender_th[1],NULL);
//pthread_join(sender_th[2],NULL);
pthread_mutex_destroy(&lock);
return 0;
}
