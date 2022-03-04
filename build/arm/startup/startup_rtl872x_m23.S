/*
 
Copyright (c) 2009-2018 ARM Limited. All rights reserved.

    SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the License); you may
not use this file except in compliance with the License.
You may obtain a copy of the License at

    www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an AS IS BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

NOTICE: This file has been modified by Nordic Semiconductor ASA.

*/

    .syntax unified
    .arch armv8-m.main

#ifdef __STARTUP_CONFIG
#include "startup_config.h"
#ifndef __STARTUP_CONFIG_STACK_ALIGNEMENT
#define __STARTUP_CONFIG_STACK_ALIGNEMENT 3
#endif
#endif

    .section .stack
#if defined(__STARTUP_CONFIG)
    .align __STARTUP_CONFIG_STACK_ALIGNEMENT
    .equ    Stack_Size, __STARTUP_CONFIG_STACK_SIZE
#elif defined(__STACK_SIZE)
    .align 3
    .equ    Stack_Size, __STACK_SIZE
#else
    .align 3
    .equ    Stack_Size, 8192
#endif
    .globl __StackTop
    .globl __StackLimit
__StackLimit:
    .space Stack_Size
    .size __StackLimit, . - __StackLimit
__StackTop:
    .size __StackTop, . - __StackTop

    .section .heap
    .align 3
#if defined(__STARTUP_CONFIG)
    .equ Heap_Size, __STARTUP_CONFIG_HEAP_SIZE
#elif defined(__HEAP_SIZE)
    .equ Heap_Size, __HEAP_SIZE
#else
    .equ    Heap_Size, 8192
#endif
    .globl __HeapBase
    .globl __HeapLimit
__HeapBase:
    .if Heap_Size
    .space Heap_Size
    .endif
    .size __HeapBase, . - __HeapBase
__HeapLimit:
    .size __HeapLimit, . - __HeapLimit

/* Reset Handler */
    .text
    .thumb
    .thumb_func
    .align 1
    .section .text,"a",%progbits
    .globl Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:


/* Loop to copy data from read only memory to RAM.
 * The ranges of copy from/to are specified by following symbols:
 *      __etext: LMA of start of the section to copy from. Usually end of text
 *      link_global_data_start: VMA of start of the section to copy to.
 *      link_bss_location: VMA of end of the section to copy to. Normally link_global_data_end is used, but by using link_bss_location
 *                    the user can add their own initialized data section before BSS section with the INTERT AFTER command.
 *
 * All addresses must be aligned to 4 bytes boundary.
 */
    ldr r1, =link_global_data_initial_values
    ldr r2, =link_global_data_start
    ldr r3, =link_bss_location

    subs r3, r2
    ble .L_loop1_done

.L_loop1:
    subs r3, #4
    ldr r0, [r1,r3]
    str r0, [r2,r3]
    bgt .L_loop1

.L_loop1_done:

/* This part of work usually is done in C library startup code. Otherwise,
 * define __STARTUP_CLEAR_BSS to enable it in this startup. This section
 * clears the RAM where BSS data is located.
 *
 * The BSS section is specified by following symbols
 *    link_bss_location: start of the BSS section.
 *    link_bss_end: end of the BSS section.
 *
 * All addresses must be aligned to 4 bytes boundary.
 */
/* Enable by default */
#ifndef __STARTUP_CLEAR_BSS
#define __STARTUP_CLEAR_BSS 1
#endif
#ifdef __STARTUP_CLEAR_BSS
    ldr r1, =link_bss_location
    ldr r2, =link_bss_end

    movs r0, 0

    subs r2, r1
    ble .L_loop3_done

.L_loop3:
    subs r2, #4
    str r0, [r1, r2]
    bgt .L_loop3

.L_loop3_done:
#endif /* __STARTUP_CLEAR_BSS */

/* Execute SystemInit function. */
    bl SystemInit

/* Call _start function provided by libraries.
 * If those libraries are not accessible, define __START as your entry point.
 */
#ifndef __START
#define __START main
#endif
    .include "spark_init.S"
    bl __START
