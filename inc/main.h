/**
 ******************************************************************************
 * @file    main.h
 * @author  Spark Application Team
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for main.c module
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif
#include "hw_config.h"
#include "spark_wlan.h"
#ifdef __cplusplus
}
#endif

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/*
 * In Eclipse Project Properties -> C/C++ Build -> Settings -> Tool Settings
 * -> ARM Sourcery Windows GCC C/C++ Compiler -> Preprocessor -> Defined symbol (-D),
 * Add : "DFU_BUILD_ENABLE"
 *
 * In Eclipse Project Properties -> C/C++ Build -> Settings -> Tool Settings
 * -> ARM Sourcery Windows GCC C/C++ Linker -> General -> Script file (-T),
 * Browse & select linker file : "linker_stm32f10x_md_dfu.ld"
 */

#ifdef DFU_BUILD_ENABLE

/*
 * Use the JTAG IOs as standard GPIOs (D3 to D7)
 * Note that once the JTAG IOs are disabled, the connection with the host debugger
 * is lost and cannot be re-established as long as the JTAG IOs remain disabled.
 */
//#define SWD_JTAG_DISABLE

/*
 * Use Independent Watchdog to force a system reset when a software error occurs
 * During JTAG program/debug, the Watchdog has to be disabled so that it does not
 * upset the debugger
 */
//#define IWDG_RESET_ENABLE
#define TIMING_IWDG_RELOAD	1000 //1sec

#endif

#define USART_RX_DATA_SIZE			256

/* Exported functions ------------------------------------------------------- */
void Timing_Decrement(void);

void USB_USART_Init(uint32_t baudRate);
uint8_t USB_USART_Available_Data(void);
int32_t USB_USART_Receive_Data(void);
void USB_USART_Send_Data(uint8_t Data);
void Handle_USBAsynchXfer(void);
void Get_SerialNum(void);

#endif /* __MAIN_H */
