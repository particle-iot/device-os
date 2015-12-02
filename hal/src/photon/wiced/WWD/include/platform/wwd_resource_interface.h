/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "wwd_constants.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WWD_RESOURCE_WLAN_FIRMWARE,
    WWD_RESOURCE_WLAN_NVRAM,
} wwd_resource_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

#if defined( WWD_DYNAMIC_NVRAM )
extern uint32_t dynamic_nvram_size;
extern void*    dynamic_nvram_image;
#endif

/******************************************************
 *               Function Declarations
 ******************************************************/

extern wwd_result_t host_platform_resource_size( wwd_resource_t resource, uint32_t* size_out );

/*
 * This function is used when WWD_DIRECT_RESOURCES is defined
 */
extern wwd_result_t host_platform_resource_read_direct( wwd_resource_t resource, const void** ptr_out );

/*
 * This function is used when WWD_DIRECT_RESOURCES is not defined
 */
extern wwd_result_t host_platform_resource_read_indirect( wwd_resource_t resource, uint32_t offset, void* buffer, uint32_t buffer_size, uint32_t* size_out );

#ifdef __cplusplus
} /* extern "C" */
#endif
