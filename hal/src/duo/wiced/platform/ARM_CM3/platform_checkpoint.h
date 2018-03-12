/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines macros to generate checkpoints
 */

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

#if defined(WICED_BOOT_CHECKPOINT_ADDR) && defined(WICED_BOOT_CHECKPOINT_BASE_VAL)

#define WICED_BOOT_CHECKPOINT_ENABLED 1

#define WICED_BOOT_CHECKPOINT_WRITE_C(val) \
    do \
    { \
        volatile uint32_t *reg = (uint32_t*)(WICED_BOOT_CHECKPOINT_ADDR); \
        *reg = (WICED_BOOT_CHECKPOINT_BASE_VAL) + (val); \
        __asm__ __volatile__("DMB"); \
    } while(0) \

#define WICED_BOOT_CHECKPOINT_WRITE_ASM(val, scratch_reg1, scratch_reg2) \
    LDR scratch_reg1, =(WICED_BOOT_CHECKPOINT_ADDR); \
    LDR scratch_reg2, =((WICED_BOOT_CHECKPOINT_BASE_VAL) + (val)); \
    STR scratch_reg2, [scratch_reg1]; \
    DMB

#else

#define WICED_BOOT_CHECKPOINT_ENABLED 0
#define WICED_BOOT_CHECKPOINT_WRITE_C(val)
#define WICED_BOOT_CHECKPOINT_WRITE_ASM(val, scratch_reg1, scratch_reg2)

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

/******************************************************
 *               Function Definitions
 ******************************************************/

