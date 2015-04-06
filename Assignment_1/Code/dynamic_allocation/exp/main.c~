#include "app.c"

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
msg=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));
inp=(struct ipc_msg*)malloc(sizeof(struct ipc_msg));
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
printf("in_fd : %d\n",in_fd);
out_fd=open("/dev/bus_out_q1",O_RDWR);
out_fd_arr[0]=out_fd;
printf("outfd  1: %d\n",out_fd);
out_fd=open("/dev/bus_out_q2",O_RDWR);
out_fd_arr[1]=out_fd;
printf("outfd  2: %d\n",out_fd);
out_fd=open("/dev/bus_out_q3",O_RDWR);
out_fd_arr[2]=out_fd;
printf("outfd  3: %d\n",out_fd);
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
usleep(10000000);
printf(" after creating thread \n");
pthread_cancel(sender_th[0]);
pthread_cancel(sender_th[1]);
pthread_cancel(sender_th[2]);
sleep(2);
pthread_cancel(rcv_th[0]);
pthread_cancel(rcv_th[1]);
pthread_cancel(rcv_th[2]);
sleep(2);
pthread_cancel(bus_dmn);
printf("gmsg-id : %ld",gmsg_id);
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
