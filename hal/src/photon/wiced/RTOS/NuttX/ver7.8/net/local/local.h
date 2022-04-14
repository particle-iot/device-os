/****************************************************************************
 * net/local/loal.h
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __NET_LOCAL_LOCAL_H
#define __NET_LOCAL_LOCAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/un.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <queue.h>
#include <stdint.h>

#ifdef CONFIG_NET_LOCAL

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Not yet any support for poll/select operations on Unix domain sockets */

#undef HAVE_LOCAL_POLL

/* Packet format in FIFO:
 *
 * 1. Sync bytes (7 at most)
 * 2. End/Start byte
 * 3. 16-bit packet length (in host order)
 * 4. Packet data (in host order)
 */

#define LOCAL_SYNC_BYTE   0x42     /* Byte in sync sequence */
#define LOCAL_END_BYTE    0xbd     /* End of sync seqence */

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/
/* Local, Unix domain socket types */

enum local_type_e
{
  LOCAL_TYPE_UNTYPED = 0,      /* Type is not determined until the socket is bound */
  LOCAL_TYPE_UNNAMED,          /* A Unix socket that is not bound to any name */
  LOCAL_TYPE_PATHNAME,         /* lc_path holds a null terminated string */
  LOCAL_TYPE_ABSTRACT          /* lc_path is length zero */
};

/* The state of a Unix socket */

enum local_state_s
{
  /* Common states */

  LOCAL_STATE_UNBOUND = 0,     /* Created by socket, but not bound */
  LOCAL_STATE_BOUND,           /* Bound to an path */

  /* SOCK_STREAM server only */

  LOCAL_STATE_LISTENING,       /* Server listening for connections */

  /* SOCK_STREAM peers only */

  LOCAL_STATE_ACCEPT,          /* Client waiting for a connection */
  LOCAL_STATE_CONNECTED,       /* Peer connected */
  LOCAL_STATE_DISCONNECTED     /* Peer disconnected */
};

/* Representation of a local connection.  There are four types of
 * connection structures:
 *
 * 1. Server.  A SOCK_STREAM that only listens for and accepts
 *    connections from server.
 * 2. Client.  A SOCK_STREAM peer that connects via the server.
 * 3. Peer. A connected SOCK_STREAM that sends() and recvs() packets.
 *    May either be the client that connect with the server of the
 *    new peer connect generated with the connection was accepted by
 *    the server.
 *
 * And
 *
 * 4. Connectionless.  Like a peer but using a connectionless datagram
 *    style of communication.  SOCK_DRAM support has not yet been
 *    implemented.
 */

struct local_conn_s
{
  /* lc_node supports a doubly linked list: Listening SOCK_STREAM servers
   * will be linked into a list of listeners; SOCK_STREAM clients will be
   * linked to the lc_waiters and lc_conn lists.
   */

  dq_entry_t lc_node;          /* Supports a doubly linked list */

  /* Fields common to SOCK_STREAM and SOCK_DGRAM */

  uint8_t lc_crefs;            /* Reference counts on this instance */
  uint8_t lc_proto;            /* SOCK_STREAM or SOCK_DGRAM */
  uint8_t lc_type;             /* See enum local_type_e */
  uint8_t lc_state;            /* See enum local_state_e */
  int16_t lc_infd;             /* File descriptor of read-only FIFO (peers) */
  int16_t lc_outfd;            /* File descriptor of write-only FIFO (peers) */
  char lc_path[UNIX_PATH_MAX]; /* Path assigned by bind() */

#ifdef CONFIG_NET_LOCAL_STREAM
  /* SOCK_STREAM fields common to both client and server */

  sem_t lc_waitsem;            /* Use to wait for a connection to be accepted */

  /* Union of fields unique to SOCK_STREAM client, server, and connected
   * peers.
   */

  union
  {
    /* Fields unique to the SOCK_STREAM server side */

    struct
    {
      uint8_t lc_pending;      /* Number of pending connections */
      uint8_t lc_backlog;      /* Maximum number of pending connections */
      dq_queue_t lc_waiters;   /* List of connections waiting to be accepted */
    } server;

    /* Fields unique to the connecting client side */

    struct
    {
      uint16_t lc_remaining;   /* (For binary compatibility with peer) */
      volatile int lc_result;  /* Result of the connection operation (client)*/
    } client;

    /* Fields common to connected peers (connected or accepted) */

