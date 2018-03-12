/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef __ARCH_ARM_SRC_BCM4390X_BCM4390X_WWD_H
#define __ARCH_ARM_SRC_BCM4390X_BCM4390X_WWD_H

#include <nuttx/config.h>

#include <stdbool.h>

#include "wwd_constants.h"
#include "network/wwd_network_constants.h"
#include "wwd_buffer.h"

#include "wiced_constants.h"

typedef enum bcm4390x_wwd_link_event
{
  BCM4390X_WWD_LINK_EVENT_UP,
  BCM4390X_WWD_LINK_EVENT_DOWN,
  BCM4390X_WWD_LINK_EVENT_WIRELESS_UP,
  BCM4390X_WWD_LINK_EVENT_WIRELESS_DOWN,
  BCM4390X_WWD_LINK_EVENT_WIRELESS_RENEW
} bcm4390x_wwd_link_event_t;

int bcm4390x_wwd_init(void);
void bcm4390x_wwd_deinit(void);

int bcm4390x_wwd_wlan_init(void);
int bcm4390x_wwd_wlan_deinit(void);

int bcm4390x_wwd_wlan_up(wiced_interface_t interface);
int bcm4390x_wwd_wlan_down(wiced_interface_t interface);

int bcm4390x_wwd_register_multicast_address(wiced_interface_t interface, const uint8_t *mac);
int bcm4390x_wwd_unregister_multicast_address(wiced_interface_t interface, const uint8_t *mac);

int bcm4390x_wwd_get_interface_mac_address(wiced_interface_t interface, uint8_t *mac);

uint8_t* bcm4390x_wwd_get_buffer_data(wiced_buffer_t buffer, uint16_t *len);

void bcm4390x_wwd_rxavail(wiced_interface_t interface, wiced_buffer_t buffer);

void bcm4390x_wwd_send_ethernet_data(wiced_interface_t interface, wiced_buffer_t buffer, uint16_t len, bool tx_enable);

uint8_t* bcm4390x_wwd_alloc_tx_buffer(wiced_buffer_t *buffer);
void bcm4390x_wwd_free_tx_buffer(wiced_buffer_t buffer);

void bcm4390x_wwd_free_rx_buffer(wiced_buffer_t buffer);

void bcm4390x_wwd_buffer_init_fifo(wiced_buffer_fifo_t *fifo);
void bcm4390x_wwd_buffer_push_to_fifo(wiced_buffer_fifo_t *fifo, wiced_buffer_t buffer);
wiced_buffer_t bcm4390x_wwd_buffer_pop_from_fifo(wiced_buffer_fifo_t *fifo);

void bcm4390x_wwd_link_event_handler(wiced_interface_t interface, bcm4390x_wwd_link_event_t event);

#endif /* __ARCH_ARM_SRC_BCM4390X_BCM4390X_WWD_H */
