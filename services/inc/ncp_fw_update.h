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
#include "core_hal.h"
#include "system_defs.h"
#include "system_mode.h"
#include "system_network.h"
#include "cellular_hal.h"
#include "platform_ncp.h"
#include "diagnostics.h"
#include "spark_wiring_diagnostics.h"
#include "ncp_fw_update_dynalib.h"
#include "static_assert.h"

#if HAL_PLATFORM_NCP
#include "at_parser.h"
#include "at_response.h"
#endif // HAL_PLATFORM_NCP

#if HAL_PLATFORM_NCP_FW_UPDATE

struct SaraNcpFwUpdateCallbacks {
    uint16_t size;
    uint16_t reserved;

    // System.updatesEnabled() / system_update.h
    int (*system_get_flag)(system_flag_t flag, uint8_t* value, void*);

    // system_cloud.h
    bool (*spark_cloud_flag_connected)(void);
    void (*spark_cloud_flag_connect)(void);
    void (*spark_cloud_flag_disconnect)(void);

    // system_cloud_internal.h
    bool (*publishEvent)(const char* event, const char* data, unsigned flags);

    // system_mode.h
    System_Mode_TypeDef (*system_mode)(void);
};
PARTICLE_STATIC_ASSERT(SaraNcpFwUpdateCallbacks_size, sizeof(SaraNcpFwUpdateCallbacks) == (sizeof(void*) * 7));

namespace particle {

namespace services {

enum SaraNcpFwUpdateState {
    FW_UPDATE_STATE_IDLE                            = 0,
    FW_UPDATE_STATE_QUALIFY_FLAGS                   = 1,
    FW_UPDATE_STATE_SETUP_CLOUD_CONNECT             = 2,
    FW_UPDATE_STATE_SETUP_CLOUD_CONNECTING          = 3,
    FW_UPDATE_STATE_SETUP_CLOUD_CONNECTED           = 4,
    FW_UPDATE_STATE_DOWNLOAD_CLOUD_DISCONNECT       = 5,
    FW_UPDATE_STATE_DOWNLOAD_CELL_DISCONNECTING     = 6,
    FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING        = 7,
    FW_UPDATE_STATE_DOWNLOAD_HTTPS_SETUP            = 8,
    FW_UPDATE_STATE_DOWNLOAD_READY                  = 9,
    FW_UPDATE_STATE_INSTALL_CELL_DISCONNECTING      = 10,
    FW_UPDATE_STATE_INSTALL_STARTING                = 11,
    FW_UPDATE_STATE_INSTALL_WAITING                 = 12,
    FW_UPDATE_STATE_FINISHED_POWER_OFF              = 13,
    FW_UPDATE_STATE_FINISHED_POWERING_OFF           = 14,
    FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING       = 15,
    FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED        = 16,
    FW_UPDATE_STATE_FINISHED_IDLE                   = 17,
};
static_assert(FW_UPDATE_STATE_IDLE == 0 && FW_UPDATE_STATE_FINISHED_IDLE == 17, "SaraNcpFwUpdateState size changed!");

enum SaraNcpFwUpdateStatus {
    FW_UPDATE_STATUS_IDLE                                           = 0,
    FW_UPDATE_STATUS_DOWNLOADING                                    = 1,
    FW_UPDATE_STATUS_UPDATING                                       = 2,
    FW_UPDATE_STATUS_SUCCESS                                        = 3,
    FW_UPDATE_STATUS_NONE                                           = -1, // for diagnostics
    FW_UPDATE_STATUS_FAILED                                         = -2,
    FW_UPDATE_STATUS_FAILED_QUALIFY_FLAGS                           = -3,
    FW_UPDATE_STATUS_FAILED_CLOUD_CONNECT_ON_ENTRY_TIMEOUT          = -4,
    FW_UPDATE_STATUS_FAILED_PUBLISH_START                           = -5,
    FW_UPDATE_STATUS_FAILED_SETUP_CELLULAR_DISCONNECT_TIMEOUT       = -6,
    FW_UPDATE_STATUS_FAILED_CELLULAR_CONNECT_TIMEOUT                = -7,
    FW_UPDATE_STATUS_FAILED_HTTPS_SETUP                             = -8,
    FW_UPDATE_STATUS_FAILED_DOWNLOAD_RETRY_MAX                      = -9,
    FW_UPDATE_STATUS_FAILED_INSTALL_CELLULAR_DISCONNECT_TIMEOUT     = -10,
    FW_UPDATE_STATUS_FAILED_START_INSTALL_TIMEOUT                   = -11,
    FW_UPDATE_STATUS_FAILED_INSTALL_AT_ERROR                        = -12,
    FW_UPDATE_STATUS_FAILED_SAME_VERSION                            = -13,
    FW_UPDATE_STATUS_FAILED_INSTALL_TIMEOUT                         = -14,
    FW_UPDATE_STATUS_FAILED_POWER_OFF_TIMEOUT                       = -15,
    FW_UPDATE_STATUS_FAILED_CLOUD_CONNECT_ON_EXIT_TIMEOUT           = -16,
    FW_UPDATE_STATUS_FAILED_PUBLISH_RESULT                          = -17,
};
static_assert(FW_UPDATE_STATUS_IDLE == 0 && FW_UPDATE_STATUS_FAILED_PUBLISH_RESULT == -17, "SaraNcpFwUpdateStatus size changed!");

struct HTTPSresponse {
    volatile bool valid;
    int command;
    int result;
    int status_code;
    char md5_sum[40];
};

struct HTTPSerror {
    int err_class;
    int err_code;
};

class SaraNcpFwUpdate {
public:
    /**
     * Get the singleton instance of this class.
     */
    static SaraNcpFwUpdate* instance();