    struct
    {
      uint16_t lc_remaining;   /* Bytes remaining in the incoming stream */
    } peer;
  } u;
#endif /* CONFIG_NET_LOCAL_STREAM */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#  define EXTERN extern "C"
extern "C"
{
#else
#  define EXTERN extern
#endif

#ifdef CONFIG_NET_LOCAL_STREAM
/* A list of all SOCK_STREAM listener connections */

EXTERN dq_queue_t g_local_listeners;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct sockaddr; /* Forward reference */
struct socket;   /* Forward reference */

/****************************************************************************
 * Name: local_initialize
 *
 * Description:
 *   Initialize the local, Unix domain connection structures.  Called once
 *   and only from the common network initialization logic.
 *
 ****************************************************************************/

void local_initialize(void);

/****************************************************************************
 * Name: local_alloc
 *
 * Description:
 *   Allocate a new, uninitialized local connection structure.  This is
 *   normally something done by the implementation of the socket() API
 *
 ****************************************************************************/

FAR struct local_conn_s *local_alloc(void);

/****************************************************************************
 * Name: local_free
 *
 * Description:
 *   Free a local connection structure that is no longer in use. This should
 *   be done by the implementation of close().
 *
 ****************************************************************************/

void local_free(FAR struct local_conn_s *conn);

/****************************************************************************
 * Name: psock_local_bind
 *
 * Description:
 *   This function implements the low-level parts of the standard local
 *   bind()operation.
 *
 ****************************************************************************/

int psock_local_bind(FAR struct socket *psock,
                     FAR const struct sockaddr *addr, socklen_t addrlen);

/****************************************************************************
 * Name: psock_local_connect
 *
 * Description:
 *   This function sets up a new local connection. The function will
 *   automatically allocate an unused local port for the new
 *   connection. However, another port can be chosen by using the
 *   psock_local_bind() call, after the psock_local_connect() function has
 *   been called.
 *
 *   This function is called as part of the implementation of sendto
 *   and recvfrom.
 *
 * Input Parameters:
 *   psock - A reference to the client-side socket structure
 *   addr - The address of the remote host.
 *
 ****************************************************************************/

int psock_local_connect(FAR struct socket *psock,
                        FAR const struct sockaddr *addr);

/****************************************************************************
 * Name: local_release
 *
 * Description:
 *   If the local, Unix domain socket is in the connected state, then
 *   disconnect it.  Release the local connection structure in any event
 *
 * Input Parameters:
 *   conn - A reference to local connection structure
 *
 ****************************************************************************/

int local_release(FAR struct local_conn_s *conn);

/****************************************************************************
 * Name: local_listen
 *
 * Description:
 *   Listen for a new connection of a SOCK_STREAM Unix domain socket.
 *
 *   This function is called as part of the implementation of listen();
 *
 * Input Parameters:
 *   server  - A reference to the server-side local connection structure
 *   backlog - Maximum number of pending connections.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 * Assumptions:
 *   The network is NOT locked
 *
 ****************************************************************************/

int local_listen(FAR struct local_conn_s *server, int backlog);

/****************************************************************************
 * Function: psock_local_accept
 *
 * Description:
 *   This function implements accept() for Unix domain sockets.  See the
 *   description of accept() for further information.
 *
 * Parameters:
 *   psock    The listening Unix domain socket structure
 *   addr     Receives the address of the connecting client
 *   addrlen  Input: allocated size of 'addr', Return: returned size of 'addr'
 *   newconn  The new, accepted  Unix domain connection structure
 *
 * Returned Value:
 *   Returns zero (OK) on success or a negated errno value on failure.
 *   See the description of accept of the possible errno values in the
 *   description of accept().
 *
 * Assumptions:
 *   Network is NOT locked.
 *
 ****************************************************************************/

int psock_local_accept(FAR struct socket *psock, FAR struct sockaddr *addr,
                       FAR socklen_t *addrlen, FAR void **newconn);

/****************************************************************************
 * Name: psock_local_send
 *
 * Description:
 *   Send a local packet as a stream.
 *
 * Parameters:
 *   psock    An instance of the internal socket structure.
 *   buf      Data to send
 *   len      Length of data to send
 *   flags    Send flags (ignored for now)
 *
 * Return:
 *   On success, returns the number of characters sent.  On  error,
 *   -1 is returned, and errno is set appropriately (see send() for the
 *   list of errno numbers).
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_STREAM
ssize_t psock_local_send(FAR struct socket *psock, FAR const void *buf,
                         size_t len, int flags);
#endif

/****************************************************************************
 * Function: psock_local_sendto
 *
 * Description:
 *   This function implements the Unix domain-specific logic of the
 *   standard sendto() socket operation.
 *
 * Input Parameters:
 *   psock    A pointer to a NuttX-specific, internal socket structure
 *   buf      Data to send
 *   len      Length of data to send
 *   flags    Send flags
 *   to       Address of recipient
 *   tolen    The length of the address structure
 *
 *   NOTE: All input parameters were verified by sendto() before this
 *   function was called.
 *
 * Returned Value:
 *   On success, returns the number of characters sent.  On  error,
 *   a negated errno value is returned.  See the description in
 *   net/socket/sendto.c for the list of appropriate return value.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_DGRAM
ssize_t psock_local_sendto(FAR struct socket *psock, FAR const void *buf,
                           size_t len, int flags, FAR const struct sockaddr *to,
                           socklen_t tolen);
#endif

/****************************************************************************
 * Name: local_send_packet
 *
 * Description:
 *   Send a packet on the write-only FIFO.
 *
 * Parameters:
 *   fd       File descriptor of write-only FIFO.
 *   buf      Data to send
 *   len      Length of data to send
 *
 * Return:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

int local_send_packet(int fd, FAR const uint8_t *buf, size_t len);

/****************************************************************************
 * Function: psock_recvfrom
 *
 * Description:
 *   recvfrom() receives messages from a local socket, and may be used to
 *   receive data on a socket whether or not it is connection-oriented.
 *
 *   If from is not NULL, and the underlying protocol provides the source
 *   address, this source address is filled in. The argument fromlen
 *   initialized to the size of the buffer associated with from, and modified
 *   on return to indicate the actual size of the address stored there.
 *
 * Parameters:
 *   psock    A pointer to a NuttX-specific, internal socket structure
 *   buf      Buffer to receive data
 *   len      Length of buffer
 *   flags    Receive flags (ignored for now)
 *   from     Address of source (may be NULL)
 *   fromlen  The length of the address structure
 *
 * Returned Value:
 *   On success, returns the number of characters sent.  If no data is
 *   available to be received and the peer has performed an orderly shutdown,
 *   recv() will return 0.  Otherwise, on errors, -1 is returned, and errno
 *   is set appropriately (see receivefrom for the complete list).
 *
 ****************************************************************************/

ssize_t psock_local_recvfrom(FAR struct socket *psock, FAR void *buf,
                             size_t len, int flags, FAR struct sockaddr *from,
                             FAR socklen_t *fromlen);

/****************************************************************************
 * Name: local_fifo_read
 *
 * Description:
 *   Read a data from the read-only FIFO.
 *
 * Parameters:
 *   fd  - File descriptor of read-only FIFO.
 *   buf - Local to store the received data
 *   len - Length of data to receive [in]
 *         Length of data actually received [out]
 *
 * Return:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.  If -ECONNRESET is received, then the sending side has closed
 *   the FIFO. In this case, the returned data may still be valid (if the
 *   returned len > 0).
 *
 ****************************************************************************/

int local_fifo_read(int fd, FAR uint8_t *buf, size_t *len);

/****************************************************************************
 * Name: local_getaddr
 *
 * Description:
 *   Return the Unix domain address of a connection.
 *
 * Parameters:
 *   conn - The connection
 *   addr - The location to return the address
 *   addrlen - The size of the memory allocat by the caller to receive the
 *             address.
 *
 * Return:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int local_getaddr(FAR struct local_conn_s *conn, FAR struct sockaddr *addr,
                  FAR socklen_t *addrlen);

/****************************************************************************
 * Name: local_sync
 *
 * Description:
 *   Read a sync bytes until the start of the packet is found.
 *
 * Parameters:
 *   fd - File descriptor of read-only FIFO.
 *
 * Return:
 *   The non-zero size of the following packet is returned on success; a
 *   negated errno value is returned on any failure.
 *
 ****************************************************************************/

int local_sync(int fd);

/****************************************************************************
 * Name: local_create_fifos
 *
 * Description:
 *   Create the FIFO pair needed for a SOCK_STREAM connection.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_STREAM
int local_create_fifos(FAR struct local_conn_s *conn);
#endif

/****************************************************************************
 * Name: local_create_halfduplex
 *
 * Description:
 *   Create the half-duplex FIFO needed for SOCK_DGRAM communication.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_DGRAM
int local_create_halfduplex(FAR struct local_conn_s *conn,
                            FAR const char *path);
#endif

/****************************************************************************
 * Name: local_release_fifos
 *
 * Description:
 *   Release references to the FIFO pair used for a SOCK_STREAM connection.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_STREAM
int local_release_fifos(FAR struct local_conn_s *conn);
#endif

/****************************************************************************
 * Name: local_release_halfduplex
 *
 * Description:
 *   Release a reference to the FIFO used for SOCK_DGRAM communication
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_DGRAM
int local_release_halfduplex(FAR struct local_conn_s *conn);
#endif

/****************************************************************************
 * Name: local_open_client_rx
 *
 * Description:
 *   Only the client-side Rx FIFO.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_STREAM
int local_open_client_rx(FAR struct local_conn_s *client, bool nonblock);
#endif

/****************************************************************************
 * Name: local_open_client_tx
 *
 * Description:
 *   Only the client-side Tx FIFO.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_STREAM
int local_open_client_tx(FAR struct local_conn_s *client, bool nonblock);
#endif

/****************************************************************************
 * Name: local_open_server_rx
 *
 * Description:
 *   Only the server-side Rx FIFO.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_STREAM
int local_open_server_rx(FAR struct local_conn_s *server, bool nonblock);
#endif

/****************************************************************************
 * Name: local_open_server_tx
 *
 * Description:
 *   Only the server-side Tx FIFO.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_STREAM
int local_open_server_tx(FAR struct local_conn_s *server, bool nonblock);
#endif

/****************************************************************************
 * Name: local_open_receiver
 *
 * Description:
 *   Only the receiving side of the half duplex FIFO.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_DGRAM
int local_open_receiver(FAR struct local_conn_s *conn, bool nonblock);
#endif

/****************************************************************************
 * Name: local_open_sender
 *
 * Description:
 *   Only the sending side of the half duplex FIFO.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_LOCAL_DGRAM
int local_open_sender(FAR struct local_conn_s *conn, FAR const char *path,
                      bool nonblock);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_NET_LOCAL */
#endif /* __NET_LOCAL_LOCAL_H */
