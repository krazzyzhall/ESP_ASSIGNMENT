#KDIR_L:=/usr/src/linux-headers-`uname -r`/build
#PWD:= $(shell pwd)
#KDIR_L:=/lib/modules/`uname -r`/build
#KDIR_G:=/opt/clanton-full/1.4.2/sysroots/i586-poky-linux/usr/src/kernel
#CC= gcc
#CC_G = i586-poky-linux-gcc
#ARCH = x86
#CROSS_COMPILE = i586-poky-linux-
#SROOT_G=/opt/clanton-full/1.4.2/sysroots/i586-poky-linux/

#APP = gmem_tester

obj-m:= Squeue.o

i686:
	make -f i686_make

clanton:
	make -f clanton_make

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
