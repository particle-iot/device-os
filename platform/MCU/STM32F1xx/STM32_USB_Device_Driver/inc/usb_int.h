/**
  ******************************************************************************
  * @file    usb_int.h
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    28-August-2012
  * @brief   Endpoint CTR (Low and High) interrupt's service routines prototypes
  ******************************************************************************
  Released into the public domain.
  This work is free: you can redistribute it and/or modify it under the terms of
  Creative Commons Zero license v1.0

  This work is licensed under the Creative Commons Zero 1.0 United States License.
  To view a copy of this license, visit http://creativecommons.org/publicdomain/zero/1.0/
  or send a letter to Creative Commons, 171 Second Street, Suite 300, San Francisco,
  California, 94105, USA.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
  *
  ******************************************************************************
  */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_INT_H
#define __USB_INT_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void CTR_LP(void);
void CTR_HP(void);

/* External variables --------------------------------------------------------*/

#endif /* __USB_INT_H */


