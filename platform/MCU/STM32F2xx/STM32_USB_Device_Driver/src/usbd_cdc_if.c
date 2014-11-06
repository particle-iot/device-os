/**
 ******************************************************************************
 * @file    usbd_cdc_if.c
 * @author  MCD Application Team
 * @version V1.1.0
 * @date    19-March-2012
 * @brief   Generic media access Layer.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED 
#pragma     data_alignment = 4 
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

/* Includes ------------------------------------------------------------------*/
#include "usb_conf.h"
#include "usbd_conf.h"
#include "usbd_cdc_core.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* These are external variables imported from CDC core to be used for IN 
   transfer management. */
extern uint8_t  APP_Rx_Buffer []; /* Write CDC received data in this buffer.
                                     These data will be sent over USB IN endpoint
                                     in the CDC core functions. */
extern uint32_t APP_Rx_ptr_in;    /* Increment this pointer or roll it back to
                                     start address when writing received data
                                     in the buffer APP_Rx_Buffer. */

/* Private function prototypes -----------------------------------------------*/
static uint16_t APP_Init     (void);
static uint16_t APP_DeInit   (void);
static uint16_t APP_Ctrl     (uint32_t Cmd, uint8_t* Buf, uint32_t Len);
static uint16_t APP_DataTx   (uint8_t* Buf, uint32_t Len);
static uint16_t APP_DataRx   (uint8_t* Buf, uint32_t Len);

CDC_IF_Prop_TypeDef APP_fops =
{
        APP_Init,
        APP_DeInit,
        APP_Ctrl,
        APP_DataTx,
        APP_DataRx
};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  APP_Init
 *         Initializes the CDC APP low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_Init(void)
{
    //Do Nothing
    return USBD_OK;
}

/**
 * @brief  APP_DeInit
 *         DeInitializes the CDC APP low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_DeInit(void)
{
    //Do Nothing
    return USBD_OK;
}

/**
 * @brief  APP_Ctrl
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{ 
    //Do Nothing
    return USBD_OK;
}

/**
 * @brief  APP_DataTx
 *         CDC received data to be send over USB IN endpoint are managed in
 *         this function.
 * @param  Buf: Buffer of data to be sent
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_DataTx (uint8_t* Buf, uint32_t Len)
{
    //Do Nothing
    return USBD_OK;
}

/**
 * @brief  APP_DataRx
 *         Data received over USB OUT endpoint are sent over CDC interface
 *         through this function.
 *
 *         @note
 *         This function will block any OUT packet reception on USB endpoint
 *         untill exiting this function. If you exit this function before transfer
 *         is complete on CDC interface (ie. using DMA controller) it will result
 *         in receiving more data while previous ones are still not sent.
 *
 * @param  Buf: Buffer of data to be received
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_DataRx (uint8_t* Buf, uint32_t Len)
{
    //Do Nothing
    return USBD_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
