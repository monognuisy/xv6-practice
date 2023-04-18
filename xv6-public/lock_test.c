#include "types.h"
#include "stat.h"
#include "user.h"

#define __TEST_LOCKNUM 10000

int 
main(int argc, char* argv[]) 
{
  int pid;
  char* s = "lock";

  if (argc < 2) {
    printf(1, "Usage: lock_test [lock / unlock]\n");
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
  if (!strcmp(argv[1], s))
    schedulerLock(2021031685);

  int y = 0;
  for (int i = 0; i < __TEST_LOCKNUM; i++) {
    for (int j = 0; j < __TEST_LOCKNUM; j++) {
      y += 1;
    }
    if (!(i % 1000)) {
        printf(1, "[parent] level: %d, y: %d\n", getLevel(), y);
    }
  }

  if (!strcmp(argv[1], s))
    schedulerUnlock(2021031685);

  wait();

  exit();
}