#include "types.h"
#include "user.h"

#define NTHREADS 4

// Entry point function for the lightweight threads
void* thread_entry(void* arg) {
  int thread_id = *((int*)arg);
  printf(1, "Thread %d: Hello, world!\n", thread_id);
  thread_exit((void*)(thread_id * 2)); // Exit with a return value

  return 0;
}

int main(void) {
  thread_t threads[NTHREADS];
  int thread_args[NTHREADS];
  void* thread_retval[NTHREADS];

  printf(1, "%d is func\n", thread_entry);

  // Create threads
  int i;
  for (i = 0; i < NTHREADS; i++) {
    thread_args[i] = i;
    if (thread_create(&threads[i], thread_entry, (void*)&thread_args[i]) < 0) {
      printf(1, "Thread creation failed!\n");
      exit();
    }
  }

  // Join threads
  for (i = 0; i < NTHREADS; i++) {
    if (thread_join(threads[i], &thread_retval[i]) < 0) {
      printf(1, "Thread join failed!\n");
      exit();
    }
    printf(1, "Joined Thread %d, Return Value: %d\n", i, (int)thread_retval[i]);
  }

  printf(1, "Main thread exiting.\n");
  exit();
}