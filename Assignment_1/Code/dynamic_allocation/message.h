//#include <pthread.h>
#include <linux/types.h>
struct ipc_msg{
   long message_id;
   long source_id;                      //sender id number i.e. thread number
   long destination_id;                 // recieving id number 
   char message[80];                   //content to be sent
   unsigned long enqueue_time;         //the jiffy when this message was queued
   unsigned long accumulated_queing_time;
};


