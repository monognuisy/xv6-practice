#include "types.h"
#include "stat.h"
#include "user.h"

#define __TEST_LOCKNUM 10000

int 
main(int argc, char* argv[]) 
{
  int pid;

  if (argc < 2) {
    printf(1, "Usage: lock_test [lock / int / etc...]\n");
    printf(1, "lock  : use scheduler lock\n");
    printf(1, "int   : use interrupt for scheduler lock\n");
    printf(1, "etc...: use mlfq scheduler\n");
    exit();
  }

  printf(1, "Special scheduling start:\n");
  
  // child process
  if ((pid = fork()) == 0) {
    int x = 0;
    for (int i = 0; i < __TEST_LOCKNUM; i++) {
      for (int j = 0; j < __TEST_LOCKNUM; j++) {
        x += 1;
      }
      if (!(i % 1000)) {
        printf(1, "[child] level: %d, x: %d\n", getLevel(), x);
      }
    }

    exit();
  }

  // parent process
  // lock scheduler
  if (!strcmp(argv[1], "lock"))
    schedulerLock(2021031685);
  else if(!strcmp(argv[1], "int")) {
    printf(1, "int called!\n");
    __asm__("int $129");
  }

  int y = 0;
  for (int i = 0; i < __TEST_LOCKNUM; i++) {
    for (int j = 0; j < __TEST_LOCKNUM; j++) {
      y += 1;
    }
    if (!(i % 1000)) {
        printf(1, "[parent] level: %d, y: %d\n", getLevel(), y);
    }
  }

  if (!strcmp(argv[1], "lock"))
    schedulerUnlock(2021031685);
  else if(!strcmp(argv[1], "int"))
    __asm__("int $130");

  wait();

  exit();
}