KDIR:=~/SDK/sysroots/i586-poky-linux/usr/src/kernel
#PWD:= $(shell pwd)
CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
SROOT=~/SDK/sysroots/i586-poky-linux/

APP = app

#obj-m:= pulse.o
#obj-m:= spidev.o
#obj-m:= led_mat.o
#obj-m:= myled_mat.o
#obj-m:= newled_mat.o

all:
#	make ARCH=x86 CROSS_COMPILE=i586-poky-linux- -C $(KDIR) M=$(PWD) modules
	i586-poky-linux-gcc -pthread -o $(APP) Application.c  #SPI_user_test.c #--sysroot=$(SROOT)

clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.symvers
	rm -f modules.order
	rm -f *.mod.c
	rm -rf .tmp_versions
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f \.*.cmd
	rm -f Module.markers
	rm -f $(APP) 
