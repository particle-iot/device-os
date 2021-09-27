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

#if HAL_PLATFORM_NCP
#include "at_parser.h"
#include "at_response.h"
#endif // HAL_PLATFORM_NCP

#if HAL_PLATFORM_NCP_FW_UPDATE

struct NcpFwUpdateCallbacks {
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
PARTICLE_STATIC_ASSERT(NcpFwUpdateCallbacks_size, sizeof(NcpFwUpdateCallbacks) == (sizeof(void*) * 7));

namespace particle {

namespace services {

const system_tick_t NCP_FW_MODEM_POWER_ON_TIMEOUT = 60000;
const system_tick_t NCP_FW_MODEM_INSTALL_START_TIMEOUT = 5 * 60000;
const system_tick_t NCP_FW_MODEM_INSTALL_FINISH_TIMEOUT = 30 * 60000;
const system_tick_t NCP_FW_MODEM_CLOUD_CONNECT_TIMEOUT = 5 * 60000;
const uint32_t NCP_FW_UBLOX_R510_ENG_VERSION = 100000000;
const int NCP_FW_UUFWINSTALL_COMPLETE = 128;

enum NcpFwUpdateState {
    FW_UPDATE_IDLE_STATE                            = 0,
    FW_UPDATE_QUALIFY_FLAGS_STATE                   = 1,
    FW_UPDATE_QUALIFY_MODEM_ON_STATE                = 2,
    FW_UPDATE_QUALIFY_RETRY_STATE                   = 3,
    FW_UPDATE_SETUP_CLOUD_CONNECT_STATE             = 4,
    FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE          = 5,
    FW_UPDATE_SETUP_CLOUD_CONNECTED_STATE           = 6,
    FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECT_STATE       = 7,
    FW_UPDATE_DOWNLOAD_CELL_DISCONNECTING_STATE     = 8,
    FW_UPDATE_DOWNLOAD_CELL_CONNECTING_STATE        = 9,
    FW_UPDATE_DOWNLOAD_HTTPS_SETUP_STATE            = 10,
    FW_UPDATE_DOWNLOAD_READY_STATE                  = 11,
    FW_UPDATE_INSTALL_CELL_DISCONNECTING_STATE      = 12,
    FW_UPDATE_INSTALL_CELL_DISCONNECTED_STATE       = 13,
    FW_UPDATE_INSTALL_STATE_STARTING                = 14,
    FW_UPDATE_INSTALL_STATE_WAITING                 = 15,
    FW_UPDATE_FINISHED_POWER_OFF_STATE              = 16,
    FW_UPDATE_FINISHED_POWERING_OFF_STATE           = 17,
    FW_UPDATE_FINISHED_CLOUD_CONNECTING_STATE       = 18,
    FW_UPDATE_FINISHED_CLOUD_CONNECTED_STATE        = 19,
    FW_UPDATE_FINISHED_IDLE_STATE                   = 20,
};

enum NcpFwUpdateStatus {
    FW_UPDATE_IDLE_STATUS                           = 0,
    FW_UPDATE_DOWNLOADING_STATUS                    = 1,
    FW_UPDATE_UPDATING_STATUS                       = 2,
    FW_UPDATE_SUCCESS_STATUS                        = 3,
    FW_UPDATE_NONE_STATUS                           = -1, // for diagnostics
    FW_UPDATE_FAILED_STATUS                         = -2,
    FW_UPDATE_FAILED_DOWNLOAD_RETRY_MAX_STATUS      = -3,
    FW_UPDATE_FAILED_START_INSTALL_TIMEOUT_STATUS   = -4,
    FW_UPDATE_FAILED_INSTALL_AT_ERROR_STATUS        = -5,
    FW_UPDATE_FAILED_SAME_VERSION_STATUS            = -6,
    FW_UPDATE_FAILED_INSTALL_TIMEOUT_STATUS         = -7,
    FW_UPDATE_FAILED_POWER_OFF_TIMEOUT_STATUS       = -8,
    FW_UPDATE_FAILED_CLOUD_CONNECT_TIMEOUT_STATUS   = -9,
    FW_UPDATE_FAILED_PUBLISH_RESULT_STATUS          = -10,
};

struct HTTPSresponse {
    int command;
    int result;
    int status_code;
    char md5_sum[40];
    int err_class;
    int err_code;
};

const int NCP_FW_DATA_HEADER = 0x5259ADE5;
const int NCP_FW_DATA_FOOTER = 0x1337ACE5;
struct NcpFwUpdateData {
    uint32_t header;                   // NCP_FW_DATA_HEADER;
    NcpFwUpdateState state;            // FW_UPDATE_IDLE_STATE;
    NcpFwUpdateStatus status;          // FW_UPDATE_IDLE_STATUS;
    int firmwareVersion;               // 0;
    int startingFirmwareVersion;       // 0;
    int updateVersion;                 // 0;
    uint8_t isUserConfig;              // 0;
    NcpFwUpdateConfig userConfigData;  // 0;
    uint32_t footer;                   // NCP_FW_DATA_FOOTER;
};

/** 
 * struct NcpFwUpdateConfig {
 *     const uint32_t start_version;
 *     const uint32_t end_version;
 *     const char filename[256];
 *     const char md5sum[32];
 * };
 */
const NcpFwUpdateConfig NCP_FW_UPDATE_CONFIG[] = {
    // { 3140001, 103140001, "SARA-R510S-01B-00-ES-0314A0001_SARA-R510S-01B-00-XX-0314ENG0099A0001.upd", "09c1a98d03c761bcbea50355f9b2a50f" },
    // { 103140001, 3140001, "SARA-R510S-01B-00-XX-0314ENG0099A0001_SARA-R510S-01B-00-ES-0314A0001.upd", "136caf2883457093c9e41fda3c6a44e3" },
    // { 2060001, 99010001, "SARA-R510S-00B-01_FW02.06_A00.01_IP_SARA-R510S-00B-01_FW99.01_A00.01.upd", "ccfdc48c0a45198d6e168b30d0740959" },
    // { 99010001, 2060001, "SARA-R510S-00B-01_FW99.01_A00.01_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd", "5fd6c0d3d731c097605895b86f28c2cf" },
};

const size_t NCP_FW_UPDATE_CONFIG_SIZE = sizeof(NCP_FW_UPDATE_CONFIG) / sizeof(NCP_FW_UPDATE_CONFIG[0]);

class NcpFwUpdate {
public:
    /**
     * Get the singleton instance of this class.
     */
    static NcpFwUpdate* instance();

