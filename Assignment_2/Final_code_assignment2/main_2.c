#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#define FLASHGETS 1
#define FLASHGETP 7
#define FLASHPUTP 3
#define FLASHPUTS 4
#define FLASHERASE 5
int
main ()
{
  int fd, pgno, res;
  int pageno = 10;
  char *str = "this is test statement to be written on eeprom\n";
  fd = open ("/dev/i2c-flash", O_RDWR);
  printf (" i2c device file opened with fd as %d \n", fd);
  pgno = ioctl (fd, FLASHGETP, 2);
  printf (" i2c eeprom current pgno: %d \n", pgno);
  res = ioctl (fd, FLASHPUTP, pageno);
  pgno = ioctl (fd, FLASHGETP, 0);
  printf (" i2c eeprom current pgno after writing pgno %d \n", pgno);
  int wres = write (fd, str, 64);
  printf ("i2c data writing has status as: %d \n", wres);
  pgno = ioctl (fd, FLASHGETP, 0);
  printf (" i2c eeprom current pgno:%d \n", pgno);
  usleep (10000);
  res = ioctl (fd, FLASHPUTP, 10);
  char *f = (char *) malloc (64 * sizeof (char));
  int rd = read (fd, f, 64);
  printf ("i2c read status %d \n", rd);
  printf (" output is %s \n ", f);
  printf
    (" --------------------ioctl functions--------------------------------- \n ");
  pgno = ioctl (fd, FLASHGETS, 0);
  printf (" i2c ioctl paggeno %d \n", pgno);
  pgno = ioctl (fd, FLASHGETP, 0);
  printf (" i2c pageno %d \n", pgno);
  pgno = ioctl (fd, FLASHPUTP, 10);
  printf (" i2c pageno %d \n", pgno);
  pgno = ioctl (fd, FLASHERASE, 0xff);
  printf (" i2c erase status %d \n", pgno);
  close (fd);
  return 0;
}
