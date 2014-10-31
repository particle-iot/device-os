#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := LwIP

VERSION := 1.4.0.rc1

$(NAME)_COMPONENTS += WICED/network/LwIP/WWD
ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_COMPONENTS += WICED/network/LwIP/WICED
endif

ifeq ($(BUILD_TYPE),debug)
GLOBAL_DEFINES := WICED_LWIP_DEBUG
endif

VALID_RTOS_LIST:= FreeRTOS

# Define some macros to allow for some network-specific checks
GLOBAL_DEFINES += NETWORK_$(NAME)=1
GLOBAL_DEFINES += $(NAME)_VERSION=$$(SLASH_QUOTE_START)v$(VERSION)$$(SLASH_QUOTE_END)

GLOBAL_INCLUDES := ver$(VERSION) \
                   ver$(VERSION)/src/include \
                   ver$(VERSION)/src/include/ipv4 \
                   WICED

$(NAME)_SOURCES :=  ver$(VERSION)/src/api/api_lib.c \
                    ver$(VERSION)/src/api/api_msg.c \
                    ver$(VERSION)/src/api/err.c \
                    ver$(VERSION)/src/api/netbuf.c \
                    ver$(VERSION)/src/api/netdb.c \
                    ver$(VERSION)/src/api/netifapi.c \
                    ver$(VERSION)/src/api/sockets.c \
                    ver$(VERSION)/src/api/tcpip.c \
                    ver$(VERSION)/src/core/dhcp.c \
                    ver$(VERSION)/src/core/dns.c \
                    ver$(VERSION)/src/core/init.c \
                    ver$(VERSION)/src/core/ipv4/autoip.c \
                    ver$(VERSION)/src/core/ipv4/icmp.c \
                    ver$(VERSION)/src/core/ipv4/igmp.c \
                    ver$(VERSION)/src/core/ipv4/inet.c \
                    ver$(VERSION)/src/core/ipv4/inet_chksum.c \
                    ver$(VERSION)/src/core/ipv4/ip.c \
                    ver$(VERSION)/src/core/ipv4/ip_addr.c \
                    ver$(VERSION)/src/core/ipv4/ip_frag.c \
                    ver$(VERSION)/src/core/def.c \
                    ver$(VERSION)/src/core/timers.c \
                    ver$(VERSION)/src/core/mem.c \
                    ver$(VERSION)/src/core/memp.c \
                    ver$(VERSION)/src/core/netif.c \
                    ver$(VERSION)/src/core/pbuf.c \
                    ver$(VERSION)/src/core/raw.c \
                    ver$(VERSION)/src/core/snmp/asn1_dec.c \
                    ver$(VERSION)/src/core/snmp/asn1_enc.c \
                    ver$(VERSION)/src/core/snmp/mib2.c \
                    ver$(VERSION)/src/core/snmp/mib_structs.c \
                    ver$(VERSION)/src/core/snmp/msg_in.c \
                    ver$(VERSION)/src/core/snmp/msg_out.c \
                    ver$(VERSION)/src/core/stats.c \
                    ver$(VERSION)/src/core/sys.c \
                    ver$(VERSION)/src/core/tcp.c \
                    ver$(VERSION)/src/core/tcp_in.c \
                    ver$(VERSION)/src/core/tcp_out.c \
                    ver$(VERSION)/src/core/udp.c \
                    ver$(VERSION)/src/netif/etharp.c \

#                    ver$(VERSION)/src/core/ipv6/icmp6.c \
#                    ver$(VERSION)/src/core/ipv6/inet6.c \
#                    ver$(VERSION)/src/core/ipv6/ip6.c \
#                    ver$(VERSION)/src/core/ipv6/ip6_addr.c \

