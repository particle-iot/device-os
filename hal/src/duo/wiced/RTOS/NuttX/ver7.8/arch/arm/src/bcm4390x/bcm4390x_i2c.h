/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef __ARCH_ARM_SRC_BCM4390X_BCM4390X_I2C_H
#define __ARCH_ARM_SRC_BCM4390X_BCM4390X_I2C_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <stdbool.h>
#include <errno.h>
#include <debug.h>
#include <nuttx/i2c.h>
#include "wiced_platform.h"


/************************************************************************************
 * Pre-processor Declarations
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/

/************************************************************************************
 * Name: up_i2ciinitialize
 *
 * Description:
 *   Initialize the selected I2C port.
 *
 * Input Parameter:
 *   Port number
 *
 ************************************************************************************/

void bcm4390x_i2cinitialize(int port, wiced_i2c_device_t **wiced_i2c_device);

#endif /* ARCH_ARM_SRC_BCM4390X_BCM4390X_I2C_H */
