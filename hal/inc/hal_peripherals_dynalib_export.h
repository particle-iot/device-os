/* 
 * File:   hal_peripherals_dynalib_export.h
 * Author: mat
 *
 * Created on 16 February 2015, 08:04
 */

#ifndef HAL_PERIPHERALS_DYNALIB_EXPORT_H
#define	HAL_PERIPHERALS_DYNALIB_EXPORT_H

#ifdef	__cplusplus
extern "C" {
#endif

#define DYNALIB_EXPORT

#include "adc_hal.h"
#include "dac_hal.h"
#include "eeprom_hal.h"
#include "gpio_hal.h"
#include "i2c_hal.h"
#include "interrupts_hal.h"
#include "pwm_hal.h"
#include "servo_hal.h"
#include "spi_hal.h"
#include "tone_hal.h"
#include "usart_hal.h"

#include "hal_peripherals_dynalib.h"


#ifdef	__cplusplus
}
#endif

#endif	/* HAL_PERIPHERALS_DYNALIB_EXPORT_H */

