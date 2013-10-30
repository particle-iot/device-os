/**
  ******************************************************************************
  * @file    spi_if.c
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    20-May-2013
  * @brief   specific media access Layer for SPI flash
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "spi_if.h"
#include "dfu_mal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : SPI_If_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t SPI_If_Init(void)
{
  sFLASH_Init();
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : SPI_If_Erase
* Description    : Erase sector
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t SPI_If_Erase(uint32_t SectorAddress)
{
  sFLASH_EraseSector(SectorAddress);
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : SPI_If_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t SPI_If_Write(uint32_t SectorAddress, uint32_t DataLength)
{
  sFLASH_WriteBuffer(MAL_Buffer, SectorAddress, (uint16_t)DataLength);
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : SPI_If_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : buffer address pointer
*******************************************************************************/
uint8_t *SPI_If_Read(uint32_t SectorAddress, uint32_t DataLength)
{
  sFLASH_ReadBuffer(MAL_Buffer, SectorAddress, (uint16_t)DataLength);
  return MAL_Buffer;
}

