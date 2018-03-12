/*
 * @brief LPC407x/8x basic chip inclusion file
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef __CHIP_LPC407X_8X_H_
#define __CHIP_LPC407X_8X_H_

#include "lpc_types.h"
#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(CORE_M4)
#error CORE_M4 is not defined for the LPC407x/8x architecture
#error CORE_M4 should be defined as part of your compiler define list
#endif

#ifndef CHIP_LPC407X_8X
#error CHIP_LPC407X_8X is not defined!
#endif

/** @defgroup IP_LPC407X_8X_FILES CHIP: LPC407X_8X Chip layer required IP layer drivers
 * @ingroup CHIP_17XX_40XX_Drivers
 * This is a list of the IP drivers required for the LPC407X_8X device family.<br>
 * (adc_001.c, adc_001.h) @ref IP_ADC_001<br>
 * (can_001.c, can_001.h) @ref IP_CAN_001<br>
 * (dac_001.c, dac_001.h) @ref IP_DAC_001<br>
 * (emc_001.c, emc_001.h) @ref IP_EMC_001<br>
 * (enet_002.c, enet_002.h) @ref IP_ENET_002<br>
 * (gpdma_001.c, gpdma_001.h) @ref IP_GPDMA_001<br>
 * (gpio_002.h) @ref IP_GPIO_002<br>
 * (i2c_001.c, i2c_001.h) @ref IP_I2C_001<br>
 * (i2s_001.c, i2s_001.h) @ref IP_I2S_001<br>
 * (lcd_001.c, lcd_001.h) @ref IP_LCD_001<br>
 * (mcpwm_001.h) @ref IP_MCPWM_001<br>
 * (qei_001.h) @ref IP_QEI_001<br>
 * (regfile_001.h) @ref IP_REGFILE_001<br>
 * (rtc_001.c, rtc_001.h) @ref IP_RTC_001<br>
 * (sdc_001.c, sdc_001.h) @ref IP_SDC_001<br>
 * (ssp_001.c, ssp_001.h) @ref IP_SSP_001<br>
 * (timer_001.c, timer_001.h) @ref IP_TIMER_001<br>
 * (usart_001.c, usart_001.h) @ref IP_USART_001<br>
 * (wwdt_001.c, wwdt_001.h) @ref IP_WWDT_001<br>
 * (cmp_001.h) @ref IP_CMP_001<br>
 * (usb_001.h) @ref IP_USB_001<br>
 * (eeprom_001.c, eeprom_001.h) @ref IP_EEPROM_001<br>
 * (crc_001.c, crc_001.h) @ref IP_CRC_001<br>
 * @{
 */

/**
 * @}
 */


#include "adc_001.h"
#include "can_001.h"
#include "dac_001.h"
#include "emc_001.h"
#include "enet_002.h"
#include "gpdma_001.h"
#include "gpio_002.h"
#include "i2c_001.h"
#include "i2s_001.h"
#include "lcd_001.h"
#include "mcpwm_001.h"
#include "qei_001.h"
#include "regfile_001.h"
#include "rtc_001.h"
#include "sdc_001.h"
#include "ssp_001.h"
#include "timer_001.h"
#include "usart_001.h"
#include "wwdt_001.h"
#include "cmp_001.h"
#include "usb_001.h"
#include "eeprom_001.h"
#include "crc_001.h"

/** @defgroup PERIPH_407X_8X_BASE CHIP: LPC407x/8x Peripheral addresses and register set declarations
 * @ingroup CHIP_17XX_40XX_Drivers
 * @{
 */

