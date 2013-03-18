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
#include "cc3000_common.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

/* Wifi Application related callbacks passed to wlan_init */
void CC3000_Async_Callback(long lEventType, char *data, unsigned char length);
char *Send_Firmware_Patch(unsigned long *length);
char *Send_Driver_Patch(unsigned long *length);
char *Send_BootLoader_Patch(unsigned long *length);

#endif /* __MAIN_H */
