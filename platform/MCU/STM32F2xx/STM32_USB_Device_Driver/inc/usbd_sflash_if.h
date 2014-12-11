/**
 ******************************************************************************
 * @file    usbd_sflash_if.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    26-Nov-2014
 * @brief   Header for usbd_sflash_if.c file.
 ******************************************************************************
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SFLASH_IF_MAL_H
#define __SFLASH_IF_MAL_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_dfu_mal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define sFLASH_START_ADD                  0x00000000
#define sFLASH_END_ADD                    0x00100000

#define sFLASH_IF_STRING                  "@Serial Flash   /0x00000000/256*004Kg"

extern DFU_MAL_Prop_TypeDef DFU_sFlash_cb;

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* __SFLASH_IF_MAL_H */

