/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HAL_LWIP_LWIPHOOKS_H
#define HAL_LWIP_LWIPHOOKS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* IPv4 hooks */
int lwip_hook_ip4_input(struct pbuf *p, struct netif *inp);
int lwip_hook_ip4_input_post_validation(struct pbuf* p, struct netif* inp);

/* IPv6 hooks */
int lwip_hook_ip6_input(struct pbuf* p, struct netif* inp);
int lwip_hook_ip6_input_post_validation(struct pbuf* p, struct netif* inp);
int lwip_hook_ip6_input_accept_multicast(struct pbuf* p, struct netif* inp, ip6_addr_t* dest);
int lwip_hook_ip6_route_multicast(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp);
int lwip_hook_ip6_input_post_local_handling(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp, u8_t proto);
int lwip_hook_ip6_forward_pre_routing(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp, u32_t* flags);
int lwip_hook_ip6_forward_post_routing(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp, struct netif* out, u32_t* flags);
struct netif* lwip_hook_ip6_route(ip6_addr_t* src, ip6_addr_t* dst);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * LWIP_HOOK_TCP_ISN:
 * Hook for generation of the Initial Sequence Number (ISN) for a new TCP
 * connection. The default lwIP ISN generation algorithm is very basic and may
 * allow for TCP spoofing attacks. This hook provides the means to implement
 * the standardized ISN generation algorithm from RFC 6528 (see contrib/adons/tcp_isn),
 * or any other desired algorithm as a replacement.
 * Called from tcp_connect() and tcp_listen_input() when an ISN is needed for
 * a new TCP connection, if TCP support (@ref LWIP_TCP) is enabled.\n
 * Signature:\code{.c}
 * u32_t my_hook_tcp_isn(const ip_addr_t* local_ip, u16_t local_port, const ip_addr_t* remote_ip, u16_t remote_port);
 * \endcode
 * - it may be necessary to use "struct ip_addr" (ip4_addr, ip6_addr) instead of "ip_addr_t" in function declarations\n
 * Arguments:
 * - local_ip: pointer to the local IP address of the connection
 * - local_port: local port number of the connection (host-byte order)
 * - remote_ip: pointer to the remote IP address of the connection
 * - remote_port: remote port number of the connection (host-byte order)\n
 * Return value:
 * - the 32-bit Initial Sequence Number to use for the new TCP connection.
 */
// #define LWIP_HOOK_TCP_ISN(local_ip, local_port, remote_ip, remote_port)

/**
 * LWIP_HOOK_TCP_INPACKET_PCB:
 * Hook for intercepting incoming packets before they are passed to a pcb. This
 * allows updating some state or even dropping a packet.
 * Signature:\code{.c}
 * err_t my_hook_tcp_inpkt(struct tcp_pcb *pcb, struct tcp_hdr *hdr, u16_t optlen, u16_t opt1len, u8_t *opt2, struct pbuf *p);
 * \endcode
 * Arguments:
 * - pcb: tcp_pcb selected for input of this packet (ATTENTION: this may be
 *        struct tcp_pcb_listen if pcb->state == LISTEN)
 * - hdr: pointer to tcp header (ATTENTION: tcp options may not be in one piece!)
 * - optlen: tcp option length
 * - opt1len: tcp option length 1st part
 * - opt2: if this is != NULL, tcp options are split among 2 pbufs. In that case,
 *         options start at right after the tcp header ('(u8_t*)(hdr + 1)') for
 *         the first 'opt1len' bytes and the rest starts at 'opt2'. opt2len can
 *         be simply calculated: 'opt2len = optlen - opt1len;'
 * - p: input packet, p->payload points to application data (that's why tcp hdr
 *      and options are passed in seperately)
 * Return value:
 * - ERR_OK: continue input of this packet as normal
 * - != ERR_OK: drop this packet for input (don't continue input processing)
 *
 * ATTENTION: don't call any tcp api functions that might change tcp state (pcb
 * state or any pcb lists) from this callback!
 */
// #define LWIP_HOOK_TCP_INPACKET_PCB(pcb, hdr, optlen, opt1len, opt2, p)

