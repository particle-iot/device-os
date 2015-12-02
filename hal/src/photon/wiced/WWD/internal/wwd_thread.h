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

/** @file
 *  The Wiced Thread allows thread safe access to the Wiced hardware bus
 *  This is an Wiced internal file and should not be used by functions outside Wiced.
 *
 *  This file provides prototypes for functions which allow multiple threads to use the Wiced hardware bus (SDIO or SPI)
 *  This is achieved by having a single thread (the "Wiced Thread") which queues messages to be sent, sending
 *  them sequentially, as well as receiving messages as they arrive.
 *
 *  Messages to be sent come from the @ref wwd_sdpcm_send_common function in wwd_sdpcm.c .  The messages already
 *  contain SDPCM headers, but not any bus headers (GSPI), and are passed via a queue
 *  This function can be called from any thread.
 *
 *  Messages are received by way of a callback supplied by in wwd_sdpcm.c - wwd_sdpcm_process_rx_packet
 *  Received messages are delivered in the context of the Wiced Thread, so the callback function needs to avoid blocking.
 *
 */

#ifndef INCLUDED_WWD_THREAD_H_
#define INCLUDED_WWD_THREAD_H_

#include "wwd_buffer.h"
#include "wwd_constants.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define WWD_THREAD_PRIORITY   RTOS_HIGHEST_PRIORITY

/** Initialises the Wiced Thread
 *
 * Initialises the Wiced thread, and its flags/semaphores,
 * then starts it running
 *
 * @return    wiced result code
 */
extern wwd_result_t wwd_thread_init( void )  /*@modifies internalState@*/;


/** Terminates the Wiced Thread
 *
 * Sets a flag then wakes the Wiced Thread to force it to terminate.
 *
 */
extern void wwd_thread_quit( void );


extern void wwd_thread_notify( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_THREAD_H_ */
