#include "types.h"
#include "stat.h"
#include "user.h"

#define __TEST_YIELDNUM 1000000

int
main(int argc, char *argv[])
{
  int pid;

  if (argc < 2) {
    printf(1, "Usage: yield_test [yield / etc...]\n");
    printf(1, "yield : yield in L2\n");
    printf(1, "etc...: default mlfq scheduler\n");
    exit();
  }
  
  if ((pid = fork()) == 0) {
    int x = 0;
    for (;;) {
      for (int i = 0; i < __TEST_YIELDNUM; i++) {
      x = x;
      }

      printf(1, "[child]: %d\n", getLevel());
    }

    exit();
  }

  int x = 0;
  for (;;) {
    // default: in L2, parent always runs first
    setPriority(getpid(), 0);
    for (int i = 0; i < __TEST_YIELDNUM; i++) {
    x = x;
    }

    printf(1, "[parent]: %d\n", getLevel());

    if (!strcmp(argv[1], "yield") && getLevel() == 2)
      yield(); 
  }

  wait();
  exit();
}