/**
 * LWIP_HOOK_TCP_OUT_TCPOPT_LENGTH:
 * Hook for increasing the size of the options allocated with a tcp header.
 * Together with LWIP_HOOK_TCP_OUT_ADD_TCPOPTS, this can be used to add custom
 * options to outgoing tcp segments.
 * Signature:\code{.c}
 * u8_t my_hook_tcp_out_tcpopt_length(const struct tcp_pcb *pcb, u8_t internal_option_length);
 * \endcode
 * Arguments:
 * - pcb: tcp_pcb that transmits (ATTENTION: this may be NULL or
 *        struct tcp_pcb_listen if pcb->state == LISTEN)
 * - internal_option_length: tcp option length used by the stack internally
 * Return value:
 * - a number of bytes to allocate for tcp options (internal_option_length <= ret <= 40)
 *
 * ATTENTION: don't call any tcp api functions that might change tcp state (pcb
 * state or any pcb lists) from this callback!
 */
// #define LWIP_HOOK_TCP_OUT_TCPOPT_LENGTH(pcb, internal_len)

/**
 * LWIP_HOOK_TCP_OUT_ADD_TCPOPTS:
 * Hook for adding custom options to outgoing tcp segments.
 * Space for these custom options has to be reserved via LWIP_HOOK_TCP_OUT_TCPOPT_LENGTH.
 * Signature:\code{.c}
 * u32_t *my_hook_tcp_out_add_tcpopts(struct pbuf *p, struct tcp_hdr *hdr, const struct tcp_pcb *pcb, u32_t *opts);
 * \endcode
 * Arguments:
 * - p: output packet, p->payload pointing to tcp header, data follows
 * - hdr: tcp header
 * - pcb: tcp_pcb that transmits (ATTENTION: this may be NULL or
 *        struct tcp_pcb_listen if pcb->state == LISTEN)
 * - opts: pointer where to add the custom options (there may already be options
 *         between the header and these)
 * Return value:
 * - pointer pointing directly after the inserted options
 *
 * ATTENTION: don't call any tcp api functions that might change tcp state (pcb
 * state or any pcb lists) from this callback!
 */
// #define LWIP_HOOK_TCP_OUT_ADD_TCPOPTS(p, hdr, pcb, opts)

/**
 * LWIP_HOOK_IP4_INPUT(pbuf, input_netif):
 * Called from ip_input() (IPv4)
 * Signature:\code{.c}
 *   int my_hook(struct pbuf *pbuf, struct netif *input_netif);
 * \endcode
 * Arguments:
 * - pbuf: received struct pbuf passed to ip_input()
 * - input_netif: struct netif on which the packet has been received
 * Return values:
 * - 0: Hook has not consumed the packet, packet is processed as normal
 * - != 0: Hook has consumed the packet.
 * If the hook consumed the packet, 'pbuf' is in the responsibility of the hook
 * (i.e. free it when done).
 */
// #define LWIP_HOOK_IP4_INPUT(pbuf, input_netif) lwip_hook_ip4_input(pbuf, input_netif)

// #define LWIP_HOOK_IP4_INPUT_POST_VALIDATION(pbuf, input_netif) lwip_hook_ip4_input_post_validation(pbuf, input_netif)

/**
 * LWIP_HOOK_IP4_ROUTE(dest):
 * Called from ip_route() (IPv4)
 * Signature:\code{.c}
 *   struct netif *my_hook(const ip4_addr_t *dest);
 * \endcode
 * Arguments:
 * - dest: destination IPv4 address
 * Returns values:
 * - the destination netif
 * - NULL if no destination netif is found. In that case, ip_route() continues as normal.
 */
// #define LWIP_HOOK_IP4_ROUTE()

/**
 * LWIP_HOOK_IP4_ROUTE_SRC(src, dest):
 * Source-based routing for IPv4 - called from ip_route() (IPv4)
 * Signature:\code{.c}
 *   struct netif *my_hook(const ip4_addr_t *src, const ip4_addr_t *dest);
 * \endcode
 * Arguments:
 * - src: local/source IPv4 address
 * - dest: destination IPv4 address
 * Returns values:
 * - the destination netif
 * - NULL if no destination netif is found. In that case, ip_route() continues as normal.
 */
// #define LWIP_HOOK_IP4_ROUTE_SRC(src, dest)

