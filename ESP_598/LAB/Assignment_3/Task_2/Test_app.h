#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <pthread.h>
#include "led_mat.h"

#define MY_CPU_FREQ 4000000

static float Dist;
/*typedef struct param{
int fd_spi;
};*/
typedef struct {
char led[8];
}pattern;
pthread_mutex_t lock;
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

float get_dist(int fd);

float get_speed(int fd);

int get_direction(int fd);

int set_spi(int fd);

float get_dist(int fd){
float dist=0;
long measure;
int status;
	write(fd,NULL,1);
	usleep(1000);
	status=read(fd,&measure,1);
	if(status<0){
		return -1;
	}
	else{
		dist=(float)((float)340*measure)/(MY_CPU_FREQ*2);
		return dist;
	}
}

float get_speed(int fd){
float dist[2],speed;
long tbefore=0,tafter=0;
int status;
	tbefore=rdtsc();
	dist[0]=get_dist(fd);
	usleep(10);
	dist[1]=get_dist(fd);
	tafter=rdtsc();
	if(dist[0]<0 || dist[1]<0){
		return -1;
	}
	else{
		if(dist[1]>dist[0]){
		speed=(dist[1]-dist[0])*(MY_CPU_FREQ)/(tafter-tbefore);
		return speed;}
		else{
			speed=(dist[0]-dist[1])*(MY_CPU_FREQ)/(tafter-tbefore);
		return speed;}
	}
}

int get_direction(int fd){
float dist[2];
	dist[0]=get_dist(fd);
	usleep(10);
	dist[1]=get_dist(fd);
	if(dist[1]>=dist[0])
	return 1;
	else
	return -1;
}

int set_spi(int fd){
char data[2];
int speed,bitperword,msb,res,i;
bitperword=8;
speed=10000000;
msb=0;
	ioctl(fd,SPI_IOC_WR_LSB_FIRST,&msb);
	ioctl(fd,SPI_MODE_0);
	ioctl(fd,SPI_IOC_WR_BITS_PER_WORD,&bitperword);
	ioctl(fd,SPI_IOC_WR_MAX_SPEED_HZ,&speed);
	ioctl(fd,ISPATTERN,(unsigned long)0);
	ioctl(fd,SEQ_ENABLE,(unsigned long)0);
	data[1]=0x00;        //to remove from mode
	data[0]=0x0F;
	res=write(fd,data,2);
	printf("res : %d\n",res);
	data[1]=0x01;        //to set to normal mode
	data[0]=0x0C;
	res=write(fd,data,2);
	printf("res : %d\n",res);
	data[1]=0x00;       // to set to decode
	data[0]=0x09;
	res=write(fd,data,2);
	printf("res : %d\n",res);
	data[1]=0x07;      //scan limit
	data[0]=0x0B;
	res=write(fd,data,2);	
	printf("res : %d\n",res);
	data[1]=0x08;
	data[0]=0x0A;
	res=write(fd,data,2);
	for(i=0;i<8;i++){
		data[0]=i;
		data[1]=0x00;
		res=write(fd,data,2);
	}
		printf("at the end of SET_SPI");
	return 0;
}

int send_pat_set(int fd, pattern *set){
int retval=0;
ioctl(fd,ISPATTERN,(unsigned long)1);   // is pattern will get reset after successfull pattern writing
retval=write(fd,set,80);
return retval;
}
int insert_seq(int fd,struct my_seq_pat *seq,int count){
int retval;
ioctl(fd,SEQ_ENABLE,(unsigned long)1);   ///to enable sequence mode
retval=ioctl(fd,INS_SEQ,seq);   //to put in sequence and create a thread that will run a thread
return retval;
}
void dist_thread(int *pfd){
int temp;
int fd=*pfd;
	while(1){
		//lock
		temp=get_dist(fd);
		if(temp>0){
			Dist=temp;
			}
		//lock
		usleep(100000);
		printf(" %f \n",Dist);
	}
}
void motionpicture(int *sfd){
int fd=*sfd;
float dist;
int speed=1;
int direction;
struct my_seq_pat *seq1,*seq2,*seq3,*seq4,*seq5;

seq1=malloc(sizeof(struct my_seq_pat));
seq1->seq=malloc(6*sizeof(int));
seq1->lseq=6;
seq1->seq[0]=0;seq1->seq[1]=150;seq1->seq[2]=2;seq1->seq[3]=150;seq1->seq[4]=0;seq1->seq[5]=0;

seq3=malloc(sizeof(struct my_seq_pat));
seq3->seq=malloc(6*sizeof(int));
seq3->lseq=6;
seq3->seq[0]=6;seq3->seq[1]=250;seq3->seq[2]=1;seq3->seq[3]=250;seq3->seq[4]=0;seq3->seq[5]=0;

seq2=malloc(sizeof(struct my_seq_pat));
seq2->seq=malloc(6*sizeof(int));
seq2->lseq=6;
seq2->seq[0]=5;seq2->seq[1]=250;seq2->seq[2]=1;seq2->seq[3]=250;seq2->seq[4]=0;seq2->seq[5]=0;

seq4=malloc(sizeof(struct my_seq_pat));
seq4->seq=malloc(6*sizeof(int));
seq4->lseq=6;
seq4->seq[0]=4;seq4->seq[1]=250;seq4->seq[2]=1;seq4->seq[3]=250;seq4->seq[4]=0;seq4->seq[5]=0;
//seq5->seq[0]=5;seq5->seq[1]=250;seq5->seq[2]=1;seq5->seq[3]=250;seq5->seq[4]=0;seq5->seq[5]=0;

seq5=malloc(sizeof(struct my_seq_pat));
seq5->seq=malloc(6*sizeof(int));
seq5->lseq=6;
seq5->seq[0]=4;seq5->seq[1]=250;seq5->seq[2]=1;seq5->seq[3]=250;seq5->seq[4]=0;seq5->seq[5]=0;

ioctl(fd,SEQ_ENABLE,(unsigned long)1);   ///to enable sequence mode
printf("entered  motionpicture %d\n",fd);
	while(1){
		//lock
		dist=Dist;
		//unlock//get_dist(fd_usonic);
//		speed=get_speed(fd_usonic);
//		direction=get_direction(fd_usonic);
//			printf("Dist %f \n",dist);

		if(dist<0 || speed<0){
			continue;
		}
		else{ 
			if(dist<20){
			ioctl(fd,INS_SEQ,seq1);   //to put in sequence and create a thread that will run a thread
			}
/*			if(dist <20 && direction ==-1 ){
		//	ioctl(fd,INS_SEQ,seq1);   //to put in sequence and create a thread that will run a thread
			}
*/			if(dist >=20 && dist <50 ){
			ioctl(fd,INS_SEQ,seq4);   //to put in sequence and create a thread that will run a thread
			}
/*			if(dist >=20 && dist <150 && direction ==-1){
			ioctl(fd,INS_SEQ,myseq);   //to put in sequence and create a thread that will run a thread
			}
*/			if(dist >=50 ){
			ioctl(fd,INS_SEQ,seq2);   //to put in sequence and create a thread that will run a thread
			}/*
			if(dist >=150 && direction ==-1){
			ioctl(fd,INS_SEQ,myseq);   //to put in sequence and create a thread that will run a thread
			}*/
		}
	}
}
