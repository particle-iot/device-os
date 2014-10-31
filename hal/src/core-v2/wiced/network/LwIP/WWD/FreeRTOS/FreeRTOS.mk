#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Wiced_Network_LwIP_FreeRTOS

GLOBAL_INCLUDES += .
$(NAME)_SOURCES := sys_arch.c


ifdef LWIP_NUM_PACKET_BUFFERS_IN_POOL
GLOBAL_DEFINES += PBUF_POOL_TX_SIZE=$(LWIP_NUM_PACKET_BUFFERS_IN_POOL)
GLOBAL_DEFINES += PBUF_POOL_RX_SIZE=$(LWIP_NUM_PACKET_BUFFERS_IN_POOL)
endif