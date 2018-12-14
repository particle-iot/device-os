/**
 ******************************************************************************
 * @file    socket_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    09-Nov-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "socket_hal.h"
#include "wiced.h"
#include "service_debug.h"
#include "spark_macros.h"
#include "delay_hal.h"
#include <algorithm>
#include <vector>
#include "lwip/api.h"
#include "network_interface.h"
#include "spark_wiring_thread.h"
#include "spark_wiring_vector.h"

wiced_result_t wiced_last_error( wiced_tcp_socket_t* socket);

/**
 * Socket handles
 * --------------
 *
 * Each socket handle is a pointer to a dynamically allocated instance of socket_t.
 * This is so we don't impose any additional limits on the number of open sockets.
 *
 * The golden rule is that the socket_t instance is not deallocated until the caller
 * issues a socket_close() call. Specifically, if a client socket is closed by the other end,
 * the handle remains valid, although attempts to perform any socket IO will fail.
 * The handle isn't deallocated until the caller issues a socket_close() call.
 */

/**
 * int32_t negative values are used for errors.
 * Since all handles are allocated in RAM, they will be in the
 * 0x20xxxxxx range.
 */
const sock_handle_t SOCKET_MAX = 0x7FFFFFFF;

/**
 * The handle value returned when a socket cannot be created.
 */
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;

/* Normalize differences between LwIP and NetX */

#ifndef WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS
// the name of the array of sockets in wiced_tcp_server_t
#define WICED_SOCKET_ARRAY accept_socket

// the number of sockets in the above array
#define WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS WICED_MAXIMUM_NUMBER_OF_ACCEPT_SOCKETS
#else
#define WICED_SOCKET_ARRAY socket
#endif

/**
 * Manages reading from a tcp packet.
 */
struct tcp_packet_t
{
    /**
     * Any outstanding packet to retrieve data from.
     */
    wiced_packet_t* packet;

    /**
     * The current offset of data already read from the packet.
     */
    unsigned offset;

    tcp_packet_t() :
            packet(nullptr),
            offset(0) {
    }

    ~tcp_packet_t() {
        dispose_packet();
    }

    void dispose_packet() {
        if (packet) {
            wiced_packet_delete(packet);
            packet = NULL;
            offset = 0;
        }
    }

};

/**
 * The info we maintain for each socket. It wraps a WICED socket.
 *
 */
struct tcp_socket_t : public wiced_tcp_socket_t {
    tcp_packet_t packet;
    bool open;
    volatile bool closed_externally;

    tcp_socket_t() : open(false), closed_externally(false) {}

    ~tcp_socket_t() {}

    void connected() { open = true; }

    bool isClosed() {
        wiced_result_t last_err = wiced_last_error(this);
        return !open ||
            closed_externally ||
            last_err == WICED_CONNECTION_RESET ||
            last_err == WICED_CONNECTION_CLOSED;
    }

    void notify_disconnected()
    {
        closed_externally = true;
    }

    void close()
    {
        if (open) {
            wiced_tcp_disconnect(this);
            open = false;
        }
        wiced_tcp_delete_socket(this);
        memset(this, 0, sizeof(wiced_tcp_socket_t));
        packet.dispose_packet();
    }
};

struct udp_socket_t : wiced_udp_socket_t
{
    ~udp_socket_t() {}

    void close()
    {
        wiced_udp_delete_socket(this);
        memset(this, 0, sizeof(wiced_udp_socket_t));
    }
};

struct tcp_server_t;

/**
 * The handle that we provide to external clients. This ensures
 */
class tcp_server_client_t {

    wiced_tcp_socket_t* socket;
    tcp_server_t* server;
    volatile bool closed_externally;
    bool closed;

public:
    tcp_packet_t packet;

    tcp_server_client_t(tcp_server_t* server, wiced_tcp_socket_t* socket) {
        this->socket = socket;
        this->server = server;
        this->closed = false;
        this->closed_externally = false;
    }

    wiced_tcp_socket_t* get_socket() { return socket; }

    wiced_result_t write(const void* buffer, size_t len, size_t* bytes_written, uint32_t flags, system_tick_t timeout, bool flush=false) {
        wiced_result_t result = WICED_TCPIP_INVALID_SOCKET;
        if (socket && !isClosed()) {
            wiced_tcp_send_flags_t wiced_flags = timeout == 0 ? WICED_TCP_SEND_FLAG_NONBLOCK : WICED_TCP_SEND_FLAG_NONE;
            uint16_t bytes_sent = (uint16_t)len;
            result = wiced_tcp_send_buffer_ex(socket, buffer, &bytes_sent, wiced_flags, timeout);
            *bytes_written = bytes_sent;
        }
        return result;
    }

