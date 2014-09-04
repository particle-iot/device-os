#include "spark_wiring_random.h"
#include <stdlib.h>

int random(int max)
{
  if (0 == max) {
    return 0;
  }
  return rand() % max;
}

int random(int min, int max)
{
  if (min >= max) {
    return min;
  }
  return random(max - min) + min;
}

void randomSeed(unsigned int seed)
{
  srand(seed);
}

