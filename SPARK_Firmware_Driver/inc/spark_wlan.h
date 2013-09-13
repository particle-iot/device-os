/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPARK_WLAN_H
#define __SPARK_WLAN_H

#include "hw_config.h"
#include "evnt_handler.h"
#include "hci.h"
#include "wlan.h"
#include "nvmem.h"
#include "socket.h"
#include "netapp.h"
#include "security.h"

/* CC3000 EEPROM - Spark File Data Storage */
#define NVMEM_SPARK_FILE_ID			14	//Do not change this ID
#define NVMEM_SPARK_FILE_SIZE		16	//Change according to requirement
#define WLAN_PROFILE_FILE_OFFSET	0
#define WLAN_POLICY_FILE_OFFSET		1
#define WLAN_TIMEOUT_FILE_OFFSET	2
#define ERROR_COUNT_FILE_OFFSET		3

#define TIMING_SPARK_PROCESS_API	200		//200ms
#define TIMING_SPARK_ALIVE_TIMEOUT	15000	//15sec
#define TIMING_SPARK_RESET_TIMEOUT	30000	//30sec
#define TIMING_SPARK_OTA_TIMEOUT	180000	//180sec

#define SPARK_CONNECT_MAX_ATTEMPT	3		//Max no of connection attempts

void Set_NetApp_Timeout(void);
void Clear_NetApp_Dhcp(void);
void Start_Smart_Config(void);

/* WLAN Application related callbacks passed to wlan_init */
void WLAN_Async_Callback(long lEventType, char *data, unsigned char length);
char *WLAN_Firmware_Patch(unsigned long *length);
char *WLAN_Driver_Patch(unsigned long *length);
char *WLAN_BootLoader_Patch(unsigned long *length);

void SPARK_WLAN_Setup(void);
void SPARK_WLAN_Loop(void);
void SPARK_WLAN_Timing(void);

/* Spark Cloud APIs */
extern int Spark_Connect(void);
extern int Spark_Disconnect(void);
extern int Spark_Process_API_Response(void);

extern __IO uint32_t TimingSparkAliveTimeout;
extern __IO uint32_t TimingSparkOTATimeout;

extern __IO uint8_t SPARK_WLAN_SLEEP;
extern __IO uint8_t SPARK_SOCKET_CONNECTED;
extern __IO uint8_t SPARK_DEVICE_ACKED;
extern __IO uint8_t SPARK_FLASH_UPDATE;
extern __IO uint8_t SPARK_LED_FADE;

extern uint8_t WLAN_SMART_CONFIG_START;

extern __IO uint8_t Spark_Error_Count;

#endif  /*__SPARK_WLAN_H*/
