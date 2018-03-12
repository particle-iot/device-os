/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
   char*        name;
   int          chip_select;
   uint32_t     size;
   uint8_t      bus_data_width;
   uint32_t     memory_freq;

   unsigned int byte_lane_select_enable : 1;
   unsigned int async                   : 1;
   unsigned int mux_addr_data           : 1;

   /* pointer to a data which is required by a particular sram memory driver */
   /* different CPUs required different set of settings to make external memory chips work */
   const void* private_data;
} wiced_sram_device_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern wiced_result_t platform_init_external_memory( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