/**
 * LWIP_HOOK_ETHARP_GET_GW(netif, dest):
 * Called from etharp_output() (IPv4)
 * Signature:\code{.c}
 *   const ip4_addr_t *my_hook(struct netif *netif, const ip4_addr_t *dest);
 * \endcode
 * Arguments:
 * - netif: the netif used for sending
 * - dest: the destination IPv4 address
 * Return values:
 * - the IPv4 address of the gateway to handle the specified destination IPv4 address
 * - NULL, in which case the netif's default gateway is used
 *
 * The returned address MUST be directly reachable on the specified netif!
 * This function is meant to implement advanced IPv4 routing together with
 * LWIP_HOOK_IP4_ROUTE(). The actual routing/gateway table implementation is
 * not part of lwIP but can e.g. be hidden in the netif's state argument.
*/
// #define LWIP_HOOK_ETHARP_GET_GW(netif, dest)

/**
 * LWIP_HOOK_IP6_INPUT(pbuf, input_netif):
 * Called from ip6_input() (IPv6)
 * Signature:\code{.c}
 *   int my_hook(struct pbuf *pbuf, struct netif *input_netif);
 * \endcode
 * Arguments:
 * - pbuf: received struct pbuf passed to ip6_input()
 * - input_netif: struct netif on which the packet has been received
 * Return values:
 * - 0: Hook has not consumed the packet, packet is processed as normal
 * - != 0: Hook has consumed the packet.
 * If the hook consumed the packet, 'pbuf' is in the responsibility of the hook
 * (i.e. free it when done).
 */
// #define LWIP_HOOK_IP6_INPUT(pbuf, input_netif)

// #define LWIP_HOOK_IP6_INPUT_ACCEPT_MULTICAST(pbuf, input_netif, dest_addr) lwip_hook_ip6_input_accept_multicast(pbuf, input_netif, dest_addr)
// #define LWIP_HOOK_IP6_INPUT_POST_LOCAL_HANDLING(pbuf, ip6hdr, input_netif, proto) lwip_hook_ip6_input_post_local_handling(pbuf, ip6hdr, input_netif, proto)
// #define LWIP_HOOK_IP6_INPUT_POST_VALIDATION(pbuf, input_netif) lwip_hook_ip6_input_post_validation(pbuf, input_netif)
// #define LWIP_HOOK_IP6_FORWARD_POST_ROUTING(pbuf, ip6hdr, input_netif, output_netif, flags) lwip_hook_ip6_forward_post_routing(pbuf, ip6hdr, input_netif, output_netif, flags)
// #define LWIP_HOOK_IP6_FORWARD_PRE_ROUTING(pbuf, ip6hdr, input_netif, flags) lwip_hook_ip6_forward_pre_routing(pbuf, ip6hdr, input_netif, flags)

/**
 * LWIP_HOOK_IP6_ROUTE(src, dest):
 * Called from ip_route() (IPv6)
 * Signature:\code{.c}
 *   struct netif *my_hook(const ip6_addr_t *dest, const ip6_addr_t *src);
 * \endcode
 * Arguments:
 * - src: source IPv6 address
 * - dest: destination IPv6 address
 * Return values:
 * - the destination netif
 * - NULL if no destination netif is found. In that case, ip6_route() continues as normal.
 */
//#define LWIP_HOOK_IP6_ROUTE(src, dest) lwip_hook_ip6_route(src, dest)

/**
 * LWIP_HOOK_ND6_GET_GW(netif, dest):
 * Called from nd6_get_next_hop_entry() (IPv6)
 * Signature:\code{.c}
 *   const ip6_addr_t *my_hook(struct netif *netif, const ip6_addr_t *dest);
 * \endcode
 * Arguments:
 * - netif: the netif used for sending
 * - dest: the destination IPv6 address
 * Return values:
 * - the IPv6 address of the next hop to handle the specified destination IPv6 address
 * - NULL, in which case a NDP-discovered router is used instead
 *
 * The returned address MUST be directly reachable on the specified netif!
 * This function is meant to implement advanced IPv6 routing together with
 * LWIP_HOOK_IP6_ROUTE(). The actual routing/gateway table implementation is
 * not part of lwIP but can e.g. be hidden in the netif's state argument.
*/
// #define LWIP_HOOK_ND6_GET_GW(netif, dest)

/**
 * LWIP_HOOK_VLAN_CHECK(netif, eth_hdr, vlan_hdr):
 * Called from ethernet_input() if VLAN support is enabled
 * Signature:\code{.c}
 *   int my_hook(struct netif *netif, struct eth_hdr *eth_hdr, struct eth_vlan_hdr *vlan_hdr);
 * \endcode
 * Arguments:
 * - netif: struct netif on which the packet has been received
 * - eth_hdr: struct eth_hdr of the packet
 * - vlan_hdr: struct eth_vlan_hdr of the packet
 * Return values:
 * - 0: Packet must be dropped.
 * - != 0: Packet must be accepted.
 */