    bool isClosed() {
        return closed || closed_externally;
    }

    void close();

    /**
     * Notification from the server that this socket has been disconnected. The server is
     * already in the process of cleaning up the socket.
     */
    void notify_disconnected()
    {
        // flag that this is closed, but don't clean up, since this call is on the
        // wiced networking worker thread. we want all calls to be on one thread, so service
        // the close on the enxt call to isClosed() by the client.
        closed_externally = true;
    }

    ~tcp_server_client_t();
};

struct tcp_server_t : public wiced_tcp_server_t
{
    tcp_server_t() {
        os_mutex_recursive_create(&accept_lock);
        memset(clients, 0, sizeof(clients));
    }

    ~tcp_server_t() {
        os_mutex_recursive_destroy(accept_lock);
    }

    /**
     * Find the index of the given client socket in our list of client sockets.
     * @param socket The socket to find.
     * @return The index of the socket (>=0) or -1 if not found.
     */
    int index(wiced_tcp_socket_t* socket) {
        return (is_client(socket)) ? socket-this->WICED_SOCKET_ARRAY : -1;
    }

    /**
     * Determines if the given socket is a client socket associated with this server
     * socket.
     * @param socket
     * @return {@code true} if the given socket is a client.
     */
    bool is_client(wiced_tcp_socket_t* socket) {
        // see if the address corresponds to the socket array
        return this->WICED_SOCKET_ARRAY<=socket && socket<this->WICED_SOCKET_ARRAY+arraySize(this->WICED_SOCKET_ARRAY);
    }

    wiced_result_t accept(wiced_tcp_socket_t* socket) {
        wiced_result_t result;
        if ((result=wiced_tcp_accept(socket))==WICED_SUCCESS) {
            os_mutex_recursive_lock(accept_lock);

            int idx = index(socket);
            if (idx>=0) {
                clients[idx] = new tcp_server_client_t(this, socket);
                to_accept.append(idx);
            }
            os_mutex_recursive_unlock(accept_lock);
        }
        return result;
    }

    /**
     * Fetches the next client socket from the accept queue.
     * @return The next client, or NULL
     */
    tcp_server_client_t* next_accept() {
        os_mutex_recursive_lock(accept_lock);
        int index = -1;
        if (to_accept.size()) {
            index = *to_accept.begin();
            to_accept.removeAt(0);
        }
        os_mutex_recursive_unlock(accept_lock);
        return index>=0 ? clients[index] : NULL;
    }

    /**
     * Asynchronous notification that the client socket has been closed.
     * @param socket
     * @return
     */
    wiced_result_t notify_disconnected(wiced_tcp_socket_t* socket) {
        os_mutex_recursive_lock(accept_lock);
        int idx = index(socket);
        tcp_server_client_t* client = clients[idx];
        if (client)
            client->notify_disconnected();
        os_mutex_recursive_unlock(accept_lock);
        return WICED_SUCCESS;
    }

    /**
     * Called from the client to disconnect this server.
     * @param socket
     * @return
     */
    wiced_result_t disconnect(wiced_tcp_socket_t* socket) {
        os_mutex_recursive_lock(accept_lock);
        wiced_tcp_disconnect(socket);
        // remove from client array as this socket is getting closed and
        // subsequently destroyed
	    int idx = index(socket);
	    if (idx >= 0) {
		  clients[idx] = NULL;
	    }
        wiced_result_t result = wiced_tcp_server_disconnect_socket(this, socket);
        os_mutex_recursive_unlock(accept_lock);
        return result;
    }

    void close() {
        os_mutex_recursive_lock(accept_lock);
        // close all clients first
        for (int i=0; i<WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS; i++) {
            tcp_server_client_t* client = clients[i];
            if (client) {
                client->close();
                clients[i] = NULL;
            }
        }
        wiced_tcp_server_stop(this);
        memset(this, 0, sizeof(wiced_tcp_server_t));
        os_mutex_recursive_unlock(accept_lock);
    }


private:
    // for each server instance, maintain an associated tcp_server_client_t instance
    tcp_server_client_t* clients[WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS];

