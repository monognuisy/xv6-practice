#include "types.h"
#include "user.h"
#include "stat.h"

void*
dosome(void* pa) 
{
  int a = *(int*)pa;
  for (int i = 0; i < 100; i++) {
    printf("%d\n", a);
  }
}

int
main(void)
{
  thread_t t1;
  int a = 10;
  thread_create(&t1, dosome, &a); 

  exit();
  return 0;
}