    void init(SaraNcpFwUpdateCallbacks callbacks);
    int process();
    int getStatusDiagnostics();
    int setConfig(const SaraNcpFwUpdateConfig* userConfigData = nullptr);
    int checkUpdate(uint32_t version = 0);
    int enableUpdates();
    int updateStatus();

private:
    struct SaraNcpFwUpdateData {
        uint16_t size;                          // sizeof(SaraNcpFwUpdateData)
        SaraNcpFwUpdateState state;             // FW_UPDATE_STATE_IDLE;
        SaraNcpFwUpdateStatus status;           // FW_UPDATE_STATUS_IDLE;
        uint32_t firmwareVersion;               // 0;
        uint32_t startingFirmwareVersion;       // 0;
        uint32_t updateVersion;                 // 0;
        uint8_t updateAvailable;                // SYSTEM_NCP_FW_UPDATE_STATUS_UNKNOWN;
        uint8_t isUserConfig;                   // 0;
        SaraNcpFwUpdateConfig userConfigData;   // 0;
    }; // Not using __attribute__((packed)) so that we can obtain a pointer to the address;

    SaraNcpFwUpdate();
    ~SaraNcpFwUpdate() = default;

    SaraNcpFwUpdateState saraNcpFwUpdateState_;
    SaraNcpFwUpdateState saraNcpFwUpdateLastState_;
    SaraNcpFwUpdateStatus saraNcpFwUpdateStatus_;
    SaraNcpFwUpdateStatus saraNcpFwUpdateStatusDiagnostics_;
    uint32_t startingFirmwareVersion_;
    uint32_t firmwareVersion_;
    uint32_t updateVersion_;
    int updateAvailable_;
    int downloadRetries_;
    int finishedCloudConnectingRetries_;
    volatile int cgevDeactProfile_;
    system_tick_t startTimer_;
    system_tick_t atOkCheckTimer_;
    system_tick_t cooldownTimer_;
    system_tick_t cooldownTimeout_;
    bool isUserConfig_;
    bool initialized_;
    SaraNcpFwUpdateData saraNcpFwUpdateData_ = {};
    /**
     * Functional callbacks that provide key system services to this NCP FW Update class.
     */
    SaraNcpFwUpdateCallbacks saraNcpFwUpdateCallbacks_ = {};
    HTTPSresponse httpsResp_ = {};

    static int cbUHTTPER(int type, const char* buf, int len, HTTPSerror* data);
    static int cbULSTFILE(int type, const char* buf, int len, int* data);
    static int cbUPSND(int type, const char* buf, int len, int* data);
    static int cbCOPS(int type, const char* buf, int len, bool* data);
    static int httpRespCallback(AtResponseReader* reader, const char* prefix, void* data);
    static int cgevCallback(AtResponseReader* reader, const char* prefix, void* data);
    uint32_t getNcpFirmwareVersion();
    int setupHTTPSProperties();
    void cooldown(system_tick_t timer);
    void updateCooldown();
    bool inCooldown();
    void validateSaraNcpFwUpdateData();
    int firmwareUpdateForVersion(uint32_t version);
    int getConfigData(SaraNcpFwUpdateConfig& configData);
    int saveSaraNcpFwUpdateData();
    int recallSaraNcpFwUpdateData();
    int deleteSaraNcpFwUpdateData();
    void logSaraNcpFwUpdateData(SaraNcpFwUpdateData& data);
};

class NcpFwUpdateDiagnostics: public AbstractUnsignedIntegerDiagnosticData {
public:
    NcpFwUpdateDiagnostics() :
            AbstractUnsignedIntegerDiagnosticData(DIAG_ID_NETWORK_NCP_FW_UPDATE_STATUS, DIAG_NAME_NETWORK_NCP_FW_UPDATE_STATUS) {
    }

    virtual int get(IntType& val) override {
        val = particle::services::SaraNcpFwUpdate::instance()->getStatusDiagnostics();
        return 0; // OK
    }
};

} // namespace services

} // namespace particle

#endif // #if HAL_PLATFORM_NCP_FW_UPDATE
