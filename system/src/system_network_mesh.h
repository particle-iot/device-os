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

#ifndef SYSTEM_NETWORK_MESH_H
#define SYSTEM_NETWORK_MESH_H

#include "hal_platform.h"

#if HAL_PLATFORM_MESH && HAL_PLATFORM_OPENTHREAD && HAL_PLATFORM_LWIP

#include "system_network_internal.h"
#include "system_setup.h"

#include <mutex>
#include <memory>
#include "openthread/lwip_openthreadif.h"
#include "system_openthread.h"
#include <openthread/platform/settings.h>
#include <openthread/dataset.h>
#include "net_hal.h"
#include "system_update.h"

class MeshNetworkInterface : public ManagedIPNetworkInterface<IPConfig, MeshNetworkInterface>
{
public:

    MeshNetworkInterface() {
        system_set_flag(SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS, 0, nullptr);
    }

    void fetch_ipconfig(IPConfig* target)  {
        /* FIXME */
    }

    virtual void start_listening() override {
        SystemSetupConsoleConfig config;
        SystemSetupConsole<SystemSetupConsoleConfig> console(config);
        ManagedNetworkInterface::start_listening(console);
    }

    virtual bool listening() override {
        return ManagedNetworkInterface::listening();
    }

    virtual void setup() override {
    }

    virtual bool clear_credentials() override {
        using namespace particle::system;
        std::lock_guard<ThreadLock> lk(ThreadLock());
        otPlatSettingsWipe(threadInstance());
        return true;
    }

    bool has_credentials() override {
        using namespace particle::system;
        std::lock_guard<ThreadLock> lk(ThreadLock());
        return otDatasetIsCommissioned(threadInstance());
    }

    virtual int set_credentials(NetworkCredentials* creds) override {
        return -1;
    }

    virtual void connect_cancel(bool cancel) override {
    }

    virtual void set_error_count(unsigned count) override {
    }

    virtual int set_hostname(const char* hostname) override
    {
        return 1;
    }

    virtual int get_hostname(char* buf, size_t buf_len, bool noDefault) override
    {
        if (buf) {
            buf[0] = '\0';
        }
        return 1;
    }

protected:
    virtual void on_finalize_listening(bool complete) override {
    }

    virtual void on_start_listening() override {
        auto& iff = iface();
        (void)iff;
    }

    virtual bool on_stop_listening() override {
        return true;
    }

    virtual void on_setup_cleanup() override {
    }

    virtual void connect_init() override {
    }

    virtual int connect_finalize() override {
        int r = iface().up();
        HAL_NET_notify_connected();
        HAL_NET_notify_dhcp(true);
        return r;
    }

    virtual int on_now() override {
        auto& iff = iface();
        (void)iff;
        return 0;
    }

    virtual void off_now() override {
        disconnect_now();
    }

    virtual void disconnect_now() override {
        iface().down();
        HAL_NET_notify_disconnected();
    }

private:
    particle::net::OpenThreadNetif& iface() {
        static particle::net::OpenThreadNetif iff(particle::system::threadInstance());
        return iff;
    }
};

#endif /* HAL_PLATFORM_MESH */

#endif /* SYSTEM_NETWORK_MESH_H */
