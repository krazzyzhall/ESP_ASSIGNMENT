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
void bus_deamon();
int sender();
int receiver();
static int out_fd_arr[3];
pthread_mutex_t lock;
static int send_fd=0;
static int available;   //to check if any out device is available
static pthread_t receiver_id[3];
int get_outfd(){
   return send_fd;
}
int link_me(pthread_t pt){
  if(available<3){
   // receiver_id[available]=pt;
     return available++;}
   else
     return -1;
}
/*
bus_deamon- 
tasks:
- handlind the message passing
-does it every 10 ms
*/
void bus_deamon(){
//initialize the bus daemon  for handeling processes
struct ipc_msg *msg=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));  // to save message from the in queue
int in_fd,out_fd1,out_fd2,out_fd3,temp_fd;    //variables required for FD's
int temp_dest;
in_fd=open("/dev/bus_in_q",O_RDWR);
send_fd=in_fd;
out_fd1=open("/dev/bus_out_q1",O_RDWR);
out_fd2=open("/dev/bus_out_q2",O_RDWR);
out_fd3=open("/dev/bus_out_q3",O_RDWR);
if(in_fd<0 || out_fd1<0 || out_fd2<0 || out_fd3<0){
   printf("unable to open one of the device files\n");
return;
}
//printf("bus daemon: opened files \n");
int rerr,werr;
// pthread_mutex_lock(&lock); 
out_fd_arr[0]=out_fd1;
out_fd_arr[1]=out_fd2;
out_fd_arr[2]=out_fd3;
//sig enable signal for getting fd for the devices. 
//pthread_mutex_unlock(&lock);
while(1){
//printf("bus daemon :in while\n bus daemon:outfd1:%d fd 2:%d fd 3:%d",out_fd_arr[0],out_fd_arr[1],out_fd_arr[2]); 
//lock the resource
// pthread_mutex_lock(&lock); 
rerr=read(in_fd,msg,(int)sizeof(struct ipc_msg));  //read from driver and save it in msg
if(rerr==-1){
 return;
}
/*if(msg==-1){
printf("no message to recieve");
}
printf("source_id %ld\n",msg->source_id);
printf("destination id : %ld\n",msg->destination_id);
printf("%s",msg->message);
//unlock the mutext*/
//pthread_mutex_unlock(&lock);
temp_dest=msg->destination_id;
//printf("\n msgdst %ld : rec1 %ld   :rec2  %ld  :rec3  %ld\n",temp_dest,receiver_id[0],receiver_id[1],receiver_id[2]);
if(temp_dest==receiver_id[0] && out_fd_arr[0]<0){temp_fd=out_fd_arr[0];}
else if(temp_dest==receiver_id[1] && out_fd_arr[1]<0){temp_fd=out_fd_arr[1];}
else if(temp_dest==receiver_id[2] && out_fd_arr[2]<0){temp_fd=out_fd_arr[2];}
else{continue;}
//lock for write
// pthread_mutex_lock(&lock); 
write(temp_fd,msg,sizeof(struct ipc_msg));
//unlock it
//pthread_mutex_unlock(&lock);
usleep(10000);
}
return;
}

/*
sender_thread for sending the message
*/
int sender(){
//initialize
int loop=0;
struct ipc_msg *snd_msg;
long dest;
pthread_t thr_id=pthread_self();
snd_msg=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));
pthread_t rcv_id[3];
int fd_send;
while(fd_send>0){
fd_send=get_outfd();
}
//printf("sender : initiated\n");
while(1){   /*sender will not start sending till all the recievers are setup */
printf("in uninitialed recv loop\n"); 
   if(receiver_id[0]>0 && receiver_id[1]>0 && receiver_id[2]>0){
       rcv_id[0]=receiver_id[0];
       rcv_id[1]=receiver_id[1];
       rcv_id[2]=receiver_id[2];
       break;
   }
usleep(100000);
}
//printf("sender :recieved all reciever id\n");
while(loop<10){
loop++;
   srand(10);
   dest=rand();
   dest=(dest)-3*(dest/3);
   snd_msg->destination_id=rcv_id[dest];
   snd_msg->source_id=thr_id;
// pthread_mutex_lock(&lock);
//printf("sender :this message is from %ld sender thread:,to rcv :%ld,%d",thr_id,receiver_id[dest],dest);
   sprintf(snd_msg->message,"this message is from %d sender thread",thr_id);
//   printf("sender:%s\n",snd_msg->message);
   //mutex lock on
   write(fd_send,snd_msg,sizeof(struct ipc_msg));
   //mutek lock off
//pthread_mutex_unlock(&lock);
   usleep(100000);
}
//end
}
/*
receiver() - for receiving and displaying the message
*/
int receiver(){
pthread_t my_id=pthread_self();
//printf("reveiver : available:%d\n",available);
int myout_fd=link_me(my_id);     //will link the reciever thread to the bus deamon and return q
//printf("receiver: out_fd = %d\n",myout_fd);
struct ipc_msg *rcv_msg=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));
while(1){
// pthread_mutex_lock(&lock);
//printf("reciever %ld :says hi %d \n",my_id,myout_fd);
read(myout_fd,rcv_msg,sizeof(struct ipc_msg));
//printf("\nreciever %ld:    %s from %ld\n",my_id,rcv_msg->message,rcv_msg->source_id);  
//pthread_mutex_unlock(&lock); 
usleep(100000);
}
return 0;
}
int main(){
//struct rtc_time t=(struct rtc_time)malloc(sizeof(struct rtc_time));
time_t now;
available=0;
struct tm * time_info;
struct ipc_msg *msg;
struct ipc_msg *inp;
msg=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));
inp=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));
pthread_t bus_dmn,sender_th[3];
pthread_t rcv_th[3];

if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
//bus_deamon
//printf("in main:\n");
pthread_create( &bus_dmn, NULL, (void*)bus_deamon,NULL);
//printf("after creating thread");
pthread_create( &receiver_id[0], NULL, (void*)receiver,NULL);
pthread_create( &receiver_id[1], NULL, (void*)receiver,NULL);
pthread_create( &receiver_id[2], NULL, (void*)receiver,NULL);
pthread_create( &sender_th[0], NULL, (void*)sender,NULL);
//pthread_create( &sender_th[1], NULL, (void*)sender,NULL);
//pthread_create( &sender_th[2], NULL, (void*)sender,NULL);
//3 receiver
//3 sender
pthread_join(bus_dmn,NULL);
pthread_join(receiver_id[0],NULL);
pthread_join(receiver_id[1],NULL);
pthread_join(receiver_id[2],NULL);
pthread_join(sender_th[0],NULL);
//pthread_join(sender_th[1],NULL);
//pthread_join(sender_th[2],NULL);
pthread_mutex_destroy(&lock);
return 0;
}
