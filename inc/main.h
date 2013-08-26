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

#include "hw_config.h"
#include "evnt_handler.h"
#include "hci.h"
#include "wlan.h"
#include "nvmem.h"
#include "socket.h"
#include "netapp.h"

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
#define SWD_JTAG_DISABLE

/*
 * Use Independent Watchdog to force a system reset when a software error occurs
 * During JTAG program/debug, the Watchdog has to be disabled so that it does not
 * upset the debugger
 */
#define IWDG_RESET_ENABLE

#endif

/* Flash Memory address where the System Flags will be saved and loaded from  */
#define SYSTEM_FLAGS_ADDRESS		((uint32_t)0x08004C00)

/* CC3000 EEPROM - Spark File Data Storage */
#define NVMEM_SPARK_FILE_ID			14	//Do not change this ID
#define NVMEM_SPARK_FILE_SIZE		16	//Change according to requirement
#define WLAN_PROFILE_FILE_OFFSET	0
#define ERROR_COUNT_FILE_OFFSET		1

#define USART_RX_DATA_SIZE			256	//2048

/* Exported functions ------------------------------------------------------- */
void Timing_Decrement(void);
void Delay(__IO uint32_t nTime);

void Load_SystemFlags(void);
void Save_SystemFlags(void);

void Set_NetApp_Timeout(void);
void Start_Smart_Config(void);

/* WLAN Application related callbacks passed to wlan_init */
void WLAN_Async_Callback(long lEventType, char *data, unsigned char length);
char *WLAN_Firmware_Patch(unsigned long *length);
char *WLAN_Driver_Patch(unsigned long *length);
char *WLAN_BootLoader_Patch(unsigned long *length);

void Start_OTA_Update(void);

void USB_USART_Init(uint32_t baudRate);
uint8_t USB_USART_Available_Data(void);
int32_t USB_USART_Receive_Data(void);
void USB_USART_Send_Data(uint8_t Data);
void Handle_USBAsynchXfer(void);
void Get_SerialNum(void);

#endif /* __MAIN_H */