    os_mutex_recursive_t accept_lock;
    spark::Vector<int> to_accept;
};

void tcp_server_client_t::close() {
    if (!closed) {
        closed = true;
        if (socket && server) {
            server->disconnect(socket);
            server = nullptr;
        }
    }
}

tcp_server_client_t::~tcp_server_client_t() {
    packet.dispose_packet();
}

class socket_t
{
    uint8_t type;
    bool closed;
    os_mutex_recursive_t mutex_ = nullptr;

public:

    union all {
        tcp_socket_t tcp;
        udp_socket_t udp;
        tcp_server_t* tcp_server;
        tcp_server_client_t* tcp_client;

        all() {}
        ~all() {}
    } s;

    socket_t* next;
    enum socket_type_t {
        NONE, TCP, UDP, TCP_SERVER, TCP_CLIENT
    };

    socket_t() {
        memset(this, 0, sizeof(*this));
        os_mutex_recursive_create(&mutex_);
    }

    ~socket_t() {
        dispose();
        os_mutex_recursive_destroy(mutex_);
    }

    uint8_t get_type() { return type; }
    bool is_type(socket_type_t t) { return t==type; }
    void set_type(socket_type_t t) { type = t; }

   void set_server(tcp_server_t* server) {
        type = TCP_SERVER;
        s.tcp_server = server;
    }

    void set_client(tcp_server_client_t* client) {
        type = TCP_CLIENT;
        s.tcp_client = client;
    }

   void dispose() {
       close();
       switch (type) {
           case TCP:
               s.tcp.~tcp_socket_t();
               break;
            case UDP:
                s.udp.~udp_socket_t();
               break;
           case TCP_SERVER:
               delete s.tcp_server;
               break;
           case TCP_CLIENT:
               delete s.tcp_client;
               break;
       }
       type = NONE;
   }

   void close() {
       if (!closed) {
         switch (type) {
            case TCP:
                s.tcp.close();
                break;
             case UDP:
                s.udp.close();
                break;
            case TCP_SERVER:
                s.tcp_server->close();
                break;
            case TCP_CLIENT:
                s.tcp_client->close();
                break;
        }
        closed = true;
       }
   }

   static wiced_result_t notify_connected(wiced_tcp_socket_t*, void* socket) {
       return WICED_SUCCESS;
   }

   static wiced_result_t notify_received(wiced_tcp_socket_t*, void* socket) {
       return WICED_SUCCESS;
   }

   static wiced_result_t notify_disconnected(wiced_tcp_socket_t*, void* socket);

   /**
    * Determines if the socket implementation is still open.
    * @return
    */
   bool is_inner_open() {
       // TCP_SERVER closes this outer instance, and UDP is not connection based.
       switch (type) {
       case TCP_CLIENT : return !s.tcp_client->isClosed();
       case TCP: return !s.tcp.isClosed();
       default: return true;
       }
   }

   bool isOpen() { return !closed && is_inner_open(); }

   void lock() {
       os_mutex_recursive_lock(mutex_);
   }

   void unlock() {
       os_mutex_recursive_unlock(mutex_);
   }
};

/**
 * Maintains a singly linked list of sockets. Access to the list is not
 * made thread-safe - callers should use SocketListLock to correctly serialize
 * access to the list.
 */
struct SocketList
{
    socket_t* items;
    Mutex mutex;

public:

    SocketList() : items(NULL)
    {
    }

    ~SocketList()
    {
    }

    /**
     * Adds an item to the linked list.
     * @param item
     * @param list
     */
    void add(socket_t* item)
    {
        item->next = items;
        items = item;
    }

    bool exists(socket_t* item)
    {
        bool exists = false;
            socket_t* list = this->items;
        while (list) {
            if (item==list) {
                exists = true;
                break;
            }
            list = list->next;
        }
        return exists;
    }

    /**
     * Removes an item from the linked list.
     * @param item
     * @param list
     */
    bool remove(socket_t* item)
    {
        bool removed = false;
		if (items==item) {
			items = item->next;
			removed = true;
		}
        else
        {
			socket_t* current = items;
            while (current) {
                if (current->next==item) {
                    current->next = item->next;
                    removed = true;
                    break;
                }
                current = current->next;
            }
        }
        return removed;
    }

