KDIR:=~/SDK/sysroots/i586-poky-linux/usr/src/kernel
#PWD:= $(shell pwd)
CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
SROOT=~/SDK/sysroots/i586-poky-linux/

APP = main_4


all:
	i586-poky-linux-gcc -pthread -lm -o $(APP) main_4.c 

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
