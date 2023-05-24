#include "types.h"
#include "user.h"
#include "stat.h"

#define NTHREADS 4

void*
thread_exit_test(void* arg)
{
  int val = (int)arg;
  printf(1, "Thread %d: Hello, world!\n", val);

  if (val == 0) {
    printf(1, "Executing demo program...\n");
    char* path = "/hellodemo";
    char* argv[10] = {[0] = path};

    exec(path, argv);
  }
  else;

  thread_exit(0);
  return 0;
}

int
main(void)
{
  thread_t threads[NTHREADS];
  int i;
  printf(1, "Thread exec test!\n");

  for (i = 0; i < NTHREADS; i++){
    thread_create(&threads[i], thread_exit_test, (void*)i);
  }

  for (i = 0; i < NTHREADS; i++) {
    thread_join(threads[i], 0);
  }

  // can't reach here!
  printf(1, "you cannot see this :)\n");
  exit();

  return 0;
}