// #define LWIP_HOOK_VLAN_CHECK(netif, eth_hdr, vlan_hdr)

/**
 * LWIP_HOOK_VLAN_SET:
 * Hook can be used to set prio_vid field of vlan_hdr. If you need to store data
 * on per-netif basis to implement this callback, see @ref netif_cd.
 * Called from ethernet_output() if VLAN support (@ref ETHARP_SUPPORT_VLAN) is enabled.\n
 * Signature:\code{.c}
 *   s32_t my_hook_vlan_set(struct netif* netif, struct pbuf* pbuf, const struct eth_addr* src, const struct eth_addr* dst, u16_t eth_type);\n
 * \endcode
 * Arguments:
 * - netif: struct netif that the packet will be sent through
 * - p: struct pbuf packet to be sent
 * - src: source eth address
 * - dst: destination eth address
 * - eth_type: ethernet type to packet to be sent\n
 * 
 * 
 * Return values:
 * - &lt;0: Packet shall not contain VLAN header.
 * - 0 &lt;= return value &lt;= 0xFFFF: Packet shall contain VLAN header. Return value is prio_vid in host byte order.
 */
// #define LWIP_HOOK_VLAN_SET(netif, p, src, dst, eth_type)

/**
 * LWIP_HOOK_MEMP_AVAILABLE(memp_t_type):
 * Called from memp_free() when a memp pool was empty and an item is now available
 * Signature:\code{.c}
 *   void my_hook(memp_t type);
 * \endcode
 */
// #define LWIP_HOOK_MEMP_AVAILABLE(memp_t_type)

/**
 * LWIP_HOOK_UNKNOWN_ETH_PROTOCOL(pbuf, netif):
 * Called from ethernet_input() when an unknown eth type is encountered.
 * Signature:\code{.c}
 *   err_t my_hook(struct pbuf* pbuf, struct netif* netif);
 * \endcode
 * Arguments:
 * - p: rx packet with unknown eth type
 * - netif: netif on which the packet has been received
 * Return values:
 * - ERR_OK if packet is accepted (hook function now owns the pbuf)
 * - any error code otherwise (pbuf is freed)
 *
 * Payload points to ethernet header!
 */
// #define LWIP_HOOK_UNKNOWN_ETH_PROTOCOL(pbuf, netif)

/**
 * LWIP_HOOK_DHCP_APPEND_OPTIONS(netif, dhcp, state, msg, msg_type, options_len_ptr):
 * Called from various dhcp functions when sending a DHCP message.
 * This hook is called just before the DHCP message trailer is added, so the
 * options are at the end of a DHCP message.
 * Signature:\code{.c}
 *   void my_hook(struct netif *netif, struct dhcp *dhcp, u8_t state, struct dhcp_msg *msg,
 *                u8_t msg_type, u16_t *options_len_ptr);
 * \endcode
 * Arguments:
 * - netif: struct netif that the packet will be sent through
 * - dhcp: struct dhcp on that netif
 * - state: current dhcp state (dhcp_state_enum_t as an u8_t)
 * - msg: struct dhcp_msg that will be sent
 * - msg_type: dhcp message type to be sent (u8_t)
 * - options_len_ptr: pointer to the current length of options in the dhcp_msg "msg"
 *                    (must be increased when options are added!)
 *
 * Options need to appended like this:
 *   LWIP_ASSERT("dhcp option overflow", *options_len_ptr + option_len + 2 <= DHCP_OPTIONS_LEN);
 *   msg->options[(*options_len_ptr)++] = &lt;option_number&gt;;
 *   msg->options[(*options_len_ptr)++] = &lt;option_len&gt;;
 *   msg->options[(*options_len_ptr)++] = &lt;option_bytes&gt;;
 *   [...]
 */
// #define LWIP_HOOK_DHCP_APPEND_OPTIONS(netif, dhcp, state, msg, msg_type, options_len_ptr)

