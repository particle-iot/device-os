/**
  ******************************************************************************
  * @file    usb_prop.h
  * @author  Satish Nair
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   All processing related to DFU
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_PROP_H
#define __USB_PROP_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void DFU_init(void);
void DFU_Reset(void);
void DFU_SetConfiguration(void);
void DFU_SetDeviceAddress (void);
void DFU_Status_In (void);
void DFU_Status_Out (void);
RESULT DFU_Data_Setup(uint8_t);
RESULT DFU_NoData_Setup(uint8_t);
RESULT DFU_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *DFU_GetDeviceDescriptor(uint16_t );
uint8_t *DFU_GetConfigDescriptor(uint16_t);
uint8_t *DFU_GetStringDescriptor(uint16_t);
uint8_t *UPLOAD(uint16_t Length);
uint8_t *DNLOAD(uint16_t Length);
uint8_t *GETSTATE(uint16_t Length);
uint8_t *GETSTATUS(uint16_t Length);
void DFU_write_crc (void);

/* External variables --------------------------------------------------------*/

#define DFU_GetConfiguration          NOP_Process
//#define DFU_SetConfiguration          NOP_Process
#define DFU_GetInterface              NOP_Process
#define DFU_SetInterface              NOP_Process
#define DFU_GetStatus                 NOP_Process
#define DFU_ClearFeature              NOP_Process
#define DFU_SetEndPointFeature        NOP_Process
#define DFU_SetDeviceFeature          NOP_Process
//#define DFU_SetDeviceAddress          NOP_Process

/*---------------------------------------------------------------------*/
/*  DFU definitions                                                    */
/*---------------------------------------------------------------------*/

/**************************************************/
/* DFU Requests                                   */
/**************************************************/

typedef enum _DFU_REQUESTS {
  DFU_DNLOAD = 1,
  DFU_UPLOAD,
  DFU_GETSTATUS,
  DFU_CLRSTATUS,
  DFU_GETSTATE,
  DFU_ABORT
} DFU_REQUESTS;

/**************************************************/
/* DFU Requests  DFU states                       */
/**************************************************/


#define STATE_appIDLE                 0
#define STATE_appDETACH               1
#define STATE_dfuIDLE                 2
#define STATE_dfuDNLOAD_SYNC          3
#define STATE_dfuDNBUSY               4
#define STATE_dfuDNLOAD_IDLE          5
#define STATE_dfuMANIFEST_SYNC        6
#define STATE_dfuMANIFEST             7
#define STATE_dfuMANIFEST_WAIT_RESET  8
#define STATE_dfuUPLOAD_IDLE          9
#define STATE_dfuERROR                10

/**************************************************/
/* DFU Requests  DFU status                       */
/**************************************************/

#define STATUS_OK                   0x00
#define STATUS_ERRTARGET            0x01
#define STATUS_ERRFILE              0x02
#define STATUS_ERRWRITE             0x03
#define STATUS_ERRERASE             0x04
#define STATUS_ERRCHECK_ERASED      0x05
#define STATUS_ERRPROG              0x06
#define STATUS_ERRVERIFY            0x07
#define STATUS_ERRADDRESS           0x08
#define STATUS_ERRNOTDONE           0x09
#define STATUS_ERRFIRMWARE          0x0A
#define STATUS_ERRVENDOR            0x0B
#define STATUS_ERRUSBR              0x0C
#define STATUS_ERRPOR               0x0D
#define STATUS_ERRUNKNOWN           0x0E
#define STATUS_ERRSTALLEDPKT        0x0F

/**************************************************/
/* DFU Requests  DFU states Manifestation State   */
/**************************************************/

#define Manifest_complete           0x00
#define Manifest_In_Progress        0x01


/**************************************************/
/* Special Commands  with Download Request        */
/**************************************************/

#define CMD_GETCOMMANDS              0x00
#define CMD_SETADDRESSPOINTER        0x21
#define CMD_ERASE                    0x41

#endif /* __USB_PROP_H */

