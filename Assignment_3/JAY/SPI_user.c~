#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
int SPI_INIT();
struct spi_ioc_transfer Display_Data;
static uint32_t spi_speed = 6250000;
static uint8_t	word_length = 8;
uint32_t mode = 0x00;
uint32_t lsb = 0;
int sfd;
int SPI_Write(unsigned long tx_buff);

int main()
{
        sfd = open("/dev/spidev1.0", O_RDWR);
	SPI_INIT();
	uint16_t tx_buff[2];
	tx_buff[1] = 0x01;
	tx_buff[0] = 0x0C;
	SPI_Write((unsigned long)tx_buff);

	tx_buff[0] = 0x0F;
	tx_buff[1] = 0x01;
	SPI_Write((unsigned long)tx_buff);
/*
	tx_buff[0] = 0x0B;
	tx_buff[1] = 0x07;
	SPI_Write((unsigned long)tx_buff);

	tx_buff[0] = 0x0A;
	tx_buff[1] = 0x0F;
	SPI_Write((unsigned long)tx_buff);

	tx_buff[0] = 0x09;
	tx_buff[1] = 0x00;
	SPI_Write((unsigned long)tx_buff);

	tx_buff[0] = 0x01;
	tx_buff[1] = 0xFF;
	SPI_Write((unsigned long)tx_buff);

	tx_buff[0] = 0x02;
	tx_buff[1] = 0xFF;
	SPI_Write((unsigned long)tx_buff);

	tx_buff[0] = 0x03;
	tx_buff[1] = 0xFF;
	SPI_Write((unsigned long)tx_buff);

	tx_buff[0] = 0x04;
	tx_buff[1] = 0xFF;
	SPI_Write((unsigned long)tx_buff);*/
close(sfd);
	

	return 0;

	
}
int SPI_Write(unsigned long tx_buff){

	int ret = 0;
	//int fd;

	Display_Data.tx_buf 		= tx_buff;
	Display_Data.len 		= 2; 			
        Display_Data.cs_change 		= 0; 		
        Display_Data.delay_usecs	= 0; 		
        Display_Data.speed_hz 		= spi_speed;
        Display_Data.bits_per_word 	= word_length;

	printf("Written\n");

//	fd = open("/dev/spidev1.0", O_RDWR);
	ret = ioctl(sfd, SPI_IOC_MESSAGE(1),&Display_Data);
	printf("%d\n",ret);
	if(ret < 0){
		printf("Transfer Failed\n");
	}
	//close(fd);
	return 0;
}
int SPI_INIT(){

	/* Setting GPIO */

	int fd;
	int gpio;


	/* Setting GPIO 4 */
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "4", 2);
	close(fd);

	fd = open("/sys/class/gpio/gpio4/direction",O_RDWR);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "out", 4); 
	close(fd);

	fd = open("/sys/class/gpio/gpio4/value",O_RDWR);
	write(fd, "1", 2); 
	close(fd);

	/* Setting GPIO 42 */
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "42", 3);
	close(fd);

	fd = open("/sys/class/gpio/gpio42/direction",O_RDWR);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "out", 4); 
	close(fd);

	fd = open("/sys/class/gpio/gpio42/value",O_RDWR);
	write(fd, "0", 2); 
	close(fd);

	/* Setting GPIO 43 */
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "43", 3);
	close(fd);

	fd = open("/sys/class/gpio/gpio43/direction",O_RDWR);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "out", 4); 
	close(fd);

	fd = open("/sys/class/gpio/gpio43/value",O_RDWR);
	write(fd, "0", 2); 
	close(fd);

	/* Setting GPIO 54 */
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "54", 3);
	close(fd);

	fd = open("/sys/class/gpio/gpio54/direction",O_RDWR);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "out", 4); 
	close(fd);

	fd = open("/sys/class/gpio/gpio54/value",O_RDWR);
	write(fd, "0", 2); 
	close(fd);

	/* Setting GPIO 55 */
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "55", 3);
	close(fd);

	fd = open("/sys/class/gpio/gpio55/direction",O_RDWR);
	if(fd<0){
	printf("failed\n");
	}
	write(fd, "out", 4); 
	close(fd);

	fd = open("/sys/class/gpio/gpio55/value",O_RDWR);
	write(fd, "0", 2); 
	close(fd);
	
	/* All GPIO set */

	printf("All GPIO created and set\n");


	/* Setting SPI speed */
	if(sfd < 0){
		printf("File not Found\n");
		return 0;
	}

	if (ioctl(sfd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) 
         {
        	printf("SPI Speed Not set\n");
         }

	if (ioctl(sfd, SPI_IOC_WR_BITS_PER_WORD, &word_length) < 0) 
         {
        	printf("Word Length not set\n");
         }

	if (ioctl(sfd, SPI_IOC_WR_MODE, &mode) < 0) 
         {
        	printf("Mode not set\n");
         }

	if (ioctl(sfd, SPI_IOC_RD_MODE, &mode) < 0) 
         {
        	printf("Mode not read\n");
         }

	printf("SPI mode  	: %d\n", mode);
	printf("SPI Bits/Word  	: 16\n");
	printf("SPI speed 	: 1Mhz\n");
//	close(fd);

	return 0;

}
