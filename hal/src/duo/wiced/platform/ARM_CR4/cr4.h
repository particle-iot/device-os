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
 *
 */
#pragma once

#include <stdint.h>
#include "platform_toolchain.h"

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

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/**
 * Read the Instruction Fault Address Register
 */
static inline ALWAYS_INLINE uint32_t get_IFAR( void )
{
    uint32_t value;
    __asm__( "mrc p15, 0, %0, c6, c0, 2" : "=r" (value) : );
    return value;
}

/**
 * Read the Instruction Fault Status Register
 */
static inline ALWAYS_INLINE uint32_t get_IFSR( void )
{
    uint32_t value;
    __asm__( "mrc p15, 0, %0, c5, c0, 1" : "=r" (value) : );
    return value;
}

/**
 * Read the Data Fault Address Register
 */
static inline ALWAYS_INLINE uint32_t get_DFAR( void )
{
    uint32_t value;
    __asm__( "mrc p15, 0, %0, c6, c0, 0" : "=r" (value) : );
    return value;
}

/**
 * Read the Data Fault Status Register
 */
static inline ALWAYS_INLINE uint32_t get_DFSR( void )
{
    uint32_t value;
    __asm__( "mrc p15, 0, %0, c5, c0, 0" : "=r" (value) : );
    return value;
}

/**
 * Read the Program Counter and Processor Status Register
 */
static inline ALWAYS_INLINE uint32_t get_CPSR( void )
{
    uint32_t value;
    __asm__( "mrs %0, cpsr" : "=r" (value) : );
    return value;
}

/**
 * Read the Link Register
 */
static inline ALWAYS_INLINE uint32_t get_LR( void )
{
    uint32_t value;
    __asm__( "mov %0, lr" : "=r" (value) : );
    return value;
}

/**
 * Wait for interrupts
 */
static inline ALWAYS_INLINE void cpu_wait_for_interrupt( void )
{
    __asm__( "wfi" : : : "memory");
}

/**
 * Data Synchronisation Barrier
 */
static inline ALWAYS_INLINE void cpu_data_synchronisation_barrier( void )
{
    __asm__( "dsb" : : : "memory");
}

/*
 * Initialize performance monitoring registers
 */
static inline void cr4_init_cycle_counter( void )
{
    /* Reset cycle counter. Performance Monitors Control Register - PMCR */
    __asm__ __volatile__ ("MCR p15, 0, %0, c9, c12, 0" : : "r"(0x00000005));
    /* Enable the count - Performance Monitors Count Enable Set Register-  PMCNTENSET */
    __asm__ __volatile__ ("MCR p15, 0, %0, c9, c12, 1" : : "r"(0x80000000));
    /* Clear overflows - Performance Monitors Overflow Flag Status Register - PMOVSR */
    __asm__ __volatile__ ("MCR p15, 0, %0, c9, c12, 3" : : "r"(0x80000000));
}

/*
 * Read cycle counter
 */
static inline ALWAYS_INLINE uint32_t cr4_get_cycle_counter( void )
{
    uint32_t count;
    __asm__ __volatile__ ("MRC p15, 0, %0, c9, c13, 0" : "=r"(count));
    return count;
}

/*
 * Read overflow status register
 */
static inline ALWAYS_INLINE uint32_t cr4_get_overflow_flag_status( void )
{
    uint32_t flag;
    __asm__ __volatile__ ("MRC p15, 0, %0, c9, c12, 3" : "=r"(flag));
    return flag;
}

/*
 * Return whether cycle counter overflowed
 */
static inline ALWAYS_INLINE int cr4_is_cycle_counter_overflowed( void )
{
    return ( cr4_get_overflow_flag_status() & 0x80000000 ) ? 1 : 0;
}


/*
 * Should be defined by MCU
 */
void platform_backplane_debug( void );
void platform_exception_debug( void );

#ifdef __cplusplus
} /*extern "C" */
#endif