    NcpFwUpdate();
    ~NcpFwUpdate();

    int checkUpdate(const NcpFwUpdateConfig* userConfigData);
    void init(NcpFwUpdateCallbacks* callbacks);
    int process();
    int getStatusDiagnostics();

private:

    NcpFwUpdateState ncpFwUpdateState_;
    NcpFwUpdateStatus ncpFwUpdateStatus_;
    NcpFwUpdateStatus ncpFwUpdateStatusDiagnostics_;
    int foatReady_;
    system_tick_t startInstallTimer_;
    system_tick_t atOkCheckTimer_;
    int lastRespCode_;
    int atResp_;
    int atResponsive_;
    int startingFirmwareVersion_;
    int firmwareVersion_;
    int updateVersion_;
    bool isUserConfig_;
    system_tick_t cooldownTimer_;
    system_tick_t cooldownTimeout_;
    bool initialized_;

    union {
        NcpFwUpdateData var;
        uint8_t data[sizeof(NcpFwUpdateData)];
    } ncpFwUpdateData_;

    /**
     * Functional callbacks that provide key system services to this NCP FW Update class.
     */
    NcpFwUpdateCallbacks* ncpFwUpdateCallbacks_;

    static int cbUUHTTPCR_(int type, const char* buf, int len, HTTPSresponse* data);
    static int cbUHTTPER_(int type, const char* buf, int len, HTTPSresponse* data);
    static int cbULSTFILE_(int type, const char* buf, int len, int* data);
    static int cbUPSND_(int type, const char* buf, int len, int* data);
    static int cbCOPS_(int type, const char* buf, int len, int* data);
    static int httpRespCallback_(AtResponseReader* reader, const char* prefix, void* data);
    uint32_t getAppFirmwareVersion_();
    int setupHTTPSProperties_();
    void cooldown_(system_tick_t timer);
    void updateCooldown_();
    bool inCooldown_();
    void validateNcpFwUpdateData_();
    int firmwareUpdateForVersion_(const int version);
    int saveNcpFwUpdateData_();
    int recallNcpFwUpdateData_();
    int deleteNcpFwUpdateData_();
};

class NcpFwUpdateDiagnostics: public AbstractUnsignedIntegerDiagnosticData {
public:
    NcpFwUpdateDiagnostics() :
            AbstractUnsignedIntegerDiagnosticData(DIAG_ID_NETWORK_NCP_FW_UPDATE_STATUS, DIAG_NAME_NETWORK_NCP_FW_UPDATE_STATUS) {
    }

    virtual int get(IntType& val) override {
        val = particle::services::NcpFwUpdate::instance()->getStatusDiagnostics();
        return 0; // OK
    }
};

} // namespace services

} // namespace particle

#endif // #if HAL_PLATFORM_NCP_FW_UPDATE
