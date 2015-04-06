sudo rmmod Squeue
make clean
make
sudo insmod Squeue.ko
gcc -o try main.c
sudo ./try
