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
 *  Defines the WICED Network Interface.
 *
 *  Provides prototypes for functions that allow WICED to communicate
 *  with a network stack in an abstract way.
 */

#ifndef HEADER_WWD_NETWORK_INTERFACE_H_INCLUDED
#define HEADER_WWD_NETWORK_INTERFACE_H_INCLUDED

#include "wwd_buffer.h"
#include "wwd_constants.h"

#ifdef __cplusplus
extern "C"
{
#endif



/* @addtogroup netif Network Interface
 * Allows WICED to communicate with a network stack in an abstract way.
 *  @{
 */

/******************************************************
 *       Enumerations
 ******************************************************/

/******************************************************
 *             Function declarations
 ******************************************************/

/*
 * Called by WICED to pass received data to the network stack
 *
 * Implemented in 'network adapter driver' which is specific to the
 * network stack in use.
 * Packets received from the Wi-Fi network by WICED are forwarded to this function which
 * must be implemented in the network interface. Ethernet headers
 * are present at the start of these packet buffers.
 *
 * This function is called asynchronously in the context of the
 * WICED thread whenever new data has arrived.
 * Packet buffers are allocated within WICED, and ownership is transferred
 * to the network stack. The network stack or application is thus
 * responsible for releasing the packet buffers.
 * Most packet buffering systems have a pointer to the 'current point' within
 * the packet buffer. When this function is called, the pointer points
 * to the start of the Ethernet header. There is other inconsequential data
 * before the Ethernet header.
 *
 * It is preferable that the @ref host_network_process_ethernet_data function simply puts
 * the received packet on a queue for processing by another thread. This avoids the
 * WICED thread being unnecessarily tied up which would delay other packets
 * being transmitted or received.
 *
 * @param buffer : Handle of the packet which has just been received. Responsibility for
 *                 releasing this buffer is transferred from WICED at this point.
 * @param interface : The interface (AP or STA) on which the packet was received.
 *
 */
/*@external@*/ extern void host_network_process_ethernet_data( /*@only@*/ wiced_buffer_t buffer, wwd_interface_t interface ); /* Network stack assumes responsibility for freeing buffer */

/* Functions provided by WICED that may be called by Network Stack */

/*
 * Called by the Network Stack to send an ethernet frame
 *
 * Implemented in 'network adapter driver' which is specific to the
 * network stack in use.
 * This function takes Ethernet data from the network stack and queues it for transmission over the wireless network.
 * The function can be called from any thread context as it is thread safe, however
 * it must not be called from interrupt context since it can block while waiting
 * for a lock on the transmit queue.
 *
 * This function returns immediately after the packet has been queued for transmit,
 * NOT after it has been transmitted.  Packet buffers passed to the WICED core
 * are released inside the WICED core once they have been transmitted.
 *
 * Some network stacks assume the driver send function blocks until the packet has been physically sent. This
 * type of stack typically releases the packet buffer immediately after the driver send function returns.
 * In this case, and assuming the buffering system can count references to packet buffers, the driver send function
 * can take an additional reference to the packet buffer. This enables the network stack and the WICED core driver
 * to independently release their own packet buffer references.
 *
 * @param buffer : Handle of the packet buffer to be sent.
 * @param interface : the interface over which to send the packet (AP or STA)
 *
 */
extern void wwd_network_send_ethernet_data( /*@only@*/ wiced_buffer_t buffer, wwd_interface_t interface ); /* Returns immediately - Wiced_buffer_tx_completed will be called once the transmission has finished */

/*  @} */


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef HEADER_WWD_NETWORK_INTERFACE_H_INCLUDED */