    void close_all()
    {
        int count = 0;
        for (socket_t* current = items; current != nullptr; current = current->next) {
            count++;
            /* NOTE: socket object should NOT be deleted here. It may only be closed.
             * It's up to the socket 'user' to invoke socket_close() to cleanup its resources
             */
            std::lock_guard<socket_t> lk(*current);
            current->close();
        }
        LOG(TRACE, "%x socket list: %d active sockets closed", this, count);
    }

    friend class SocketListLock;
};

struct SocketListLock : std::lock_guard<Mutex>
{
    SocketListLock(SocketList& list) :
        std::lock_guard<Mutex>(list.mutex)
    {};
};

class ServerSocketList : public SocketList
{
public:
    /**
     *
     * Low-level function to find the server that a given wiced tcp client
     * is associated with. The WICED callbacks provide the client socket, but
     * not the server it is associated with.
     * @param client
     * @return
     */
    tcp_server_t* server_for_socket(wiced_tcp_socket_t* client)
    {
        socket_t* server = items;
        while (server) {
            if (server->s.tcp_server->is_client(client))
                return server->s.tcp_server;
            server = server->next;
    }
        return NULL;
}

};

/**
 * Singly linked lists for servers and clients. Ensures we can completely shutdown
 * the socket layer when bringing down a network interface.
 */
static ServerSocketList servers;
static SocketList clients;

SocketList& list_for(socket_t* socket) {
    return (socket->get_type()==socket_t::TCP_SERVER) ? servers : clients;
}

void add_socket(socket_t* socket) {
    if (socket) {
        list_for(socket).add(socket);
    }
    }

/**
 * Determines if the given socket still exists in the list of current sockets.
 */
bool exists_socket(socket_t* socket) {
    return socket && list_for(socket).exists(socket);
}

inline bool is_udp(socket_t* socket) { return socket && socket->is_type(socket_t::UDP); }
inline bool is_tcp(socket_t* socket) { return socket && socket->is_type(socket_t::TCP); }
inline bool is_client(socket_t* socket) { return socket && socket->is_type(socket_t::TCP_CLIENT); }
inline bool is_server(socket_t* socket) { return socket && socket->is_type(socket_t::TCP_SERVER); }
inline bool is_open(socket_t* socket) { return socket && socket->isOpen(); }
inline tcp_socket_t* tcp(socket_t* socket) { return is_tcp(socket) ? &socket->s.tcp : NULL; }
inline udp_socket_t* udp(socket_t* socket) { return is_udp(socket) ? &socket->s.udp : NULL; }
inline tcp_server_client_t* client(socket_t* socket) { return is_client(socket) ? socket->s.tcp_client : NULL; }
inline tcp_server_t* server(socket_t* socket) { return is_server(socket) ? socket->s.tcp_server : NULL; }

wiced_result_t socket_t::notify_disconnected(wiced_tcp_socket_t*, void* socket) {
    if (socket && 0) {
        // replace with unique_lock once the multithreading changes have been incorporated
        SocketListLock lock(clients);
        if (exists_socket((socket_t*)socket)) {
            tcp_socket_t* tcp_socket = tcp((socket_t*)socket);
            if (tcp_socket)
                tcp_socket->notify_disconnected();
        }
    }
    return WICED_SUCCESS;
}

wiced_tcp_socket_t* as_wiced_tcp_socket(socket_t* socket)
{
    if (is_tcp(socket)) {
        return tcp(socket);
    }
    else if (is_client(socket)) {
        return socket->s.tcp_client->get_socket();
    }
    return NULL;
}

/**
 * Determines if the given socket handle is valid.
 * @param handle    The handle to test
 * @return {@code true} if the socket handle is valid, {@code false} otherwise.
 * Note that this doesn't guarantee the socket can be used, only that the handle
 * is within a valid range. To determine if a handle has an associated socket,
 * use {@link #from_handle}
 */
inline bool is_valid(sock_handle_t handle) {
    return handle<SOCKET_MAX;
}

uint8_t socket_handle_valid(sock_handle_t handle) {
    return is_valid(handle);
}

/**
 * Fetches the socket_t info from an opaque handle.
 * @return The socket_t pointer, or NULL if no socket is available for the
 * given handle.
 */
socket_t* from_handle(sock_handle_t handle) {
    return is_valid(handle) ? (socket_t*)handle : NULL;
}

