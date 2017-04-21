/**
  ******************************************************************************
  * @file    usbd_dfu_mal.c
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

/* Includes ------------------------------------------------------------------*/
#include "usbd_conf.h"
#include "usbd_dfu_mal.h"
#include "usbd_flash_if.h"
#include "usbd_dct_if.h"

#ifdef DFU_MAL_SUPPORT_sFLASH
#include "usbd_sflash_if.h"
#endif

#ifdef DFU_MAL_SUPPORT_OTP
 #include "usbd_otp_if.h"
#endif

#ifdef DFU_MAL_SUPPORT_MEM
 #include "usbd_mem_if_template.h"
#endif

#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Global Memories callback and string descriptors reference tables.
   To add a new memory, modify the value of MAX_USED_MEDIA in usbd_dfu_mal.h
   and add the pointer to the callback structure in this table.
   Then add the pointer to the memory string descriptor in usbd_dfu_StringDesc table.
   No other operation is required. */
DFU_MAL_Prop_TypeDef* tMALTab[MAX_USED_MEDIA] = {
    &DFU_Flash_cb,
    &DFU_DCT_cb
#ifdef DFU_MAL_SUPPORT_sFLASH
  , &DFU_sFlash_cb
#endif
#ifdef DFU_MAL_SUPPORT_OTP
  , &DFU_Otp_cb
#endif
#ifdef DFU_MAL_SUPPORT_MEM
  , &DFU_Mem_cb
#endif
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

__ALIGN_BEGIN const uint8_t* usbd_dfu_StringDesc[MAX_USED_MEDIA] __ALIGN_END  = {
    FLASH_IF_STRING,
    DCT_IF_STRING
#ifdef DFU_MAL_SUPPORT_sFLASH
  , sFLASH_IF_STRING
#endif
#ifdef DFU_MAL_SUPPORT_OTP
  , OTP_IF_STRING
#endif
#ifdef DFU_MAL_SUPPORT_MEM
  , MEM_IF_STRING
#endif
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* RAM Buffer for Downloaded Data */
__ALIGN_BEGIN uint8_t  MAL_Buffer[XFERSIZE] __ALIGN_END ;

/* Private function prototypes -----------------------------------------------*/
static uint8_t  MAL_CheckAdd  (uint32_t Idx, uint32_t Add);
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  MAL_Init
  *         Initializes the Media on the STM32
  * @param  None
  * @retval Result of the opeartion (MAL_OK in all cases)
  */
uint16_t MAL_Init(void)
{
  uint32_t memIdx = 0;

  /* Init all supported memories */
  for(memIdx = 0; memIdx < MAX_USED_MEDIA; memIdx++)
  {
    /* If the check addres is positive, exit with the memory index */
    if (tMALTab[memIdx]->pMAL_Init != NULL)
    {
      tMALTab[memIdx]->pMAL_Init();
    }
  }

  return MAL_OK;
}

/**
  * @brief  MAL_DeInit
  *         DeInitializes the Media on the STM32
  * @param  None
  * @retval Result of the opeartion (MAL_OK in all cases)
  */
uint16_t MAL_DeInit(void)
{
  uint32_t memIdx = 0;

  /* Init all supported memories */
  for(memIdx = 0; memIdx < MAX_USED_MEDIA; memIdx++)
  {
    /* Check if the command is supported */
    if (tMALTab[memIdx]->pMAL_DeInit != NULL)
    {
      tMALTab[memIdx]->pMAL_DeInit();
    }
  }

  return MAL_OK;
}

/**
  * @brief  MAL_Erase
  *         Erase a sector of memory.
  * @param  Add: Sector address/code
  * @retval Result of the opeartion: MAL_OK if all operations are OK else MAL_FAIL
  */
uint16_t MAL_Erase(uint32_t Idx, uint32_t Add)
{
  uint32_t memIdx = Idx;
  
  if(MAL_OK != MAL_CheckAdd(Idx, Add))
  {
    return MAL_FAIL;
  }

  /* Check if the area is protected */
  if (DFU_MAL_IS_PROTECTED_AREA(Add))
  {
    return MAL_FAIL;
  }

  if (memIdx < MAX_USED_MEDIA)
  {
    /* Check if the command is supported */
    if (tMALTab[memIdx]->pMAL_Erase != NULL)
    {
      return tMALTab[memIdx]->pMAL_Erase(Add);
    }
    else
    {
      return MAL_FAIL;
    }
  }
  else
  {
    return MAL_FAIL;
  }
}

/**
  * @brief  MAL_Write
  *         Write sectors of memory.
  *         Write is tried twice in case of a first time failure.
  *         After a successful write, data is verified.  If verification
  *         fails this sequence is tried once more before failing.
  * @param  Add: Sector address/code
  * @param  Len: Number of data to be written (in bytes)
  * @retval Result of the opeartion: MAL_OK if all operations are OK else MAL_FAIL
  */
uint16_t MAL_Write (uint32_t Idx, uint32_t Add, uint32_t Len)
{
  uint32_t memIdx = Idx;
  
  if(MAL_OK != MAL_CheckAdd(Idx, Add))
  {
    return MAL_FAIL;
  }

  /* Check if the area is protected */
  if (DFU_MAL_IS_PROTECTED_AREA(Add))
  {
    return MAL_FAIL;
  }

  if (memIdx < MAX_USED_MEDIA)
  {
    /* Check if the command is supported */
    if (tMALTab[memIdx]->pMAL_Write != NULL)
    {
      int8_t tries_remaining = 2;
      uint16_t status = MAL_FAIL;
      do {
        status = tMALTab[memIdx]->pMAL_Write(Add, Len);
        if (status != MAL_OK) {
          // Write failed, try once more.
          status = tMALTab[memIdx]->pMAL_Write(Add, Len);
          // If write failed twice, don't bother wasting time verifying and trying again.
          if (status != MAL_OK) {
            return MAL_FAIL;
          }
        }

        // Write was successful, let's verify now.
        status = tMALTab[memIdx]->pMAL_Verify(Add, Len);
        if (status == MAL_OK) {
          return MAL_OK;
        }
        // If verify failed, fall through and try write-verify one more time.

      } while (tries_remaining-- > 0);

      return MAL_FAIL;
    }
    else
    {
      return MAL_FAIL;
    }
  }
  else
  {
    return MAL_FAIL;
  }
}

/**
  * @brief  MAL_Read
  *         Read sectors of memory.
  * @param  Add: Sector address/code
  * @param  Len: Number of data to be written (in bytes)
  * @retval Buffer pointer
  */
const uint8_t* MAL_Read(uint32_t Idx, uint32_t Add, uint32_t Len)
{
  if (Idx < MAX_USED_MEDIA && tMALTab[Idx]->pMAL_Read != NULL && MAL_CheckAdd(Idx, Add) == MAL_OK)
  {
    const uint8_t* data = tMALTab[Idx]->pMAL_Read(Add, Len);
    if (data != NULL)
    {
      return data;
    }
  }
  // Fill DFU packet with zeros to make reading errors more apparent
  memset(MAL_Buffer, 0x00, XFERSIZE);
  return MAL_Buffer;
}

/**
  * @brief  MAL_GetStatus
  *         Get the status of a given memory.
  * @param  Add: Sector address/code (allow to determine which memory will be addressed)
  * @param  Cmd: 0 for erase and 1 for write
  * @param  buffer: pointer to the buffer where the status data will be stored.
  * @retval Buffer pointer
  */
uint16_t MAL_GetStatus(uint32_t Idx, uint32_t Add , uint8_t Cmd, uint8_t *buffer)
{
  uint32_t memIdx = Idx;
  
  if(MAL_OK != MAL_CheckAdd(Idx, Add))
  {
    return MAL_FAIL;
  }

  if (memIdx < MAX_USED_MEDIA)
  {
    if (Cmd & 0x01)
    {
      SET_POLLING_TIMING(tMALTab[memIdx]->EraseTiming);
    }
    else
    {
      SET_POLLING_TIMING(tMALTab[memIdx]->WriteTiming);
    }

    return MAL_OK;
  }
  else
  {
    return MAL_FAIL;
  }
}

/**
  * @brief  MAL_CheckAdd
  *         Determine which memory should be managed.
  * @param  Add: Sector address/code (allow to determine which memory will be addressed)
  * @retval Index of the addressed memory.
  */
static uint8_t MAL_CheckAdd(uint32_t Idx, uint32_t Add)
{
  uint32_t memIdx = Idx;

  if (tMALTab[memIdx]->pMAL_CheckAdd != NULL)
  {
    return tMALTab[memIdx]->pMAL_CheckAdd(Add);
  }
  
  return MAL_FAIL;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
