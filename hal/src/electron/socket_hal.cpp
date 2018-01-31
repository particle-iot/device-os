#ifndef HAL_CELLULAR_EXCLUDE

#include <stdint.h>
#include "socket_hal.h"
#include "parser.h"

const sock_handle_t SOCKET_MAX = (sock_handle_t)7; // 7 total sockets, handle 0-6
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;


sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif)
{
    sock_handle_t handle = electronMDM.socketSocket(protocol==IPPROTO_TCP ? MDM_IPPROTO_TCP : MDM_IPPROTO_UDP, port);
    electronMDM.socketSetBlocking(handle, 0);
    return handle;
}

int32_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    const uint8_t* addr_data = addr->sa_data;
    uint16_t port = addr_data[0]<<8 | addr_data[1];
    MDM_IP ip = IPADR(addr_data[2], addr_data[3], addr_data[4], addr_data[5]);
    electronMDM.socketSetBlocking(sd, 5000);
    bool result = electronMDM.socketConnect(sd, ip, port);
    electronMDM.socketSetBlocking(sd, 0);
    return (result ? 0 : 1);
}

sock_result_t socket_reset_blocking_call()
{
    return 0;
}

sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{
    electronMDM.socketSetBlocking(sd, _timeout);
    sock_result_t result = 0;
    if (_timeout==0) {
    		result = electronMDM.socketReadable(sd);
    		if (result==0)		// no data, so return without polling for data
    			return 0;
    		if (result>0)		// clear error
    			result = 0;
    }
    if (!result)
    		result = electronMDM.socketRecv(sd, (char*)buffer, len);
    return result;
}

sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port)
{
    return -1;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize)
{
    int port;
    MDM_IP ip;

    sock_result_t result = electronMDM.socketReadable(sock);
	if (result<=0)			// error or no data
		return result;

	// have some data to let's get it.
	result = electronMDM.socketRecvFrom(sock, &ip, &port, (char*)buffer, bufLen);
    if (result > 0) {
        uint32_t ipv4 = ip;
        addr->sa_data[0] = (port>>8) & 0xFF;
        addr->sa_data[1] = port & 0xFF;
        addr->sa_data[2] = (ipv4 >> 24) & 0xFF;
        addr->sa_data[3] = (ipv4 >> 16) & 0xFF;
        addr->sa_data[4] = (ipv4 >> 8) & 0xFF;
        addr->sa_data[5] = ipv4 & 0xFF;
    }
    return result;
}

sock_result_t socket_accept(sock_handle_t sock)
{
    return 0;
}

uint8_t socket_active_status(sock_handle_t socket)
{
    bool result = electronMDM.socketIsConnected(socket);
    return (result ? 0 : 1);
}

sock_result_t socket_close(sock_handle_t sock)
{
    bool result = electronMDM.socketFree(sock); // closes and frees the socket
    return (result ? 0 : 1);
}

sock_result_t socket_shutdown(sock_handle_t sd, int how)
{
    return -1;
}

sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len)
{
    return electronMDM.socketSend(sd, (const char*)buffer, len);
}

sock_result_t socket_send_ex(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, system_tick_t timeout, void* reserved)
{
    /* NOTE: non-blocking mode and timeouts are not supported */
    return socket_send(sd, buffer, len);
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size)
{
    const uint8_t* addr_data = addr->sa_data;
    uint16_t port = addr_data[0]<<8 | addr_data[1];
    MDM_IP ip = IPADR(addr_data[2], addr_data[3], addr_data[4], addr_data[5]);
    return electronMDM.socketSendTo(sd, ip, port, (const char*)buffer, len);
}

inline bool is_valid(sock_handle_t handle)
{
    return handle<SOCKET_MAX;
}

uint8_t socket_handle_valid(sock_handle_t handle)
{
    return is_valid(handle);
}

sock_handle_t socket_handle_invalid()
{
    return SOCKET_INVALID;
}

sock_result_t socket_join_multicast(const HAL_IPAddress* addr, network_interface_t nif, socket_multicast_info_t* reserved)
{
    return -1;
}

sock_result_t socket_leave_multicast(const HAL_IPAddress* addr, network_interface_t nif, socket_multicast_info_t* reserved)
{
    return -1;
}

sock_result_t socket_peer(sock_handle_t sd, sock_peer_t* peer, void* reserved)
{
    return -1;
}

sock_result_t socket_create_tcp_server(uint16_t port, network_interface_t nif)
{
    return -1;
}

#endif // !defined(HAL_CELLULAR_EXCLUDE)