/**
 * Discards a previously allocated socket. If the socket is already invalid, returns silently.
 * Once a socket has been passed to the client, this is the only time the object is
 * deleted. Since the client initiates this call, the client is aware can the
 * socket is no longer valid.
 * @param handle    The handle to discard.
 * @return SOCKET_INVALID always.
 */
sock_handle_t socket_dispose(sock_handle_t handle) {
    if (socket_handle_valid(handle)) {
        socket_t* socket = from_handle(handle);
        SocketList& list = list_for(socket);
        /* IMPORTANT: SocketListLock is acquired first */
        SocketListLock lock(list);
        std::lock_guard<socket_t> lk(*socket);
        if (list.remove(socket))
            delete socket;
    }
    return SOCKET_INVALID;
}

void close_all(SocketList& list)
{
    SocketListLock lock(list);
    list.close_all();
}

void socket_close_all()
{
    close_all(clients);
    close_all(servers);
}

#define SOCKADDR_TO_PORT_AND_IPADDR(addr, addr_data, port, ip_addr) \
    const uint8_t* addr_data = addr->sa_data; \
    unsigned port = addr_data[0]<<8 | addr_data[1]; \
    wiced_ip_address_t INITIALISER_IPV4_ADDRESS(ip_addr, MAKE_IPV4_ADDRESS(addr_data[2], addr_data[3], addr_data[4], addr_data[5]));

sock_result_t as_sock_result(wiced_result_t result)
{
    return -result;
}

sock_result_t as_sock_result(socket_t* socket)
{
    return (sock_result_t)(socket);
}

/**
 * Connects the given socket to the address.
 * @param sd        The socket handle to connect
 * @param addr      The address to connect to
 * @param addrlen   The length of the address details.
 * @return 0 on success.
 */
sock_result_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    wiced_result_t result = WICED_INVALID_SOCKET;
    socket_t* socket = from_handle(sd);
    tcp_socket_t* tcp_socket = tcp(socket);
    if (tcp_socket) {
        std::lock_guard<socket_t> lk(*socket);
        result = wiced_tcp_bind(tcp_socket, WICED_ANY_PORT);
        if (result==WICED_SUCCESS) {
            // WICED callbacks are broken
            //wiced_tcp_register_callbacks(tcp(socket), socket_t::notify_connected, socket_t::notify_received, socket_t::notify_disconnected, (void*)socket);
            SOCKADDR_TO_PORT_AND_IPADDR(addr, addr_data, port, ip_addr);
            unsigned timeout = 5*1000;
            result = wiced_tcp_connect(tcp_socket, &ip_addr, port, timeout);
            if (result==WICED_SUCCESS) {
                tcp_socket->connected();
            } else {
              // Work around WICED bug that doesn't set connection handler to NULL after deleting
              // it, leading to deleting the same memory twice and a crash
              // WICED/network/LwIP/WICED/tcpip.c:920
              tcp_socket->conn_handler = NULL;
            }
        }
    }
    return as_sock_result(result);
}

/**
 * Is there any way to unblock a blocking call on WICED? Perhaps shutdown the networking layer?
 * @return
 */
sock_result_t socket_reset_blocking_call()
{
    return 0;
}

wiced_result_t read_packet(wiced_packet_t* packet, uint8_t* target, uint16_t target_len, uint16_t* read_len)
{
    uint16_t read = 0;
    wiced_result_t result = WICED_SUCCESS;
    uint16_t fragment;
    uint16_t available;
    uint8_t* data;

    while (target_len!=0 && (result = wiced_packet_get_data(packet, read, &data, &fragment, &available))==WICED_SUCCESS && available!=0) {
        uint16_t to_read = std::min(fragment, target_len);
        memcpy(target+read, data, to_read);
        read += to_read;
        target_len -= to_read;
        available -= to_read;
        if (!available)
            break;
    }
    if (read_len!=NULL)
        *read_len = read;
    return result;
}

int read_packet_and_dispose(tcp_packet_t& packet, void* buffer, int len, wiced_tcp_socket_t* tcp_socket, int _timeout)
{
    int bytes_read = 0;
    if (!packet.packet) {
        packet.offset = 0;
        wiced_result_t result = wiced_tcp_receive(tcp_socket, &packet.packet, _timeout);
        if (result!=WICED_SUCCESS && result!=WICED_TIMEOUT) {
            DEBUG("Socket %d receive fail %d", (int)(int)tcp_socket->socket, int(result));
            return -result;
        }
    }
    uint8_t* data;
    uint16_t available;
    uint16_t total;
    bool dispose = true;
    if (packet.packet && (wiced_packet_get_data(packet.packet, packet.offset, &data, &available, &total)==WICED_SUCCESS)) {
        int read = std::min(uint16_t(len), available);
        packet.offset += read;
        memcpy(buffer, data, read);
        dispose = (total==read);
        bytes_read = read;
        DEBUG("Socket %d receive bytes %d of %d", (int)(int)tcp_socket->socket, int(bytes_read), int(available));
    }
    if (dispose) {
        packet.dispose_packet();
    }
    return bytes_read;
}

