/**
 ******************************************************************************
 * @file    usb_conf.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    21-Oct-2014
 * @brief   General low level driver configuration
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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
#ifndef __USB_CONF__H__
#define __USB_CONF__H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx.h"

/* NOTE: BM-09 uses USB_OTG_HS so comment defines related to USB_OTG_FS - Satish */

/* USB Core and PHY interface configuration.
   Tip: To avoid modifying these defines each time you need to change the USB
        configuration, you can declare the needed define in your toolchain
        compiler preprocessor.
 */
/****************** USB OTG FS PHY CONFIGURATION *******************************
 *  The USB OTG FS Core supports one on-chip Full Speed PHY.
 *
 *  The USE_EMBEDDED_PHY symbol is defined in the project compiler preprocessor
 *  when FS core is used.
 *******************************************************************************/
#ifndef USE_USB_OTG_FS
//#define USE_USB_OTG_FS
#endif /* USE_USB_OTG_FS */

#ifdef USE_USB_OTG_FS 
//#define USB_OTG_FS_CORE
#endif

/****************** USB OTG HS PHY CONFIGURATION *******************************
 *  The USB OTG HS Core supports two PHY interfaces:
 *   (i)  An ULPI interface for the external High Speed PHY: the USB HS Core will
 *        operate in High speed mode
 *   (ii) An on-chip Full Speed PHY: the USB HS Core will operate in Full speed mode
 *
 *  You can select the PHY to be used using one of these two defines:
 *   (i)  USE_ULPI_PHY: if the USB OTG HS Core is to be used in High speed mode
 *   (ii) USE_EMBEDDED_PHY: if the USB OTG HS Core is to be used in Full speed mode
 *
 *  Notes:
 *   - The USE_ULPI_PHY symbol is defined in the project compiler preprocessor as
 *     default PHY when HS core is used.
 *   - On STM322xG-EVAL and STM324xG-EVAL boards, only configuration(i) is available.
 *     Configuration (ii) need a different hardware, for more details refer to your
 *     STM32 device datasheet.
 *******************************************************************************/
#ifndef USE_USB_OTG_HS
#define USE_USB_OTG_HS
#endif /* USE_USB_OTG_HS */

#ifndef USE_ULPI_PHY
//#define USE_ULPI_PHY
#endif /* USE_ULPI_PHY */

#ifndef USE_EMBEDDED_PHY
#define USE_EMBEDDED_PHY
#endif /* USE_EMBEDDED_PHY */

#ifdef USE_USB_OTG_HS 
#define USB_OTG_HS_CORE
#endif

/*******************************************************************************
 *                      FIFO Size Configuration in Device mode
 *
 *  (i) Receive data FIFO size = RAM for setup packets +
 *                   OUT endpoint control information +
 *                   data OUT packets + miscellaneous
 *      Space = ONE 32-bits words
 *     --> RAM for setup packets = 10 spaces
 *        (n is the nbr of CTRL EPs the device core supports)
 *     --> OUT EP CTRL info      = 1 space
 *        (one space for status information written to the FIFO along with each
 *        received packet)
 *     --> data OUT packets      = (Largest Packet Size / 4) + 1 spaces
 *        (MINIMUM to receive packets)
 *     --> OR data OUT packets  = at least 2*(Largest Packet Size / 4) + 1 spaces
 *        (if high-bandwidth EP is enabled or multiple isochronous EPs)
 *     --> miscellaneous = 1 space per OUT EP
 *        (one space for transfer complete status information also pushed to the
 *        FIFO with each endpoint's last packet)
 *
 *  (ii)MINIMUM RAM space required for each IN EP Tx FIFO = MAX packet size for
 *       that particular IN EP. More space allocated in the IN EP Tx FIFO results
 *       in a better performance on the USB and can hide latencies on the AHB.
 *
 *  (iii) TXn min size = 16 words. (n  : Transmit FIFO index)
 *   (iv) When a TxFIFO is not used, the Configuration should be as follows:
 *       case 1 :  n > m    and Txn is not used    (n,m  : Transmit FIFO indexes)
 *       --> Txm can use the space allocated for Txn.
 *       case2  :  n < m    and Txn is not used    (n,m  : Transmit FIFO indexes)
 *       --> Txn should be configured with the minimum space of 16 words
 *  (v) The FIFO is used optimally when used TxFIFOs are allocated in the top
 *       of the FIFO.Ex: use EP1 and EP2 as IN instead of EP1 and EP3 as IN ones.
 *   (vi) In HS case12 FIFO locations should be reserved for internal DMA register
 *        so total FIFO size should be 1012 Only instead of 1024
 *******************************************************************************/

