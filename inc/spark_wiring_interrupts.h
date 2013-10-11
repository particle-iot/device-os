
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

extern "C" {
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
//void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
//void EXTI15_10_IRQHandler(void);
}

void userISRFunction_single(uint8_t intNumber);
void userISRFunction_multiple(uint8_t intNumStart, uint8_t intNUmEnd);



#endif /* SPARK_WIRING_INTERRUPTS_H_ */
