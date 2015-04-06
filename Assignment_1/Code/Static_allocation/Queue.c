#include "Queue.h"

struct Squeue* createSqueue(){
    return (struct Squeue*)malloc(sizeof(struct Squeue));
}

int Enqueue(struct Squeue* queue,void * content){
     struct queue_unit *temp,*temp2;
     temp2=NULL;
     temp=queue->head;
     temp2=(struct queue_unit*)malloc(sizeof(struct queue_unit));
     if(temp2==NULL){
        return -1;
     }  
     temp2->cont=content;
     if(queue->head==NULL){    //base case when queue is null
          queue->head=temp2;
          queue->head->next=queue->head;
          queue->head->prev=queue->head;
          return 0;
     }
     queue->head=temp2;     
     queue->head->next=temp;
     queue->head->prev=temp->prev;
     temp->prev=queue->head;
     return 0;
}
void* Dqueue(struct Squeue *queue){
    void *vtemp;
    vtemp=NULL;
    if(queue->head==NULL)return vtemp;   //if nothing in the queue
    if(queue->head->prev==queue->head){  // if only one element in the queue
           vtemp=queue->head->cont;
           queue->head=NULL;
           return vtemp;
    }
    struct queue_unit *temp=queue->head->prev;     // general case
    temp->prev->next=queue->head;
    queue->head->prev=temp->prev;
    vtemp=temp->cont;
    free(temp);
    return vtemp;
}

int Release_queue(struct Squeue *queue){
      free(queue);
      return 0;
}