/**
 * LWIP_HOOK_DHCP_PARSE_OPTION(netif, dhcp, state, msg, msg_type, option, len, pbuf, option_value_offset):
 * Called from dhcp_parse_reply when receiving a DHCP message.
 * This hook is called for every option in the received message that is not handled internally.
 * Signature:\code{.c}
 *   void my_hook(struct netif *netif, struct dhcp *dhcp, u8_t state, struct dhcp_msg *msg,
 *                u8_t msg_type, u8_t option, u8_t option_len, struct pbuf *pbuf, u16_t option_value_offset);
 * \endcode
 * Arguments:
 * - netif: struct netif that the packet will be sent through
 * - dhcp: struct dhcp on that netif
 * - state: current dhcp state (dhcp_state_enum_t as an u8_t)
 * - msg: struct dhcp_msg that was received
 * - msg_type: dhcp message type received (u8_t, ATTENTION: only valid after
 *             the message type option has been parsed!)
 * - option: option value (u8_t)
 * - len: option data length (u8_t)
 * - pbuf: pbuf where option data is contained
 * - option_value_offset: offset in pbuf where option data begins
 *
 * A nice way to get the option contents is pbuf_get_contiguous():
 *  u8_t buf[32];
 *  u8_t *ptr = (u8_t*)pbuf_get_contiguous(p, buf, sizeof(buf), LWIP_MIN(option_len, sizeof(buf)), offset);
 */
// #define LWIP_HOOK_DHCP_PARSE_OPTION(netif, dhcp, state, msg, msg_type, option, len, pbuf, offset)

/**
 * LWIP_HOOK_SOCKETS_SETSOCKOPT(s, sock, level, optname, optval, optlen, err)
 * Called from socket API to implement setsockopt() for options not provided by lwIP.
 * Core lock is held when this hook is called.
 * Signature:\code{.c}
 *   int my_hook(int s, struct lwip_sock *sock, int level, int optname, const void *optval, socklen_t optlen, int *err)
 * \endcode
 * Arguments:
 * - s: socket file descriptor
 * - sock: internal socket descriptor (see lwip/priv/sockets_priv.h)
 * - level: protocol level at which the option resides
 * - optname: option to set
 * - optval: value to set
 * - optlen: size of optval
 * - err: output error
 * Return values:
 * - 0: Hook has not consumed the option, code continues as normal (to internal options)
 * - != 0: Hook has consumed the option, 'err' is returned
 */
// #define LWIP_HOOK_SOCKETS_SETSOCKOPT(s, sock, level, optname, optval, optlen, err)

/**
 * LWIP_HOOK_SOCKETS_GETSOCKOPT(s, sock, level, optname, optval, optlen, err)
 * Called from socket API to implement getsockopt() for options not provided by lwIP.
 * Core lock is held when this hook is called.
 * Signature:\code{.c}
 *   int my_hook(int s, struct lwip_sock *sock, int level, int optname, void *optval, socklen_t *optlen, int *err)
 * \endcode
 * Arguments:
 * - s: socket file descriptor
 * - sock: internal socket descriptor (see lwip/priv/sockets_priv.h)
 * - level: protocol level at which the option resides
 * - optname: option to get
 * - optval: value to get
 * - optlen: size of optval
 * - err: output error
 * Return values:
 * - 0: Hook has not consumed the option, code continues as normal (to internal options)
 * - != 0: Hook has consumed the option, 'err' is returned
 */
// #define LWIP_HOOK_SOCKETS_GETSOCKOPT(s, sock, level, optname, optval, optlen, err)

/**
 * LWIP_HOOK_NETCONN_EXTERNAL_RESOLVE(name, addr, addrtype, err)
 * Called from netconn APIs (not usable with callback apps) allowing an
 * external DNS resolver (which uses sequential API) to handle the query.
 * Signature:\code{.c}
 *   int my_hook(const char *name, ip_addr_t *addr, u8_t addrtype, err_t *err)
 * \endcode
 * Arguments:
 * - name: hostname to resolve
 * - addr: output host address
 * - addrtype: type of address to query
 * - err: output error
 * Return values:
 * - 0: Hook has not consumed hostname query, query continues into DNS module
 * - != 0: Hook has consumed the query
 *
 * err must also be checked to determine if the hook consumed the query, but
 * the query failed
 */
// #define LWIP_HOOK_NETCONN_EXTERNAL_RESOLVE(name, addr, addrtype, err)

#endif /* HAL_LWIP_LWIPHOOKS_H */
