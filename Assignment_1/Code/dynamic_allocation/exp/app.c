#include"app.h"   //header which includes all necessary headers and definitions and declarations

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
                 ran=rand()%10 + 1;
                usleep(ran*BD_slp_time);   //will sleep if fails to read
                }
            temp_dest=msg->destination_id;
             if     (temp_dest==receiver_id[0] && out_fd_arr[0]>0) {printf("in 0\n"); temp_fd=out_fd_arr[0]; }  //
             else if(temp_dest==receiver_id[1] && out_fd_arr[1]>0) {printf("in 1\n"); temp_fd=out_fd_arr[1]; }
             else if(temp_dest==receiver_id[2] && out_fd_arr[2]>0) {printf("in 2\n"); temp_fd=out_fd_arr[2]; }
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
           usleep(snd_slp_time/10);
    }
   
   usleep((ran+1)*snd_slp_time);
}
return 0;
}
/*
receiver() - for receiving and displaying the message
*/
int receiver(long *pmy_id){
int re;
long my_id=*pmy_id;
pthread_mutex_lock(&lock);
int myout_fd=out_fd_arr[link_me(my_id)];     //will link the reciever thread to the bus deamon and return q
pthread_mutex_unlock(&lock); 
ipc_msg *rcv_msg=(ipc_msg*)malloc(sizeof(ipc_msg));
while(1){
re=-1;
rcv_msg=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));
while(re < 0){
re=read(myout_fd,rcv_msg,sizeof(struct ipc_msg));}
printf(" \n reciever %ld: %s from %ld,with total_enqueue_time as :%ld.\n",my_id,rcv_msg->message,rcv_msg->source_id,rcv_msg->accumulated_queing_time);  
free(rcv_msg);
usleep(rcv_slp_time);
}
return 0;
}