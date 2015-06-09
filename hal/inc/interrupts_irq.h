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
#if defined(STM32F10X_MD) || defined(STM32F10X_HD)
    __All_irq = 0,
    SysInterrupt_SysTick = 1,
    SysInterrupt_TIM1_CC = 2,
    SysInterrupt_TIM2 = 3,
    SysInterrupt_TIM3 = 4,
    SysInterrupt_TIM4 = 5,
    __Last_irq = 6
#elif defined(STM32F2XX)
    __All_irq = 0,
    SysInterrupt_SysTick,
    SysInterrupt_TIM1_CC,
    SysInterrupt_TIM2,
    SysInterrupt_TIM3,
    SysInterrupt_TIM4,
    SysInterrupt_TIM5,
    SysInterrupt_TIM6_DAC_IRQ,
    SysInterrupt_TIM6_Update,
    SysInterrupt_TIM7,
    SysInterrupt_TIM1_BRK_TIM9_IRQ,
    SysInterrupt_TIM1_Break,
    SysInterrupt_TIM9_Compare1,
    SysInterrupt_TIM9_Compare2,
    SysInterrupt_TIM9_Update,
    SysInterrupt_TIM9_Trigger,
    SysInterrupt_TIM1_UP_TIM10_IRQ,
    SysInterrupt_TIM1_Update,
    SysInterrupt_TIM10_Compare,
    SysInterrupt_TIM10_Update,
    SysInterrupt_TIM8_BRK_TIM12_IRQ,
    SysInterrupt_TIM8_Break,
    SysInterrupt_TIM12_Compare1,
    SysInterrupt_TIM12_Compare2,
    SysInterrupt_TIM12_Update,
    SysInterrupt_TIM12_Trigger,
    SysInterrupt_TIM8_UP_TIM13_IRQ,
    SysInterrupt_TIM8_Update,
    SysInterrupt_TIM13_Compare,
    SysInterrupt_TIM13_Update,
    SysInterrupt_TIM8_TRG_COM_TIM14_IRQ,
    SysInterrupt_TIM8_Trigger,
    SysInterrupt_TIM14_COM,
    SysInterrupt_TIM14_Compare,
    SysInterrupt_TIM14_Update,
    SysInterrupt_TIM8_IRQ,
    SysInterrupt_TIM8_Compare1,
    SysInterrupt_TIM8_Compare2,
    SysInterrupt_TIM8_Compare3,
    SysInterrupt_TIM1_TRG_COM_TIM11_IRQ,
    SysInterrupt_TIM1_Trigger,
    SysInterrupt_TIM1_COM,
    SysInterrupt_TIM11_Compare,
    SysInterrupt_TIM11_Update,

    __Last_irq = 45
#else
    __Last_irq = 0
#endif
} hal_irq_t;

#ifdef	__cplusplus
}
#endif

#endif	/* INTERRUPTS_IRQ_H */