#define LPC_EEPROM_BASE           0x00200080
#define LPC_GPDMA_BASE            0x20080000
#define LPC_ENET_BASE             0x20084000
#define LPC_LCD_BASE              0x20088000
#define LPC_USB_BASE              0x2008C000
#define LPC_CRC_BASE              0x20090000
#define LPC_SPIFI_BASE            0x20094000
#define LPC_GPIO0_BASE            0x20098000
#define LPC_GPIO1_BASE            0x20098020
#define LPC_GPIO2_BASE            0x20098040
#define LPC_GPIO3_BASE            0x20098060
#define LPC_GPIO4_BASE            0x20098080
#define LPC_GPIO5_BASE            0x200980A0
#define LPC_EMC_BASE              0x2009C000
#define LPC_RTC_BASE              0x40024000
#define LPC_REGFILE_BASE          0x40024044
#define LPC_WWDT_BASE             0x40000000
#define LPC_UART0_BASE            0x4000C000
#define LPC_UART1_BASE            0x40010000
#define LPC_UART2_BASE            0x40098000
#define LPC_UART3_BASE            0x4009C000
#define LPC_UART4_BASE            0x400A4000
#define LPC_SSP0_BASE             0x40088000
#define LPC_SSP1_BASE             0x40030000
#define LPC_SSP2_BASE             0x400AC000
#define LPC_TIMER0_BASE           0x40004000
#define LPC_TIMER1_BASE           0x40008000
#define LPC_TIMER2_BASE           0x40090000
#define LPC_TIMER3_BASE           0x40094000
#define LPC_MCPWM_BASE            0x400B8000
#define LPC_PWM0_BASE             0x40014000
#define LPC_PWM1_BASE             0x40018000
#define LPC_I2C0_BASE             0x4001C000
#define LPC_I2C1_BASE             0x4005C000
#define LPC_I2C2_BASE             0x400A0000
#define LPC_I2S_BASE              0x400A8000
#define LPC_CANAF_RAM_BASE        0x40038000
#define LPC_CANAF_BASE            0x4003C000
#define LPC_CANCR_BASE            0x40040000
#define LPC_CAN1_BASE             0x40044000
#define LPC_CAN2_BASE             0x40048000
#define LPC_QEI_BASE              0x400BC000
#define LPC_DAC_BASE              0x4008C000
#define LPC_ADC_BASE              0x40034000
#define LPC_GPIOINT_BASE          0x40028080
#define LPC_IOCON_BASE            0x4002C000
#define LPC_SDC_BASE              0x400C0000
#define LPC_SYSCTL_BASE           0x400FC000
#define LPC_CMP_BASE              0x40020000

/* Normalize types */
typedef IP_EEPROM_001_T LPC_EEPROM_T;
typedef IP_GPDMA_001_T LPC_GPDMA_T;
typedef IP_SDC_001_T LPC_SDC_T;
typedef IP_EMC_001_T LPC_EMC_T;
typedef IP_USB_001_T LPC_USB_T;
typedef IP_ENET_002_T LPC_ENET_T;
typedef IP_REGFILE_001_T LPC_REGFILE_T;
typedef IP_RTC_001_T LPC_RTC_T;
typedef IP_WWDT_001_T LPC_WWDT_T;
typedef IP_USART_001_T LPC_USART_T;
typedef IP_SSP_001_T LPC_SSP_T;
typedef IP_TIMER_001_T LPC_TIMER_T;
typedef IP_MCPWM_001_T LPC_MCPWM_T;
typedef IP_I2C_001_T LPC_I2C_T;
typedef IP_I2S_001_T LPC_I2S_T;
typedef IP_QEI_001_T LPC_QEI_T;
typedef IP_DAC_001_T LPC_DAC_T;
typedef IP_ADC_001_T LPC_ADC_T;
typedef IP_GPIO_002_T LPC_GPIO_T;
typedef IP_LCD_001_T LPC_LCD_T;
typedef IP_CAN_001_AF_RAM_T LPC_CANAF_RAM_T;
typedef IP_CAN_001_AF_T LPC_CANAF_T;
typedef IP_CAN_001_CR_T LPC_CANCR_T;
typedef IP_CAN_001_T LPC_CAN_T;
typedef IP_CRC_001_T LPC_CRC_T;

