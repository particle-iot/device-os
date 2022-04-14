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

#ifndef HAL_NETWORK_LWIP_LOCK_H
#define HAL_NETWORK_LWIP_LOCK_H

#include <lwip/opt.h>

#ifdef __cplusplus

namespace particle { namespace net {

class LwipTcpIpCoreLock {
public:
    LwipTcpIpCoreLock() :
            locked_(false) {
        lock();
    }

    ~LwipTcpIpCoreLock() {
        if (locked_) {
            unlock();
        }
    }

    LwipTcpIpCoreLock(LwipTcpIpCoreLock&& lock) :
            locked_(lock.locked_) {
        lock.locked_ = false;
    }

    void lock() {
        LOCK_TCPIP_CORE();
        locked_ = true;
    }

    void unlock() {
        UNLOCK_TCPIP_CORE();
        locked_ = false;
    }

    LwipTcpIpCoreLock(const LwipTcpIpCoreLock&) = delete;
    LwipTcpIpCoreLock& operator=(const LwipTcpIpCoreLock&) = delete;

private:
    bool locked_;
};

} } /* particle::net */

#endif /* __cplusplus */

#endif /* HAL_NETWORK_LWIP_LOCK_H */
