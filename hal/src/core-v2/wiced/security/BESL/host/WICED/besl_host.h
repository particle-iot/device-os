/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define BESL_INFO(x)     WPRINT_SECURITY_INFO(x)
#define BESL_ERROR(x)    WPRINT_SECURITY_ERROR(x)
#define BESL_DEBUG(x)    WPRINT_SECURITY_DEBUG(x)

#define BESL_ASSERT(string, x)

#ifdef BESL_HOST_IS_ALIGNED

#define BESL_READ_16(ptr)              (((uint8_t*)ptr)[0] + (((uint8_t*)ptr)[1] << 8))
#define BESL_READ_16_BE(ptr)           (((uint8_t*)ptr)[1] + (((uint8_t*)ptr)[0] << 8))
#define BESL_READ_32(ptr)              (((uint8_t*)ptr)[0] + ((((uint8_t*)ptr)[1] << 8)) + (((uint8_t*)ptr)[2] << 16) + (((uint8_t*)ptr)[3] << 24))
#define BESL_READ_32_BE(ptr)           (((uint8_t*)ptr)[3] + ((((uint8_t*)ptr)[2] << 8)) + (((uint8_t*)ptr)[1] << 16) + (((uint8_t*)ptr)[0] << 24))
#define BESL_WRITE_16(ptr, value)      do { ((uint8_t*)ptr)[0] = (uint8_t)value; ((uint8_t*)ptr)[1]=(uint8_t)(value>>8); } while(0)
#define BESL_WRITE_16_BE(ptr, value)   do { ((uint8_t*)ptr)[1] = (uint8_t)value; ((uint8_t*)ptr)[0]=(uint8_t)(value>>8); } while(0)
#define BESL_WRITE_32(ptr, value)      do { ((uint8_t*)ptr)[0] = (uint8_t)value; ((uint8_t*)ptr)[1]=(uint8_t)(value>>8); ((uint8_t*)ptr)[2]=(uint8_t)(value>>16); ((uint8_t*)ptr)[3]=(uint8_t)(value>>24); } while(0)
#define BESL_WRITE_32_BE(ptr, value)   do { ((uint8_t*)ptr)[3] = (uint8_t)value; ((uint8_t*)ptr)[2]=(uint8_t)(value>>8); ((uint8_t*)ptr)[1]=(uint8_t)(value>>16); ((uint8_t*)ptr)[0]=(uint8_t)(value>>24); } while(0)

#else

#define BESL_READ_16(ptr)            ((uint16_t*)ptr)[0]
#define BESL_READ_16_BE(ptr)         htobe16(((uint16_t*)ptr)[0])
#define BESL_READ_32(ptr)            ((uint32_t*)ptr)[0]
#define BESL_READ_32_BE(ptr)         htobe32(((uint32_t*)ptr)[0])
#define BESL_WRITE_16(ptr, value)    ((uint16_t*)ptr)[0] = value
#define BESL_WRITE_16_BE(ptr, value) ((uint16_t*)ptr)[0] = htobe16(value)
#define BESL_WRITE_32(ptr, value)    ((uint32_t*)ptr)[0] = value
#define BESL_WRITE_32_BE(ptr, value) ((uint32_t*)ptr)[0] = htobe32(value)

#endif

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

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
