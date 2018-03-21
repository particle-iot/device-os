/**
  ******************************************************************************
  * @file    usbd_flash_if.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   Specific media access Layer for internal flash.
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
#include "usbd_flash_if.h"
#include "usbd_dfu_mal.h"
#include "usb_bsp.h"
#include "hw_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
uint16_t FLASH_If_Init(void);
uint16_t FLASH_If_Erase (uint32_t Add);
uint16_t FLASH_If_Write (uint32_t Add, uint32_t Len);
const uint8_t *FLASH_If_Read  (uint32_t Add, uint32_t Len);
uint16_t FLASH_If_Verify  (uint32_t Add, uint32_t Len);
uint16_t FLASH_If_DeInit(void);
uint16_t FLASH_If_CheckAdd(uint32_t Add);


/* Private variables ---------------------------------------------------------*/
DFU_MAL_Prop_TypeDef DFU_Flash_cb =
  {
    FLASH_IF_STRING,
    FLASH_If_Init,
    FLASH_If_DeInit,
    FLASH_If_Erase,
    FLASH_If_Write,
    FLASH_If_Read,
    FLASH_If_Verify,
    FLASH_If_CheckAdd,
    0, /* Erase Time in ms */
    0  /* Programming Time in ms */
  };

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  FLASH_If_Init
  *         Memory initialization routine.
  * @param  None
  * @retval MAL_OK if operation is successeful, MAL_FAIL else.
  */
uint16_t FLASH_If_Init(void)
{
  return MAL_OK;
}

/**
  * @brief  FLASH_If_DeInit
  *         Memory deinitialization routine.
  * @param  None
  * @retval MAL_OK if operation is successeful, MAL_FAIL else.
  */
uint16_t FLASH_If_DeInit(void)
{
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : FLASH_If_Erase
* Description    : Erase sector
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t FLASH_If_Erase(uint32_t Add)
{
  /* Unlock the internal flash */
  FLASH_Unlock();

  FLASH_ClearFlags();

#if defined (STM32F2XX) || defined (STM32F4XX)
  /* Disable sector write protection if it's already protected */
  uint16_t OB_WRP_Sector = FLASH_SectorToWriteProtect(FLASH_INTERNAL, Add);
  FLASH_WriteProtection_Disable(OB_WRP_Sector);

  /* Check which sector has to be erased */
  uint16_t FLASH_Sector = FLASH_SectorToErase(FLASH_INTERNAL, Add);
  FLASH_Status status = FLASH_EraseSector(FLASH_Sector, VoltageRange_3);

#elif defined(STM32F10X_CL)
  /* Call the standard Flash erase function */
  FLASH_ErasePage(Add);
#endif /* STM32F2XX */

  /* Lock the internal flash */
  FLASH_Lock();

  if (status != FLASH_COMPLETE) return MAL_FAIL;

  return MAL_OK;
}

/**
  * @brief  FLASH_If_Write
  *         Memory write routine.
  * @param  Add: Address to be written to.
  * @param  Len: Number of data to be written (in bytes).
  * @retval MAL_OK if operation is successeful, MAL_FAIL else.
  */
uint16_t FLASH_If_Write(uint32_t Add, uint32_t Len)
{
  uint32_t idx = 0;

  if  (Len & 0x3) /* Not an aligned data */
  {
    for (idx = Len; idx < ((Len & 0xFFFC) + 4); idx++)
    {
      MAL_Buffer[idx] = 0xFF;
    }
  }

  /* Unlock the internal flash */
  FLASH_Unlock();

  FLASH_ClearFlags();

  /* Data received are Word multiple */
  FLASH_Status status = FLASH_COMPLETE;
  for (idx = 0; idx < Len && status == FLASH_COMPLETE; idx = idx + 4)
  {
    status = FLASH_ProgramWord(Add, *(uint32_t *)(MAL_Buffer + idx));
    Add += 4;
  }

  /* Lock the internal flash */
  FLASH_Lock();

  if (status != FLASH_COMPLETE) return MAL_FAIL;

  return MAL_OK;
}

/**
  * @brief  FLASH_If_Read
  *         Memory read routine.
  * @param  Add: Address to be read from.
  * @param  Len: Number of data to be read (in bytes).
  * @retval Pointer to the phyisical address where data should be read.
  */
const uint8_t *FLASH_If_Read (uint32_t Add, uint32_t Len)
{
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  uint32_t idx = 0;
  for (idx = 0; idx < Len; idx += 4)
  {
    *(uint32_t*)(MAL_Buffer + idx) = *(uint32_t *)(Add + idx);
  }
  return (uint8_t*)(MAL_Buffer);
#else
  return  (const uint8_t *)(Add);
#endif // USB_OTG_HS_INTERNAL_DMA_ENABLED
}

/**
  * @brief  FLASH_If_Verify
  *         Memory verify routine. Assumes FLASH_If_Write was
  *         called just prior, with same Add and Len.
  * @param  Add: Address to be read/verified from.
  * @param  Len: Number of data to be read/verified (in bytes).
  * @retval MAL_OK if operation is successeful, MAL_FAIL else.
  */
uint16_t FLASH_If_Verify (uint32_t Add, uint32_t Len)
{
  uint16_t status = MAL_OK;
  uint32_t idx = 0;
  for (idx = 0; idx < Len; idx += 4)
  {
    if ( (*(uint32_t*)(MAL_Buffer + idx)) != (*(uint32_t *)(Add + idx)) )
    {
      status = MAL_FAIL;
      break;
    }
  }
  return status;
}

/**
  * @brief  FLASH_If_CheckAdd
  *         Check if the address is an allowed address for this memory.
  * @param  Add: Address to be checked.
  * @param  Len: Number of data to be read (in bytes).
  * @retval MAL_OK if the address is allowed, MAL_FAIL else.
  */
uint16_t FLASH_If_CheckAdd(uint32_t Add)
{
  if ((Add >= FLASH_START_ADD) && (Add < FLASH_END_ADD))
  {
    return MAL_OK;
  }
  else
  {
    return MAL_FAIL;
  }
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
