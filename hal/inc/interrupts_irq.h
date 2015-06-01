/* 
 * File:   interrupts_irq.h
 * Author: mat1
 *
 * Created on May 26, 2015, 1:11 PM
 */

#ifndef INTERRUPTS_IRQ_H
#define	INTERRUPTS_IRQ_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum hal_irq_t {
#if defined(STM32F1XX)
    __All_irq = 0,
    SysInterrupt_SysTick = 1,
    SysInterrupt_TIM1_CC = 2,
    SysInterrupt_TIM2 = 3,
    SysInterrupt_TIM3 = 4,
    SysInterrupt_TIM4 = 5,
    __Last_irq = 6
#endif
    
#if defined(STM32F2XX)
    __All_irq = 0,
    SysInterrupt_SysTick = 1,
    SysInterrupt_TIM1_CC = 2,
    SysInterrupt_TIM2 = 3,
    SysInterrupt_TIM3 = 4,
    SysInterrupt_TIM4 = 5,
    SysInterrupt_TIM5 = 6,
    SysInterrupt_TIM6 = 7,
    SysInterrupt_TIM7 = 8,
    __Last_irq = 9
#endif    
            
    
} hal_irq_t;

#ifdef	__cplusplus
}
#endif

#endif	/* INTERRUPTS_IRQ_H */

