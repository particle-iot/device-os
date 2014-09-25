/* 
 * File:   pinmap_impl.h
 * Author: mat
 *
 * Created on 25 September 2014, 01:32
 */

#ifndef PINMAP_IMPL_H
#define	PINMAP_IMPL_H

#include "stm32f10x.h"


#ifdef	__cplusplus
extern "C" {
#endif

typedef struct STM32_Pin_Info {
  GPIO_TypeDef* gpio_peripheral;
  pin_t gpio_pin;
  uint8_t adc_channel;
  TIM_TypeDef* timer_peripheral;
  uint16_t timer_ch;
  PinMode pin_mode;
  uint16_t timer_ccr;
  int32_t user_property;
} STM32_Pin_Info;

/* Exported constants --------------------------------------------------------*/

extern STM32_Pin_Info PIN_MAP[];

extern void HAL_GPIO_Save_Pin_Mode(PinMode mode);
extern PinMode HAL_GPIO_Recall_Pin_Mode();

#define NONE ((uint8_t)0xFF)
#define ADC_CHANNEL_NONE NONE

#ifdef	__cplusplus
}
#endif

#endif	/* PINMAP_IMPL_H */

