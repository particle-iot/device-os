/**
  ******************************************************************************
  * @file    dfu_mal.c
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   Generic media access Layer
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "dfu_mal.h"
#include "usb_lib.h"
#include "usb_type.h"
#include "usb_desc.h"
#include "flash_if.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint16_t (*pMAL_Init) (void);
uint16_t (*pMAL_Erase) (uint32_t SectorAddress);
uint16_t (*pMAL_Write) (uint32_t SectorAddress, uint32_t DataLength);
uint8_t  *(*pMAL_Read)  (uint32_t SectorAddress, uint32_t DataLength);
uint8_t  MAL_Buffer[wTransferSize]; /* RAM Buffer for Downloaded Data */

/* This table holds the Typical Sector Erase and 1024 Bytes Write timings.
   These timings will be returned to the host when it checks the device
   status during a write or erase operation to know how much time the host
   should wait before issuing the next get status request. 
   These defines are set in usb_conf.h file.
   The values of this table should be extracted from relative memories 
   datasheet (Typical or Maximum timming value for Sector Erase and for 
   1024 bytes Write). All timings are expressed in millisecond unit (ms).
   Note that "Sector" refers here to the memory unit used for Erase/Write 
   operations. It could be a sector, a page, a block, a word ...
   If the erase operation is not supported, it is advised to set the erase
   timing to 1 (which means 1ms: one USB frame). */
static const uint16_t  TimingTable[1][2] =
  { /*       Sector Erase time,            Sector Program time*/    
    { INTERN_FLASH_SECTOR_ERASE_TIME, INTERN_FLASH_SECTOR_WRITE_TIME }, /* Internal Flash */
  };

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Init(void)
{
  FLASH_If_Init(); /* Internal Flash */

  return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_Erase
* Description    : Erase sector
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Erase(uint32_t SectorAddress)
{
  switch (SectorAddress & MAL_MASK)
  {
    case INTERNAL_FLASH_BASE:
      pMAL_Erase = FLASH_If_Erase;
      break;
      
    default:
      return MAL_FAIL;
  }
  return pMAL_Erase(SectorAddress);
}

/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Write (uint32_t SectorAddress, uint32_t DataLength)
{
  switch (SectorAddress & MAL_MASK)
  {
    case INTERNAL_FLASH_BASE:
      pMAL_Write = FLASH_If_Write;
      break;

    default:
      return MAL_FAIL;
  }
  return pMAL_Write(SectorAddress, DataLength);
}

/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint8_t *MAL_Read (uint32_t SectorAddress, uint32_t DataLength)
{
  switch (SectorAddress & MAL_MASK)
  {
    case INTERNAL_FLASH_BASE:
      pMAL_Read = FLASH_If_Read;
      break;
      
    default:
      return 0;
  }
  return pMAL_Read (SectorAddress, DataLength);
}

/*******************************************************************************
* Function Name  : MAL_GetStatus
* Description    : Get status
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint16_t MAL_GetStatus(uint32_t SectorAddress , uint8_t Cmd, uint8_t *buffer)
{
  uint8_t x = 0;
  uint8_t y = Cmd & 0x01;
  
  SET_POLLING_TIMING(TimingTable[x][y]);  /* Media Erase/Write Timing */

  return MAL_OK;
}

/*********************** (C) COPYRIGHT STMicroelectronics *********************/
