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
/* Flash Memory address where the System Flags will be saved and loaded from  */
#define SYSTEM_FLAGS_ADDRESS	((uint32_t)0x08004C00)

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

#endif /* __MAIN_H */
