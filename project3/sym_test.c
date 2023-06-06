#include "types.h"
#include "user.h"
#include "stat.h"
#include "fcntl.h"

#define BUFLEN 100

char buf1[BUFLEN], buf2[BUFLEN];
char filename[16] = "sym_original";
char symname[16] = "sym_sym";

int writeTest() {
  int fd;

  fd = open(symname, O_WRONLY);

  if (fd < 0) return -1;
  if (write(fd, buf1, BUFLEN) < 0) return -1;
  if (close(fd) < 0) return -1;
  
  return 0;
}

int readTest() {
  int fd;

  fd = open(filename, O_RDONLY);

  if (fd < 0) return -1;
  if (read(fd, buf2, BUFLEN) < 0) return -1;
  
  for (int i = 0; i < BUFLEN; i++) {
    if (buf2[i] != (i % 26) + 'a') {
      printf(1, "%dth character, expected %c, found %c\n", i, (i % 26) + 'a', buf2[i]);
      return -1;
    }
  }

  if (close(fd) < 0) return -1;
  if (unlink(symname) < 0) return -1;
  if (unlink(filename) < 0) return -1;

  return 0;
}

void failed(char* msg) {
  printf(2, "symlink test: %s\n", msg);
  exit();
}

int
main(void)
{
  for (int i = 0; i < BUFLEN; i++) {
    buf1[i] = (i % 26) + 'a';
  }  

  // touch original file
  int fd;
  fd = open(filename, O_CREATE | O_WRONLY);

  if (fd < 0) failed("main");
  if (close(fd) < 0) return -1;

  // make symlink
  symlink(filename, symname);

  if (writeTest() < 0) failed("write failed");
  if (readTest() < 0) failed("read failed");

  printf(1, "test finished!\n");

  exit();
  return 0;
}