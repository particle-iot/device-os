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

#include <stdint.h>
#include "besl_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Endian management functions */
uint32_t besl_host_hton32(uint32_t intlong);
uint16_t besl_host_hton16(uint16_t intshort);
uint32_t besl_host_hton32_ptr(uint8_t* in, uint8_t* out);
uint16_t besl_host_hton16_ptr(uint8_t* in, uint8_t* out);


extern besl_result_t besl_host_get_mac_address(besl_mac_t* address, uint32_t interface );
extern besl_result_t besl_host_set_mac_address(besl_mac_t* address, uint32_t interface );
extern void besl_host_random_bytes(uint8_t* buffer, uint16_t buffer_length);
extern void besl_host_get_time(besl_time_t* time);

/* Memory allocation functions */
extern void* besl_host_malloc( const char* name, uint32_t size );
extern void* besl_host_calloc( const char* name, uint32_t num, uint32_t size );
extern void  besl_host_free( void* p );

#ifdef __cplusplus
} /*extern "C" */
#endif
