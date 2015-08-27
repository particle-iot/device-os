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
 *  Header for using WWD with no RTOS or network stack
 *
 *  It is possible to use these WWD without any operating system. To do this,
 *  the user application is required to periodically use the functions in this
 *  file to allow WWD to send and receive data across the SPI/SDIO bus.
 *
 */

#ifndef INCLUDED_WWD_POLL_H
#define INCLUDED_WWD_POLL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Function declarations
 ******************************************************/

/*@-exportlocal@*/

/** Sends the first queued packet
 *
 * Checks the queue to determine if there is any packets waiting
 * to be sent. If there are, then it sends the first one.
 *
 * This function is normally used by the WWD Thread, but can be
 * called periodically by systems which have no RTOS to ensure
 * packets get sent.
 *
 * @return    1 : packet was sent
 *            0 : no packet sent
 */
extern int8_t wwd_thread_send_one_packet( void )  /*@modifies internalState @*/;


/** Receives a packet if one is waiting
 *
 * Checks the wifi chip fifo to determine if there is any packets waiting
 * to be received. If there are, then it receives the first one, and calls
 * the callback @ref wwd_sdpcm_process_rx_packet (in wwd_sdpcm.c).
 *
 * This function is normally used by the WWD Thread, but can be
 * called periodically by systems which have no RTOS to ensure
 * packets get received properly.
 *
 * @return    1 : packet was received
 *            0 : no packet waiting
 */
extern int8_t wwd_thread_receive_one_packet( void )  /*@modifies internalState @*/;


/** Sends and Receives all waiting packets
 *
 * Repeatedly calls wwd_thread_send_one_packet and wwd_thread_receive_one_packet
 * to send and receive packets, until there are no more packets waiting to
 * be transferred.
 *
 * This function is normally used by the WWD Thread, but can be
 * called periodically by systems which have no RTOS to ensure
 * packets get send and received properly.
 *
 * @return    1 : packet was sent or received
 *            0 : no packet was sent or received
 */
extern int8_t wwd_thread_poll_all( void ) /*@modifies internalState@*/;

/*@+exportlocal@*/

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_POLL_H */
