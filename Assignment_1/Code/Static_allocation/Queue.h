//------------------------------------Struct Definition----------------------
struct queue_unit{
      struct queue_unit *next;     // pointer to the next element of the queue
      void *cont;                  // content of this element of the queue
      struct queue_unit *prev;     // pointer to previous element of the queue
};

struct Squeue{
       struct queue_unit *head;   //queue Structure holding the head info of the 
                                  // queue created
};
//-------------------------------------- Function Definition------------------



struct Squeue* createSqueue(); //Function for Creating the queue will return a pointer
                               // to a newly created queue
int Enqueue(struct Squeue* queue,void * content); // function for Enquing a Queue unit
void* Dqueue(struct Squeue *queue);              //function for Dwqueuing a Queue unit
int Release_queue(struct Squeue *queue);         //function for Releasing and freeing 
                                             //the memory of created queue