/**
 * Receives data from a socket.
 * @param sd
 * @param buffer
 * @param len
 * @param _timeout
 * @return The number of bytes read. -1 if the end of the stream is reached.
 */
sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{
    sock_result_t bytes_read = -1;
    socket_t* socket = from_handle(sd);
    if (is_open(socket)) {
        std::lock_guard<socket_t> lk(*socket);
        if (is_tcp(socket)) {
            tcp_socket_t* tcp_socket = tcp(socket);
            tcp_packet_t& packet = tcp_socket->packet;
            bytes_read = read_packet_and_dispose(packet, buffer, len, tcp_socket, _timeout);
        }
        else if (is_client(socket)) {
            tcp_server_client_t* server_client = client(socket);
            bytes_read = read_packet_and_dispose(server_client->packet, buffer, len, server_client->get_socket(), _timeout);
        }
    }
    if (bytes_read<0)
    		DEBUG("socket_receive on %d returned %d", sd, bytes_read);
    return bytes_read;
}

/**
 * Notification from the networking thread that the given client socket connected
 * to the server.
 * @param socket
 */
wiced_result_t server_connected(wiced_tcp_socket_t* s, void* pv)
{
    SocketListLock lock(servers);
    tcp_server_t* server = servers.server_for_socket(s);
    wiced_result_t result = WICED_ERROR;
    if (server) {
        result = server->accept(s);
    }
    return result;
}

/**
 * Notification that the client socket has data.
 * @param socket
 */
wiced_result_t server_received(wiced_tcp_socket_t* socket, void* pv)
{
    return WICED_SUCCESS;
}

/**
 * Notification that the client socket closed the connection.
 * @param socket
 */
wiced_result_t server_disconnected(wiced_tcp_socket_t* s, void* pv)
{
    SocketListLock lock(servers);
    tcp_server_t* server = servers.server_for_socket(s);
    wiced_result_t result = WICED_ERROR;
    if (server) {
        // disconnect the socket from the server, but maintain the client
        // socket handle.
        result = server->notify_disconnected(s);
    }
    return result;
}

sock_result_t socket_create_tcp_server(uint16_t port, network_interface_t nif)
{
    socket_t* socket = new socket_t();
    tcp_server_t* server = new tcp_server_t();
    wiced_result_t result = WICED_OUT_OF_HEAP_SPACE;
    if (socket && server) {
        result = wiced_tcp_server_start(server, wiced_wlan_interface(nif),
            port, WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS, server_connected, server_received, server_disconnected, NULL);
    }
    if (result!=WICED_SUCCESS) {
        if (socket) {
            delete socket;
            socket = NULL;
        }
        if (server) {
            server->close();
            delete server;
            server = NULL;
        }
    }
    else {
        socket->set_server(server);
        SocketListLock lock(list_for(socket));
        add_socket(socket);
    }

    return socket ? as_sock_result(socket) : as_sock_result(result);
}

/**
 * Fetch the next waiting client socket from the server
 * @param sock
 * @return
 */
sock_result_t socket_accept(sock_handle_t sock)
{
    sock_result_t result = SOCKET_INVALID;
    socket_t* socket = from_handle(sock);
    if (is_open(socket) && is_server(socket)) {
        std::lock_guard<socket_t> lk(*socket);
        tcp_server_t* server = socket->s.tcp_server;
        tcp_server_client_t* client = server->next_accept();
        if (client) {
            socket_t* socket = new socket_t();
            socket->set_client(client);
            {
                SocketListLock lock(list_for(socket));
                add_socket(socket);
            }
            result = (sock_result_t)socket;
        }
    }
    return result;
}

/**
 * Determines if a given socket is bound.
 * @param sd    The socket handle to test
 * @return non-zero if bound, 0 otherwise.
 */
