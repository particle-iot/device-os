/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "hal_platform.h"
#include "system_tick_hal.h"
#include "at_parser.h"
// #include "at_command.h"
#include "at_response.h"

#if HAL_PLATFORM_CELLULAR

namespace particle {

namespace system {

enum NcpFWUpdateState {
    FW_UPDATE_IDLE_STATE                            = 0,
    FW_UPDATE_IDLE_STATE_1 = 101, // DEBUG
    FW_UPDATE_IDLE_STATE_2 = 102, // DEBUG
    FW_UPDATE_IDLE_STATE_3 = 103, // DEBUG
    FW_UPDATE_QUALIFY_FLAGS_STATE                   = 1,
    FW_UPDATE_QUALIFY_MODEM_ON_STATE                = 2,
    FW_UPDATE_SETUP_CLOUD_CONNECT_STATE             = 3,
    FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE          = 4,
    FW_UPDATE_SETUP_CLOUD_CONNECTED_STATE           = 5,
    FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECT_STATE       = 6,
    FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECTED_STATE     = 7,
    FW_UPDATE_DOWNLOAD_READY_STATE                  = 8,
    FW_UPDATE_INSTALL_CELL_DISCONNECTING_STATE      = 9,
    FW_UPDATE_INSTALL_STATE_STARTING                = 10,
    FW_UPDATE_INSTALL_STATE_WAITING                 = 12,
    FW_UPDATE_FINISHED_FACTORY_RESET_STATE          = 13,
    FW_UPDATE_FINISHED_POWERING_OFF_STATE           = 14,
    FW_UPDATE_FINISHED_CLOUD_CONNECTING_STATE       = 15,
    FW_UPDATE_FINISHED_CLOUD_CONNECTED_STATE        = 16,
    FW_UPDATE_FINISHED_STATE                        = 17,
};
enum NcpFWUpdateStatus {
    FW_UPDATE_NONE_STATUS           = 0,
    FW_UPDATE_DOWNLOADING_STATUS    = 1,
    FW_UPDATE_UPDATING_STATUS       = 2,
    FW_UPDATE_SUCCESS_STATUS        = 3,
    FW_UPDATE_FAILED_STATUS         = 4,
};

struct HTTPSresponse {
    int command;
    int result;
    int status_code;
    char md5_sum[40];
    int err_class;
    int err_code;
};

class NcpFwUpdate {
public:
    /**
     * Get the singleton instance of this class.
     */
    static NcpFwUpdate* instance();

    NcpFwUpdate();
    ~NcpFwUpdate();

    void checkUpdate();

    NcpFWUpdateStatus process();

private:

    NcpFWUpdateState ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
    NcpFWUpdateStatus ncpFwUpdateStatus_ = FW_UPDATE_NONE_STATUS;
    int foatReady_;
    system_tick_t startInstallTimer_;
    system_tick_t atOkCheckTimer_;
    int lastRespCode_;
    int atResp_;
    int atResponsive_;
    int startingFirmwareVersion_;
    int firmwareVersion_;
    int updateVersion_;
    system_tick_t cooldownTimer_;
    system_tick_t cooldownTimeout_;

    // void dumpAtCmd_(const char* buf, int len);
    static void ATResponseOut_(void* data, const char* msg);
    static int cbUUHTTPCR_(int type, const char* buf, int len, HTTPSresponse* data);
    static int cbUHTTPER_(int type, const char* buf, int len, HTTPSresponse* data);
    static int cbUUFWINSTALL_(int type, const char* buf, int len, int* data);
    static int cbULSTFILE_(int type, const char* buf, int len, int* data);
    static int cbATI9_(int type, const char* buf, int len, int* val);
    static int cbUPSND_(int type, const char* buf, int len, int* data);
    static int httpRespCallback_(AtResponseReader* reader, const char* prefix, void* data);
    int getAppFirmwareVersion_();
    int setupHTTPSProperties_();
    void cooldown_(system_tick_t timer);
    void updateCooldown_();
    bool inCooldown_();
};

} // namespace system

} // namespace particle

#endif // #if HAL_PLATFORM_CELLULAR
