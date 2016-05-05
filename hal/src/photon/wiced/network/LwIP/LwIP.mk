#
# Copyright (c) 2015 Broadcom
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# 3. Neither the name of Broadcom nor the names of other contributors to this
# software may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# 4. This software may not be used as a standalone product, and may only be used as
# incorporated in your product or device that incorporates Broadcom wireless connectivity
# products and solely for the purpose of enabling the functionalities of such Broadcom products.
#
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
                    ver$(VERSION)/src/netif/etharp.c

#                    ver$(VERSION)/src/core/ipv6/icmp6.c \
#                    ver$(VERSION)/src/core/ipv6/inet6.c \
#                    ver$(VERSION)/src/core/ipv6/ip6.c \
#                    ver$(VERSION)/src/core/ipv6/ip6_addr.c \