uint8_t socket_active_status(sock_handle_t sd)
{
    socket_t* socket = from_handle(sd);
    return (socket && socket->isOpen()) ? SOCKET_STATUS_ACTIVE : SOCKET_STATUS_INACTIVE;
}

/**
 * Closes the socket handle.
 * @param sock
 * @return
 */
sock_result_t socket_close(sock_handle_t sock)
{
    sock_result_t result = WICED_SUCCESS;
    socket_t* socket = from_handle(sock);
    if (socket) {
        {
            std::lock_guard<socket_t> lk(*socket);
            socket->close();
        }
        socket_dispose(sock);
        LOG_DEBUG(TRACE, "socket closed %x", int(sock));
    }
    return result;
}

sock_result_t socket_shutdown(sock_handle_t sd, int how)
{
    sock_result_t result = WICED_ERROR;
    socket_t* socket = from_handle(sd);
    if (socket && is_open(socket) && is_tcp(socket)) {
        std::lock_guard<socket_t> lk(*socket);
        result = wiced_tcp_close_shutdown(tcp(socket), (wiced_tcp_shutdown_flags_t)how);
        LOG_DEBUG(TRACE, "socket shutdown %x %x", sd, how);
    }
    return result;
}

/**
 * Create a new socket handle.
 * @param family    Must be {@code AF_INET}
 * @param type      Either SOCK_DGRAM or SOCK_STREAM
 * @param protocol  Either IPPROTO_UDP or IPPROTO_TCP
 * @return
 */
sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif)
{
    if (family!=AF_INET || !((type==SOCK_DGRAM && protocol==IPPROTO_UDP) || (type==SOCK_STREAM && protocol==IPPROTO_TCP)))
        return SOCKET_INVALID;

    sock_handle_t result = SOCKET_INVALID;
    socket_t* socket = new socket_t();
    if (socket) {
        wiced_result_t wiced_result;
        socket->set_type((protocol==IPPROTO_UDP ? socket_t::UDP : socket_t::TCP));
        if (protocol==IPPROTO_TCP) {
            wiced_result = wiced_tcp_create_socket(tcp(socket), wiced_wlan_interface(nif));
        }
        else {
            wiced_result = wiced_udp_create_socket(udp(socket), port, wiced_wlan_interface(nif));
        }
        if (wiced_result!=WICED_SUCCESS) {
            socket->set_type(socket_t::NONE);
            delete socket;
            result = as_sock_result(wiced_result);
        }
        else {
            SocketListLock lock(list_for(socket));
            add_socket(socket);
            result = as_sock_result(socket);
        }
    }
    return result;
}

/**
 * Send data to a socket.
 * @param sd    The socket handle to send data to.
 * @param buffer    The data to send
 * @param len       The number of bytes to send
 * @return
 */
sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len)
{
    return socket_send_ex(sd, buffer, len, 0, SOCKET_WAIT_FOREVER, nullptr);
}

sock_result_t socket_send_ex(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, system_tick_t timeout, void* reserved)
{
    sock_result_t result = SOCKET_INVALID;
    socket_t* socket = from_handle(sd);

    uint16_t bytes_sent = 0;

    if (is_open(socket)) {
        std::lock_guard<socket_t> lk(*socket);
        wiced_result_t wiced_result = WICED_TCPIP_INVALID_SOCKET;
        if (is_tcp(socket)) {
            wiced_tcp_send_flags_t wiced_flags = timeout == 0 ? WICED_TCP_SEND_FLAG_NONBLOCK : WICED_TCP_SEND_FLAG_NONE;
            bytes_sent = (uint16_t)len;
            wiced_result = wiced_tcp_send_buffer_ex(tcp(socket), buffer, &bytes_sent, wiced_flags, timeout);
        }
        else if (is_client(socket)) {
            tcp_server_client_t* server_client = client(socket);
            size_t written = 0;
            wiced_result = server_client->write(buffer, len, &written, flags, timeout);
            bytes_sent = (uint16_t)written;
        }
        if (!wiced_result)
            DEBUG("Write %d bytes to socket %d result=%d", (int)len, (int)sd, wiced_result);
        result = wiced_result ? as_sock_result(wiced_result) : bytes_sent;
    }
    return result;
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len,
        uint32_t flags, sockaddr_t* addr, socklen_t addr_size)
{
    socket_t* socket = from_handle(sd);
    wiced_result_t result = WICED_INVALID_SOCKET;
    if (is_open(socket) && is_udp(socket)) {
        std::lock_guard<socket_t> lk(*socket);
        SOCKADDR_TO_PORT_AND_IPADDR(addr, addr_data, port, ip_addr);
        uint16_t available = 0;
        wiced_packet_t* packet = NULL;
        uint8_t* data;
        if ((result=wiced_packet_create_udp(udp(socket), len, &packet, &data, &available))==WICED_SUCCESS) {
            size_t size = std::min(available, uint16_t(len));
            memcpy(data, buffer, size);
            /* Set the end of the data portion */
            wiced_packet_set_data_end(packet, (uint8_t*) data + size);
            result = wiced_udp_send(udp(socket), &ip_addr, port, packet);
            len = size;
        }
    }
    // return negative value on error, or length if successful.
    return result ? -result : len;
}