/****************** USB OTG HS CONFIGURATION **********************************/
#ifdef USB_OTG_HS_CORE
#define RX_FIFO_HS_SIZE                           512
#define TX0_FIFO_HS_SIZE                          500
#define TX1_FIFO_HS_SIZE                          0
#define TX2_FIFO_HS_SIZE                          0
#define TX3_FIFO_HS_SIZE                          0
#define TX4_FIFO_HS_SIZE                          0
#define TX5_FIFO_HS_SIZE                          0

/* #define USB_OTG_HS_SOF_OUTPUT_ENABLED */

#ifdef USE_ULPI_PHY
#define USB_OTG_ULPI_PHY_ENABLED
#endif
#ifdef USE_EMBEDDED_PHY
#define USB_OTG_EMBEDDED_PHY_ENABLED
/* wakeup is working only when HS core is configured in FS mode */
#define USB_OTG_HS_LOW_PWR_MGMT_SUPPORT
#endif
/* #define USB_OTG_HS_INTERNAL_DMA_ENABLED */ /*In High Speed mode, it is not possible to
                                                 use DMA for writing/reading to/from Flash/OTP memories
                                                 because the USB HS DMA is not connected (at product level)
                                                 to these memories (refer to RM0033 or RM0090 reference manual
                                                 for more details). 
                                                 If DMA mode is enabled, an intermediate buffer is used to 
                                                 interface between memory and DMA controller. This may result
                                                 in performance degradation for transfers relative to these 
                                                 memories in High Speed mode. It is advised to disable DMA 
                                                 mode if it is not used for other interfaces. */
/* #define USB_OTG_HS_DEDICATED_EP1_ENABLED */
#endif

/****************** USB OTG FS CONFIGURATION **********************************/
#ifdef USB_OTG_FS_CORE
#define RX_FIFO_FS_SIZE                           160
#define TX0_FIFO_FS_SIZE                          160
#define TX1_FIFO_FS_SIZE                          0
#define TX2_FIFO_FS_SIZE                          0
#define TX3_FIFO_FS_SIZE                          0

// #define USB_OTG_FS_LOW_PWR_MGMT_SUPPORT
// #define USB_OTG_FS_SOF_OUTPUT_ENABLED
#endif

/****************** USB OTG MISC CONFIGURATION ********************************/
//#define VBUS_SENSING_ENABLED  //OTG_HS_VBUS (PB13 pin) on BM-09 is not exposed

/****************** USB OTG MODE CONFIGURATION ********************************/
//#define USE_HOST_MODE
#define USE_DEVICE_MODE
//#define USE_OTG_MODE

#ifndef USB_OTG_FS_CORE
#ifndef USB_OTG_HS_CORE
#error  "USB_OTG_HS_CORE or USB_OTG_FS_CORE should be defined"
#endif
#endif

#ifndef USE_DEVICE_MODE
#ifndef USE_HOST_MODE
#error  "USE_DEVICE_MODE or USE_HOST_MODE should be defined"
#endif
#endif

#ifndef USE_USB_OTG_HS
#ifndef USE_USB_OTG_FS
#error  "USE_USB_OTG_HS or USE_USB_OTG_FS should be defined"
#endif
#else //USE_USB_OTG_HS
#ifndef USE_ULPI_PHY
#ifndef USE_EMBEDDED_PHY
#error  "USE_ULPI_PHY or USE_EMBEDDED_PHY should be defined"
#endif
#endif
#endif

/* Following defines needed in platform's usb driver */
#define __ALIGN_BEGIN
#define __ALIGN_END
#define __packed

#endif //__USB_CONF__H__

