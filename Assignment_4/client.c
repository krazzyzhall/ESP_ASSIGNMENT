#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>

#define PORT_TIME       13              /* "time" (not available on RedHat) */
#define PORT_APP        9999              /* FTP connection port */
//#define SERVER_ADDR     "169.254.5.52"     /* localhost */
#define SERVER_ADDR     "127.0.0.1"     /* localhost */
#define MAXBUF          1024

int main(int argc,char *argv[])
{   int sockfd;
    struct sockaddr_in dest;
    char buffer[MAXBUF];

    /*---Open socket for streaming---*/
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("Socket");
        exit(errno);
    }

    /*---Initialize server address/port struct---*/
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT_APP);
    if ( inet_aton(argv[1], &dest.sin_addr.s_addr) == 0 )
    {
        perror(SERVER_ADDR);
        exit(errno);
    }

    /*---Connect to server---*/
    if ( connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 )
    {
        perror("Connect ");
        exit(errno);
    }

    /*---Get "Hello?"---*/
    bzero(buffer, MAXBUF);
//	send data to application
float	x,y,z,gx,gy,gz,n;
while(1){
	scanf("%f",&x);scanf("%f",&y);scanf("%f",&z);scanf("%f",&gx);scanf("%f",&gy);scanf("%f",&gz);
	sprintf(buffer,"%f %f %f %f %f %f\n",x,y,z,gx,gy,gz);
	n=write(sockfd,buffer,strlen(buffer));
	if (n < 0){	
		perror("error :  writing failed for socket");
                exit(1);
	}
    printf("%s", buffer);
}    //recv(sockfd, buffer, sizeof(buffer), 0);

    /*---Clean up---*/
    close(sockfd);
    return 0;
}


