/**
  ******************************************************************************
  * @file    usbd_dct_if.c
  * @author  Matthew McGowan
  * @version V1.1.0
  * @brief   Specific media access Layer for DCT interface.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_dct_if.h"
#include "usbd_dfu_mal.h"
#include "dct.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

uint16_t DCT_If_Init(void);
uint16_t DCT_If_Erase (uint32_t Add);
uint16_t DCT_If_Write (uint32_t Add, uint32_t Len);
const uint8_t *DCT_If_Read  (uint32_t Add, uint32_t Len);
uint16_t DCT_If_Verify (uint32_t Add, uint32_t Len);
uint16_t DCT_If_DeInit(void);
uint16_t DCT_If_CheckAdd(uint32_t Add);


DFU_MAL_Prop_TypeDef DFU_DCT_cb =
  {
    DCT_IF_STRING,
    DCT_If_Init,
    DCT_If_DeInit,
    DCT_If_Erase,
    DCT_If_Write,
    DCT_If_Read,
    DCT_If_Verify,
    DCT_If_CheckAdd,
    1, /* Erase Time in ms */
    500  /* Programming Time in ms */
  };


uint16_t DCT_If_Init(void) {
    return MAL_OK;
}

uint16_t DCT_If_Erase (uint32_t Add) {
    return MAL_OK;
}

uint16_t DCT_If_Write (uint32_t Add, uint32_t Len) {
    return dct_write_app_data(MAL_Buffer, Add, Len) ? MAL_FAIL : MAL_OK;
}

const uint8_t *DCT_If_Read  (uint32_t Add, uint32_t Len) {
    return dct_read_app_data(Add);
}

uint16_t DCT_If_Verify (uint32_t Add, uint32_t Len) {
    return MAL_OK; /* unimplemented, todo */
}

uint16_t DCT_If_DeInit(void) {
    return MAL_OK;
}

uint16_t DCT_If_CheckAdd(uint32_t Add) {
    return (Add<0x4000) ? MAL_OK : MAL_FAIL;
}
