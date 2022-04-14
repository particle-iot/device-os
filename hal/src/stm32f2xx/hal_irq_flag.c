#include "stm32f2xx.h"

int HAL_disable_irq()
{
  int is = __get_PRIMASK();
  __disable_irq();
  return is;
}

void HAL_enable_irq(int is) {
    if ((is & 1) == 0) {
        __enable_irq();
    }
}
