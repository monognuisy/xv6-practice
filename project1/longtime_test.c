#include "types.h"
#include "stat.h"
#include "user.h"

#define __TEST_LONGNUM 500000

int
main(void)
{
  int x = 10;
  for (int i = 0; i < __TEST_LONGNUM; i++) {
    for (int j = 0; j < __TEST_LONGNUM; j++) {
      if (j & 1)
        x += 1;
      else 
        x -= 1;
    }

    if (!(i % 1000)) {
      printf(1, "nice: %d\n", i);
      printf(1, "level: %d\n", getLevel());
    }
  }

  exit();
}