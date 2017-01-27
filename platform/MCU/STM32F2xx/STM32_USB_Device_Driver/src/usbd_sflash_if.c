/**
 ******************************************************************************
 * @file    usbd_sflash_if.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    26-Nov-2014
 * @brief   Specific media access Layer for serial flash.
 ******************************************************************************
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_sflash_if.h"
#include "usbd_dfu_mal.h"
#include "spi_flash.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
uint16_t sFLASH_If_Init(void);
uint16_t sFLASH_If_Erase (uint32_t Add);
uint16_t sFLASH_If_Write (uint32_t Add, uint32_t Len);
const uint8_t *sFLASH_If_Read  (uint32_t Add, uint32_t Len);
uint16_t sFLASH_If_Verify (uint32_t Add, uint32_t Len);
uint16_t sFLASH_If_DeInit(void);
uint16_t sFLASH_If_CheckAdd(uint32_t Add);

/* Private variables ---------------------------------------------------------*/
DFU_MAL_Prop_TypeDef DFU_sFlash_cb =
{
        sFLASH_IF_STRING,
        sFLASH_If_Init,
        sFLASH_If_DeInit,
        sFLASH_If_Erase,
        sFLASH_If_Write,
        sFLASH_If_Read,
        sFLASH_If_Verify,
        sFLASH_If_CheckAdd,
        0, /* Erase Time in ms */
        0  /* Programming Time in ms */
};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  sFLASH_If_Init
 *         Memory initialization routine.
 * @param  None
 * @retval MAL_OK if operation is successeful, MAL_FAIL else.
 */
uint16_t sFLASH_If_Init(void)
{
    sFLASH_Init();
    return MAL_OK;
}

/**
 * @brief  sFLASH_If_DeInit
 *         Memory deinitialization routine.
 * @param  None
 * @retval MAL_OK if operation is successeful, MAL_FAIL else.
 */
uint16_t sFLASH_If_DeInit(void)
{
    return MAL_OK;
}

/*******************************************************************************
 * Function Name  : sFLASH_If_Erase
 * Description    : Erase sector
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
uint16_t sFLASH_If_Erase(uint32_t Add)
{
    sFLASH_EraseSector(Add);
    return MAL_OK;
}

/**
 * @brief  sFLASH_If_Write
 *         Memory write routine.
 * @param  Add: Address to be written to.
 * @param  Len: Number of data to be written (in bytes).
 * @retval MAL_OK if operation is successeful, MAL_FAIL else.
 */
uint16_t sFLASH_If_Write(uint32_t Add, uint32_t Len)
{
    sFLASH_WriteBuffer(MAL_Buffer, Add, (uint16_t)Len);
    return MAL_OK;
}

/**
 * @brief  sFLASH_If_Read
 *         Memory read routine.
 * @param  Add: Address to be read from.
 * @param  Len: Number of data to be read (in bytes).
 * @retval Pointer to the phyisical address where data should be read.
 */
const uint8_t *sFLASH_If_Read (uint32_t Add, uint32_t Len)
{
    sFLASH_ReadBuffer(MAL_Buffer, Add, (uint16_t)Len);
    return MAL_Buffer;
}

/**
 * @brief  sFLASH_If_Verify
 *         Memory verify routine.
 * @param  Add: Address to be verified to.
 * @param  Len: Number of data to be verified (in bytes).
 * @retval MAL_OK if operation is successeful, MAL_FAIL else.
 */
uint16_t sFLASH_If_Verify(uint32_t Add, uint32_t Len)
{
    return MAL_OK; /* unimplemented, todo */
}

/**
 * @brief  sFLASH_If_CheckAdd
 *         Check if the address is an allowed address for this memory.
 * @param  Add: Address to be checked.
 * @param  Len: Number of data to be read (in bytes).
 * @retval MAL_OK if the address is allowed, MAL_FAIL else.
 */
uint16_t sFLASH_If_CheckAdd(uint32_t Add)
{
    if((Add >= sFLASH_START_ADD) && (Add < sFLASH_END_ADD))
    {
        return MAL_OK;
    }
    else
    {
        return MAL_FAIL;
    }
}
