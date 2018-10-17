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

/**
 * @file
 * @brief
 *  This file defines the POSIX-compatible socket_hal and associated types.
 */

#ifndef SOCKET_HAL_POSIX_H
#define SOCKET_HAL_POSIX_H

#include "socket_hal_posix_impl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup socket_hal
 *
 * @brief
 *   This module provides POSIX-compatible socket functions.
 *
 * @{
 *
 */

/** Compatibility sock_handle_t */
typedef int sock_handle_t;
/** Compatibility sock_result_t */
typedef int sock_result_t;
/** Compatibility socket_handle_valid() macro */
#define socket_handle_valid(x) ((x) >= 0)
/** Compatibility SOCKET_WAIT_FOREVER definition */
#define SOCKET_WAIT_FOREVER (0xffffffff)

/**
 * Accept a connection on a socket.
 *
 * @param[in]     s        a socket that has been created with sock_socket()
 * @param[out]    addr     a pointer to a sockaddr structure, which
 *                         will be filled with the address of the peer
 * @param[inout]  addrlen  length of addr in bytes, on return it will
 *                         contain the actual size of the peer address
 *
 * @returns       On success, a nonnegative descriptor for the accepted socket.
 *                On error, -1 is returned, and errno is set appropriately.
 */
int sock_accept(int s, struct sockaddr* addr, socklen_t* addrlen);

/**
 * Assigns the address (name) specified by name to the socket s.
 *
 * @param[in]  s        a socket that has been created with sock_socket()
 * @param[in]  name     a pointer to a sockaddr structure
 * @param[in]  namelen  length of name in bytes
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_bind(int s, const struct sockaddr* name, socklen_t namelen);

/**
 * Shut down part of a full-duplex connection.
 *
 * If how is SHUT_RD, no more data can be received.
 * If how is SHUT_WR, no more data can be transmitted.
 * If how is SHUT_RDWR, no more data can be received or transmitted.
 *
 * @param[in]  s     a socket that has been created with sock_socket()
 * @param[in]  how   SHUT_RD, SHUT_WR or SHUT_RDWR
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_shutdown(int s, int how);

/**
 * Get an address of the connected peer.
 *
 * @param[in]    s        a socket that has been created with sock_socket()
 * @param[out]   name     a pointer to a sockaddr structure, which
 *                        will be filled with the address of the peer
 * @param[inout] namelen  length of name in bytes, on return it will
 *                        contain the actual size of the peer address
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_getpeername(int s, struct sockaddr* name, socklen_t* namelen);

/**
 * Get the current address for the specified socket.
 *
 * @param[in]    s        a socket that has been created with sock_socket()
 * @param[out]   name     a pointer to a sockaddr structure, which
 *                        will be filled with the local socket address
 * @param[inout] namelen  length of name in bytes, on return it will
 *                        contain the actual size of the local address
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_getsockname(int s, struct sockaddr* name, socklen_t* namelen);

/**
 * Retrieves the value of the option.
 *
 * @param[in]    s        a socket that has been created with sock_socket()
 * @param[in]    level    protocol level
 * @param[in]    optname  the option name
 * @param[out]   optval   the option value
 * @param[inout] optlen   the option value length, on return it will
 *                        contain the actual size of the option value
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen);

/**
 * Sets the value of the option.
 *
 * @param[in]  s        a socket that has been created with sock_socket()
 * @param[in]  level    protocol level
 * @param[in]  optname  the option name
 * @param[in]  optval   the option value
 * @param[in]  optlen   the option value length
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen);

/**
 * Closes the socket.
 *
 * @param[in]  s     a socket that has been created with sock_socket()
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_close(int s);

/**
 * Initiate a connection on a socket.
 *
 * @param[in]  s        a socket that has been created with sock_socket()
 * @param[in]  name     an address of the remote peer
 * @param[in]  namelen  the size of an address of the remote peer
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_connect(int s, const struct sockaddr* name, socklen_t namelen);

/**
 * Listen for connections on a socket.
 *
 * @param[in]  s        a socket that has been created with sock_socket()
 * @param[in]  backlog  maximum number of pending connections
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_listen(int s, int backlog);

/**
 * Receive data from the socket.
 *
 * @param[in]  s      a socket that has been created with sock_socket()
 * @param[out] mem    the receive buffer
 * @param[in]  len    the length of the receive buffer
 * @param[in]  flags  a combination of MSG_DONTWAIT, MSG_PEEK and MSG_TRUNC
 *
 * @returns    The number of bytes received or -1 on error, with errno set accordingly.
 */
ssize_t sock_recv(int s, void* mem, size_t len, int flags);

/**
 * Receive a message from the socket.
 *
 * @param[in]    s        a socket that has been created with sock_socket()
 * @param[out]   mem      the receive buffer
 * @param[in]    len      the length of the receive buffer
 * @param[in]    flags    The flags
 * @param[out]   from     the source address of the message
 * @param[inout] fromlen  length of source address in bytes, on return it will
 *                        contain the actual size of the source address
 *
 * @returns    The number of bytes received or -1 on error, with errno set accordingly.
 */
ssize_t sock_recvfrom(int s, void* mem, size_t len, int flags,
                      struct sockaddr* from, socklen_t* fromlen);

/**
 * Send the data through the socket.
 *
 * @param[in]  s        a socket that has been created with sock_socket()
 * @param[in]  dataptr  the data buffer to send
 * @param[in]  size     the size of the data buffer
 * @param[in]  flags    a combination of MSG_MORE and MSG_DONTWAIT
 *
 * @returns    The number of bytes sent or -1 on error, with errno set accordingly.
 */
ssize_t sock_send(int s, const void* dataptr, size_t size, int flags);

/**
 * Send a message through a socket.
 *
 * @param[in]  s        a socket that has been created with sock_socket()
 * @param[in]  dataptr  the message to send
 * @param[in]  size     the size of the message
 * @param[in]  flags    a combination of MSG_MORE and MSG_DONTWAIT
 * @param[in]  to       target address
 * @param[in]  tolen    target address length
 *
 * @returns    The number of bytes sent or -1 on error, with errno set accordingly.
 */
ssize_t sock_sendto(int s, const void* dataptr, size_t size, int flags,
                    const struct sockaddr* to, socklen_t tolen);

/**
 * Create an endpoint for communication - a socket.
 *
 * @param[in]  domain    the protocol family
 * @param[in]  type      the socket type: SOCK_RAW, SOCK_STREAM or SOCK_DGRAM
 * @param[in]  protocol  a particular protocol to be used with socket
 *
 * @returns    The socket descriptor on success, or -1 on error, with errno set accordingly.
 */
int sock_socket(int domain, int type, int protocol);

/**
 * Manipulates socket descriptor.
 *
 * @param[in]  s        a socket that has been created with sock_socket()
 * @param[in]  cmd      an operation type
 * @param[in]  ...      (optional) argument
 *
 * @retval  0  Success
 * @retval -1  Error, errno is set appropriately.
 */
int sock_fcntl(int s, int cmd, ...);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SOCKET_HAL_POSIX_H */
