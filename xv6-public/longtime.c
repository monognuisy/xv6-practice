#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM 10000

int
main(void)
{
  int x = 10;
  for (int i = 0; i < NUM; i++) {
    for (int j = 0; j < NUM; j++) {
      x += 1;
    }

    if (!(i % 100)) {
      printf(1, "nice: %d\n", x);
      printf(1, "level: %d\n", getLevel());
    }
  }

  exit();
}