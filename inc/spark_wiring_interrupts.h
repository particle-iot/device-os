
#ifndef __SPARK_WIRING_INTERRUPTS_H
#define __SPARK_WIRING_INTERRUPTS_H

#include "spark_wiring.h"

/*
*Interrupts
*/

typedef enum InterruptMode {
  CHANGE,
  RISING,
  FALLING
} InterruptMode;

typedef void (*voidFuncPtr)(void);

void attachInterrupt(uint16_t pin, voidFuncPtr handler, InterruptMode mode);
void detachInterrupt(uint16_t pin);
void interrupts(void);
void noInterrupts(void);

#endif /* SPARK_WIRING_INTERRUPTS_H_ */