/* Assign LPC_* names to structures mapped to addresses */
#define LPC_EEPROM                ((IP_EEPROM_001_T              *) LPC_EEPROM_BASE)
#define LPC_GPDMA                 ((IP_GPDMA_001_T            *) LPC_GPDMA_BASE)
#define LPC_EMC                   ((IP_EMC_001_T              *) LPC_EMC_BASE)
#define LPC_USB                   ((IP_USB_001_T              *) LPC_USB_BASE)
#define LPC_LCD                   ((IP_LCD_001_T              *) LPC_LCD_BASE)
#define LPC_ETHERNET              ((IP_ENET_002_T             *) LPC_ENET_BASE)
#define LPC_GPIO                  ((IP_GPIO_002_T             *) LPC_GPIO0_BASE)
#define LPC_GPIO1                 ((IP_GPIO_002_T              *) LPC_GPIO1_BASE)
#define LPC_GPIO2                 ((IP_GPIO_002_T             *) LPC_GPIO2_BASE)
#define LPC_GPIO3                 ((IP_GPIO_002_T             *) LPC_GPIO3_BASE)
#define LPC_GPIO4                 ((IP_GPIO_002_T             *) LPC_GPIO4_BASE)
#define LPC_GPIO5                 ((IP_GPIO_002_T             *) LPC_GPIO5_BASE)
#define LPC_RTC                   ((IP_RTC_001_T              *) LPC_RTC_BASE)
#define LPC_REGFILE               ((IP_REGFILE_001_T          *) LPC_REGFILE_BASE)
#define LPC_WWDT                  ((IP_WWDT_001_T             *) LPC_WWDT_BASE)
#define LPC_UART0                 ((IP_USART_001_T            *) LPC_UART0_BASE)
#define LPC_UART1                 ((IP_USART_001_T            *) LPC_UART1_BASE)
#define LPC_UART2                 ((IP_USART_001_T            *) LPC_UART2_BASE)
#define LPC_UART3                 ((IP_USART_001_T            *) LPC_UART3_BASE)
#define LPC_UART4                 ((IP_USART_001_T            *) LPC_UART4_BASE)
#define LPC_SSP0                  ((IP_SSP_001_T              *) LPC_SSP0_BASE)
#define LPC_SSP1                  ((IP_SSP_001_T              *) LPC_SSP1_BASE)
#define LPC_SSP2                  ((IP_SSP_001_T              *) LPC_SSP2_BASE)
#define LPC_TIMER0                ((IP_TIMER_001_T            *) LPC_TIMER0_BASE)
#define LPC_TIMER1                ((IP_TIMER_001_T            *) LPC_TIMER1_BASE)
#define LPC_TIMER2                ((IP_TIMER_001_T            *) LPC_TIMER2_BASE)
#define LPC_TIMER3                ((IP_TIMER_001_T            *) LPC_TIMER3_BASE)
#define LPC_MCPWM                 ((IP_MCPWM_001_T            *) LPC_MCPWM_BASE)
#define LPC_I2C0                  ((IP_I2C_001_T              *) LPC_I2C0_BASE)
#define LPC_I2C1                  ((IP_I2C_001_T              *) LPC_I2C1_BASE)
#define LPC_I2C2                  ((IP_I2C_001_T              *) LPC_I2C2_BASE)
#define LPC_I2S                   ((IP_I2S_001_T              *) LPC_I2S_BASE)
#define LPC_QEI                   ((IP_QEI_001_T              *) LPC_QEI_BASE)
#define LPC_DAC                   ((IP_DAC_001_T              *) LPC_DAC_BASE)
#define LPC_ADC                   ((IP_ADC_001_T              *) LPC_ADC_BASE)
#define LPC_IOCON                 ((LPC_IOCON_T               *) LPC_IOCON_BASE)
#define LPC_SDC                   ((IP_SDC_001_T              *) LPC_SDC_BASE)
#define LPC_SYSCTL                ((LPC_SYSCTL_T              *) LPC_SYSCTL_BASE)
#define LPC_CMP                   ((IP_CMP_001_T              *) LPC_CMP_BASE)
#define LPC_CANAF_RAM             ((LPC_CANAF_RAM_T           *) LPC_CANAF_RAM_BASE)
#define LPC_CANAF                 ((LPC_CANAF_T               *) LPC_CANAF_BASE)
#define LPC_CANCR                 ((LPC_CANCR_T               *) LPC_CANCR_BASE)
#define LPC_CAN1                  ((LPC_CAN_T                 *) LPC_CAN1_BASE)
#define LPC_CAN2                  ((LPC_CAN_T                 *) LPC_CAN2_BASE)
#define LPC_CRC                   ((LPC_CRC_T                 *) LPC_CRC_BASE)

/**
 * @}
 */

#include "eeprom_17xx_40xx.h"
#include "gpio_17xx_40xx.h"
#include "uart_17xx_40xx.h"
#include "gpdma_17xx_40xx.h"
#include "i2c_17xx_40xx.h"
#include "i2s_17xx_40xx.h"
#include "ssp_17xx_40xx.h"
#include "rtc_17xx_40xx.h"
#include "emc_17xx_40xx.h"
#include "lcd_17xx_40xx.h"
#include "adc_17xx_40xx.h"
#include "dac_17xx_40xx.h"
#include "timer_17xx_40xx.h"
#include "iocon_17xx_40xx.h"
#include "sysctl_17xx_40xx.h"
#include "clock_17xx_40xx.h"
#include "fmc_17xx_40xx.h"
#include "can_17xx_40xx.h"
#include "crc_17xx_40xx.h"
#include "enet_17xx_40xx.h"
#include "sdc_17xx_40xx.h"
#include "sdmmc_17xx_40xx.h"
#include "wwdt_17xx_40xx.h"
#include "cmp_17xx_40xx.h"
#include "spifi_17xx_40xx.h"
#include "fpu_init.h"

/* FIXME - missing PWM and possibly CREG drivers */

#ifdef __cplusplus
}
#endif

#endif /* __CHIP_LPC407X_8X_H_ */
