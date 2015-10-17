
#include <stdint.h>
#include "socket_hal.h"
#include "parser.h"

const sock_handle_t SOCKET_MAX = (sock_handle_t)7;
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;


sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif)
{
    return electronMDM.socketSocket(protocol==IPPROTO_TCP ? ElectronMDM::MDM_IPPROTO_TCP : ElectronMDM::MDM_IPPROTO_UDP, port);
}

int32_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    const uint8_t* addr_data = addr->sa_data;
    uint16_t port = addr_data[0]<<8 | addr_data[1];
    ElectronMDM::IP ip = IPADR(addr_data[2], addr_data[3], addr_data[4], addr_data[5]);
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
    return electronMDM.socketRecv(sd, (char*)buffer, len);
}

sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port)
{
    return -1;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize)
{
    return 0;
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
    bool result = electronMDM.socketClose(sock);
    electronMDM.socketFree(sock);
    return (result ? 0 : 1);
}

sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len)
{
    return electronMDM.socketSend(sd, (const char*)buffer, len);
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size)
{
    return 0;
}

inline bool is_valid(sock_handle_t handle)
{
    return handle<=SOCKET_MAX;
}

uint8_t socket_handle_valid(sock_handle_t handle)
{
    return is_valid(handle);
}

sock_handle_t socket_handle_invalid()
{
    return SOCKET_INVALID;
}

sock_result_t socket_join_multicast(const HAL_IPAddress* addr, network_interface_t nif, void* reserved)
{
    return -1;
}

sock_result_t socket_leave_multicast(const HAL_IPAddress* addr, network_interface_t nif, void* reserved)
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