sock_result_t socket_receivefrom(sock_handle_t sd, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize)
{
    socket_t* socket = from_handle(sd);
    volatile wiced_result_t result = WICED_INVALID_SOCKET;
    uint16_t read_len = 0;
    if (is_open(socket) && is_udp(socket)) {
        std::lock_guard<socket_t> lk(*socket);
        wiced_packet_t* packet = NULL;
        // UDP receive timeout changed to 0 sec so as not to block
        if ((result=wiced_udp_receive(udp(socket), &packet, WICED_NO_WAIT))==WICED_SUCCESS) {
            wiced_ip_address_t wiced_ip_addr;
            uint16_t port;
            if ((result=wiced_udp_packet_get_info(packet, &wiced_ip_addr, &port))==WICED_SUCCESS) {
                uint32_t ipv4 = GET_IPV4_ADDRESS(wiced_ip_addr);
                addr->sa_data[0] = (port>>8) & 0xFF;
                addr->sa_data[1] = port & 0xFF;
                addr->sa_data[2] = (ipv4 >> 24) & 0xFF;
                addr->sa_data[3] = (ipv4 >> 16) & 0xFF;
                addr->sa_data[4] = (ipv4 >> 8) & 0xFF;
                addr->sa_data[5] = ipv4 & 0xFF;
                result=read_packet(packet, (uint8_t*)buffer, bufLen, &read_len);
            }
            wiced_packet_delete(packet);
        }
    }
    return result!=WICED_SUCCESS && result!=WICED_TIMEOUT ? as_sock_result(result) : sock_result_t(read_len);
}


sock_handle_t socket_handle_invalid()
{
    return SOCKET_INVALID;
}

sock_result_t socket_join_multicast(const HAL_IPAddress *address, network_interface_t nif, socket_multicast_info_t * /*reserved*/)
{
    wiced_ip_address_t multicast_address;
    SET_IPV4_ADDRESS(multicast_address, address->ipv4);
    return as_sock_result(wiced_multicast_join(wiced_wlan_interface(nif), &multicast_address));
}

sock_result_t socket_leave_multicast(const HAL_IPAddress *address, network_interface_t nif, socket_multicast_info_t * /*reserved*/)
{
    wiced_ip_address_t multicast_address;
    SET_IPV4_ADDRESS(multicast_address, address->ipv4);
    return as_sock_result(wiced_multicast_leave(wiced_wlan_interface(nif), &multicast_address));
}

/* WICED extension */
wiced_result_t wiced_last_error( wiced_tcp_socket_t* socket)
{
    wiced_assert("Bad args", (socket != NULL));
    if ( socket->conn_handler == NULL )
    {
        return WICED_NOT_CONNECTED;
    }
    else
    {
        return LWIP_TO_WICED_ERR( netconn_err(socket->conn_handler) );
    }
}

sock_result_t socket_peer(sock_handle_t sd, sock_peer_t* peer, void* reserved)
{
    sock_result_t result = WICED_INVALID_SOCKET;
    socket_t* socket = from_handle(sd);

    if (socket) {
        std::lock_guard<socket_t> lk(*socket);
        tcp_server_client_t* c = client(from_handle(sd));
        if (c && peer)
        {
            wiced_tcp_socket_t* wiced_sock = c->get_socket();
            wiced_ip_address_t ip;
            result = wiced_tcp_server_peer( wiced_sock, &ip, &peer->port);
            if (result==WICED_SUCCESS) {
                peer->address.ipv4 = GET_IPV4_ADDRESS(ip);
            }
        }
    }
    return result;
}
