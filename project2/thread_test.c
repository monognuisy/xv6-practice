#include "types.h"
#include "user.h"
#include "stat.h"

void*
dosome(void* pa) 
{
  printf(1, "wow1\n");
  int a = *(int*)pa;
  for (int i = 0; i < 100; i++) {
    printf(1, "%d\n", a);
  }

  return 0;
}

int
main(void)
{
  thread_t t1;
  int a = 10;

  printf(1, "what?\n");
  thread_create(&t1, dosome, &a); 

  exit();
  return 0;
}