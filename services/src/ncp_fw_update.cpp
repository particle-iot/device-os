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

#if MODULE_FUNCTION != 2 // BOOTLOADER

#include "ncp_fw_update.h"
#include "system_error.h"
#if HAL_PLATFORM_NCP_FW_UPDATE

#include "logging.h"
#define SARA_NCP_FW_UPDATE_LOG_CATEGORY "system.ncp.update"
LOG_SOURCE_CATEGORY(SARA_NCP_FW_UPDATE_LOG_CATEGORY);

#include "cellular_enums_hal.h"
#include "platform_ncp.h"

#if HAL_PLATFORM_NCP
#include "network/ncp/cellular/cellular_network_manager.h"
#include "network/ncp/cellular/cellular_ncp_client.h"
#include "network/ncp/cellular/ncp.h"
#include "system_cache.h"
#endif // HAL_PLATFORM_NCP

#include "check.h"
#include "delay_hal.h"
#include "system_network.h"

#if SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING
#define NCPFW_LOG_DEBUG(_level, _fmt, ...) LOG_DEBUG_C(_level, SARA_NCP_FW_UPDATE_LOG_CATEGORY, _fmt, ##__VA_ARGS__)
#else
#define NCPFW_LOG_DEBUG(_level, _fmt, ...)
#endif // SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING

// #ifndef UNIT_TEST
#define FMT_LU "lu"
#define NCPFW_LOG(_level, _fmt, ...) LOG_C(_level, SARA_NCP_FW_UPDATE_LOG_CATEGORY, _fmt, ##__VA_ARGS__)
// #else
// #include <inttypes.h>
// #define FMT_LU PRIu32
// #define NCPFW_LOG(_level, _fmt, ...) printf(_fmt"\r\n", ##__VA_ARGS__)
// #endif

#ifndef UNIT_TEST
#define SEND_COMMAND cellular_command
#else
#define SEND_COMMAND sendCommand
#endif

namespace particle {

namespace services {

namespace {

#define CHECK_NCPID(x) \
        do { \
            const bool _ok = (bool)(platform_primary_ncp_identifier() == (x)); \
            if (!_ok) { \
                return SYSTEM_ERROR_NOT_SUPPORTED; \
            } \
        } while (false)

inline bool inSafeMode() {
    return system_mode() == System_Mode_TypeDef::SAFE_MODE;
}

inline void delay(system_tick_t delay_ms) {
    return HAL_Delay_Milliseconds(delay_ms);
}

inline system_tick_t millis() {
    return HAL_Timer_Get_Milli_Seconds();
}

#ifndef UNIT_TEST
static int cbUPSND(int type, const char* buf, int len, int* data)
{
    int val = 0;
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        // +UPSND: 0,8,1 - IP Connection
        // +UPSND: 0,8,0 - No IP Connection
        if (sscanf(buf, "%*[\r\n]+UPSND: %*d,%*d,%d", &val) >= 1) {
            *data = val;
        }
    }
    return WAIT;
}
static int cbCOPS(int type, const char* buf, int len, bool* data)
{
    int act;
    // +COPS: 0,2,"310410",7
    // +COPS: 0,0,"AT&T",7
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        if (sscanf(buf, "\r\n+COPS: %*d,%*d,\"%*6[0-9]\",%d", &act) >= 1) {
            if (act >= 7) {
                *data = true;
            }
        } else if (sscanf(buf, "\r\n+COPS: %*d,%*d,\"%*[^\"]\",%d", &act) >= 1) {
            if (act >= 7) {
                *data = true;
            }
        }
    }
    return WAIT;
}
system_error_t setupHTTPSProperties_impl() {
    // Wait for registration, using COPS AcT to determine this instead of CEREG.
    bool registered = false;
    uint32_t start = millis();
    do {
        cellular_command((_CALLBACKPTR_MDM)cbCOPS, (void*)&registered, 10000, "AT+COPS?\r\n");
        if (!registered) {
            delay(15000);
        }
    } while (!registered && millis() - start <= NCP_FW_MODEM_CELLULAR_CONNECT_TIMEOUT);
    if (!registered) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // TODO: We should really have some single source of default registration timeout somewhere,
    // right now this is spread out through Quectel, u-blox NCP and Electron modem implementations.
    // Tests could also use such a definition.

    cellular_command(nullptr, nullptr, 10000, "AT+CMEE=2\r\n");
    cellular_command(nullptr, nullptr, 10000, "AT+CGPADDR=0\r\n");
    // Activate data connection
    int actVal = 0;
    cellular_command((_CALLBACKPTR_MDM)cbUPSND, (void*)&actVal, 10000, "AT+UPSND=0,8\r\n");
    if (actVal != 1) {
        cellular_command(nullptr, nullptr, 10000, "AT+UPSD=0,100,1\r\n");
        cellular_command(nullptr, nullptr, 10000, "AT+UPSD=0,0,0\r\n");
        int r = cellular_command(nullptr, nullptr, 180000, "AT+UPSDA=0,3\r\n");
        if (r == RESP_ERROR) {
            cellular_command(nullptr, nullptr, 10000, "AT+CEER\r\n");
        }
    }

    // Setup security settings
    NCPFW_LOG_DEBUG(INFO, "setupHTTPSProperties_");
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,0,3\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_1; // Highest level (3) root cert checks
    }
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,1,3\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_2; // Minimum TLS v1.2
    }
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,2,99,\"C0\",\"2F\"\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_3; // Cipher suite
    }
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,3,\"ubx_digicert_global_root_ca\"\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_4; // Cert name
    }
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,4,\"fw.particle.io\"\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_5; // Expected server hostname
    }
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,10,\"fw.particle.io\"\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_6; // SNI (Server Name Indication)
    }
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,1,\"fw.particle.io\"\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_7;
    }
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,5,443\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_8;
    }
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,6,1,2\r\n")) {
        return SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_9;
    }

    return SYSTEM_ERROR_NONE;
}
#else
// For mocks, we need to removed the variable arguments and expose a function signature sendCommandWithArgs
int sendCommand(_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, ...) {
    va_list args;
    va_start(args, format);
    const int resp = sendCommandWithArgs(cb, param, timeout_ms, format, args);
    va_end(args);
    return resp;
}
#endif // UNIT_TEST

} // namespace

SaraNcpFwUpdate::SaraNcpFwUpdate() :
        saraNcpFwUpdateState_(FW_UPDATE_STATE_IDLE),
        saraNcpFwUpdateLastState_(saraNcpFwUpdateState_),
        saraNcpFwUpdateStatus_(FW_UPDATE_STATUS_IDLE),
        saraNcpFwUpdateStatusDiagnostics_(FW_UPDATE_STATUS_NONE), // initialize to none, only set when update process complete.
        saraNcpFwUpdateErrorDiagnostics_(SYSTEM_ERROR_NONE), // initialize to none, only set when update process complete.
        startingFirmwareVersion_(1),  // Keep these initialized differently to prevent
        firmwareVersion_(0),          // code from thinking an update was successful
        updateVersion_(2),            //  |
        updateAvailable_(SYSTEM_NCP_FW_UPDATE_UNKNOWN),
        downloadRetries_(0),
        finishedCloudConnectingRetries_(0),
        cgevDeactProfile_(0),
        startTimer_(0),
        atOkCheckTimer_(0),
        cooldownTimer_(0),
        cooldownTimeout_(0),
        isUserConfig_(false),
        initialized_(false) {
}

void SaraNcpFwUpdate::init(SaraNcpFwUpdateCallbacks callbacks) {
    this->saraNcpFwUpdateCallbacks_ = callbacks;
    memset(&saraNcpFwUpdateData_, 0, sizeof(saraNcpFwUpdateData_));
    recallSaraNcpFwUpdateData();
    if (validateSaraNcpFwUpdateData() != SYSTEM_ERROR_NONE) {
        recallSaraNcpFwUpdateData();
    }
    if (!inSafeMode()) {
        // If not in safe mode, make sure to reset the firmware update state.
        if (saraNcpFwUpdateData_.state == FW_UPDATE_STATE_FINISHED_IDLE) {
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_IDLE;
            if (saraNcpFwUpdateData_.status != FW_UPDATE_STATUS_IDLE) {
                NCPFW_LOG(INFO, "Firmware update = %s, status: %d error: %d", (saraNcpFwUpdateData_.status == FW_UPDATE_STATUS_SUCCESS) ? "success" : "failure", saraNcpFwUpdateData_.status, saraNcpFwUpdateData_.error);
                saraNcpFwUpdateStatusDiagnostics_ = saraNcpFwUpdateData_.status;
                saraNcpFwUpdateErrorDiagnostics_ = saraNcpFwUpdateData_.error;
                saraNcpFwUpdateData_.status = FW_UPDATE_STATUS_IDLE;
                saraNcpFwUpdateData_.error = SYSTEM_ERROR_NONE;
                saraNcpFwUpdateData_.updateAvailable = SYSTEM_NCP_FW_UPDATE_UNKNOWN;
                saveSaraNcpFwUpdateData();
            }
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_IDLE;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_NONE;
            updateAvailable_ = SYSTEM_NCP_FW_UPDATE_UNKNOWN;
        } else if (saraNcpFwUpdateData_.state == FW_UPDATE_STATE_INSTALL_WAITING) {
            NCPFW_LOG(INFO, "Resuming update in Safe Mode!");
            delay(200);
            system_reset(SYSTEM_RESET_MODE_SAFE, 0, 0, SYSTEM_RESET_FLAG_NO_WAIT, nullptr);
        } else {
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_IDLE; // default to disable updates
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_IDLE;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_NONE;
            updateAvailable_ = SYSTEM_NCP_FW_UPDATE_UNKNOWN;
        }
    } else {
        // Ensure we recall the previously set state
        saraNcpFwUpdateState_ = saraNcpFwUpdateData_.state;
        // If we come up in Safe Mode as FINISHED, ensure we have an IDLE status
        // to prevent a dependency issue causing a safe mode loop
        if (saraNcpFwUpdateState_ == FW_UPDATE_STATE_FINISHED_IDLE) {
            saraNcpFwUpdateData_.status = FW_UPDATE_STATUS_IDLE;
            saraNcpFwUpdateData_.error = SYSTEM_ERROR_NONE;
            saraNcpFwUpdateData_.updateAvailable = SYSTEM_NCP_FW_UPDATE_UNKNOWN;
        }
        saraNcpFwUpdateStatus_ = saraNcpFwUpdateData_.status;
        saraNcpFwUpdateError_ = saraNcpFwUpdateData_.error;
        firmwareVersion_ = saraNcpFwUpdateData_.firmwareVersion;
        startingFirmwareVersion_ = saraNcpFwUpdateData_.startingFirmwareVersion;
        updateVersion_ = saraNcpFwUpdateData_.updateVersion;
        isUserConfig_ = saraNcpFwUpdateData_.isUserConfig;
        updateAvailable_ = saraNcpFwUpdateData_.updateAvailable;
    }
    initialized_ = true;
}

int SaraNcpFwUpdate::getStatusDiagnostics() {
    return saraNcpFwUpdateStatusDiagnostics_;
}

int SaraNcpFwUpdate::getErrorDiagnostics() {
    return saraNcpFwUpdateErrorDiagnostics_;
}

SaraNcpFwUpdate* SaraNcpFwUpdate::instance() {
    static SaraNcpFwUpdate instance;
    return &instance;
}

int SaraNcpFwUpdate::lock() {
    return mutex_.lock();
}

int SaraNcpFwUpdate::unlock() {
    return mutex_.unlock();
}

int SaraNcpFwUpdate::setConfig(const SaraNcpFwUpdateConfig* userConfigData) {
    SaraNcpFwUpdateLock lk;
    CHECK_NCPID(PLATFORM_NCP_SARA_R510);
    if (saraNcpFwUpdateStatus_ != FW_UPDATE_STATUS_IDLE) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (userConfigData) {
        validateSaraNcpFwUpdateData();
        memcpy(&saraNcpFwUpdateData_.userConfigData, userConfigData, sizeof(SaraNcpFwUpdateConfig));
        logSaraNcpFwUpdateData(saraNcpFwUpdateData_);
        isUserConfig_ = true;
    } else {
        isUserConfig_ = false;
    }
    saraNcpFwUpdateData_.isUserConfig = isUserConfig_;

    // Check if update avail with/without userConfigData
    CHECK(checkUpdate());

    return SYSTEM_ERROR_NONE;
}

// Check if firmware version requires an upgrade
int SaraNcpFwUpdate::checkUpdate(uint32_t version /* default = 0 */) {
    SaraNcpFwUpdateLock lk;
    CHECK_NCPID(PLATFORM_NCP_SARA_R510);
    // Prevent power cycling the modem during an update from altering the state of member variables
    if (updateAvailable_ == SYSTEM_NCP_FW_UPDATE_IN_PROGRESS) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (!version) {
        firmwareVersion_ = getNcpFirmwareVersion();
        if (firmwareVersion_ == 0) {
            NCPFW_LOG(ERROR, "Modem has unknown firmware version or powered off");
            return SYSTEM_ERROR_INVALID_STATE;
        }
    } else {
        firmwareVersion_ = version;
    }
    if ((!isUserConfig_ && firmwareUpdateForVersion(firmwareVersion_) == SYSTEM_ERROR_NOT_FOUND) ||
            (isUserConfig_ && firmwareVersion_ != saraNcpFwUpdateData_.userConfigData.start_version)) {
        NCPFW_LOG(INFO, "No firmware update available");
        updateAvailable_ = SYSTEM_NCP_FW_UPDATE_NOT_AVAILABLE;
    } else {
        NCPFW_LOG(INFO, "NCP FW Update Fully Qualified!");
        saraNcpFwUpdateData_.firmwareVersion = firmwareVersion_;
        updateAvailable_ = SYSTEM_NCP_FW_UPDATE_PENDING;
        saraNcpFwUpdateData_.updateAvailable = updateAvailable_;
        saveSaraNcpFwUpdateData();
    }

    // Reset some variables used in the update process
    cooldownTimer_ = 0;
    cooldownTimeout_ = 0;
    downloadRetries_ = 0;
    finishedCloudConnectingRetries_ = 0;

    return SYSTEM_ERROR_NONE;
}

int SaraNcpFwUpdate::enableUpdates() {
    SaraNcpFwUpdateLock lk;
    CHECK_NCPID(PLATFORM_NCP_SARA_R510);
    if (!initialized_ ||
            updateAvailable_ != SYSTEM_NCP_FW_UPDATE_PENDING ||
            !network_is_on(NETWORK_INTERFACE_CELLULAR, nullptr) ||
            network_is_off(NETWORK_INTERFACE_CELLULAR, nullptr)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    saraNcpFwUpdateState_ = FW_UPDATE_STATE_QUALIFY_FLAGS;
    updateAvailable_ = SYSTEM_NCP_FW_UPDATE_IN_PROGRESS;

    return SYSTEM_ERROR_NONE;
}

int SaraNcpFwUpdate::updateStatus() {
    CHECK_NCPID(PLATFORM_NCP_SARA_R510);
    if (!initialized_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    return updateAvailable_;
}

int SaraNcpFwUpdate::validateSaraNcpFwUpdateData() {
    if (saraNcpFwUpdateData_.size != sizeof(SaraNcpFwUpdateData)) {
        memset(&saraNcpFwUpdateData_, 0, sizeof(saraNcpFwUpdateData_));
        saraNcpFwUpdateData_.size = sizeof(SaraNcpFwUpdateData);
        saraNcpFwUpdateData_.state = FW_UPDATE_STATE_IDLE;
        saraNcpFwUpdateData_.status = FW_UPDATE_STATUS_IDLE;
        // saraNcpFwUpdateData_.updateAvailable = SYSTEM_NCP_FW_UPDATE_UNKNOWN; // 0
        // saraNcpFwUpdateData_.isUserConfig = false; // 0

        saveSaraNcpFwUpdateData();

        isUserConfig_ = false;
        updateAvailable_ = SYSTEM_NCP_FW_UPDATE_UNKNOWN; // 0
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_IDLE;
        saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_IDLE;
        saraNcpFwUpdateError_ = SYSTEM_ERROR_NONE;
        NCPFW_LOG(INFO, "saraNcpFwUpdateData_ initialized");
        return SYSTEM_ERROR_BAD_DATA;
    }

    return SYSTEM_ERROR_NONE;
}

void SaraNcpFwUpdate::logSaraNcpFwUpdateData(SaraNcpFwUpdateData& data) {
    NCPFW_LOG(INFO, "saraNcpFwUpdateData size:%u state:%d status:%d error:%d fv:%" FMT_LU " sfv:%" FMT_LU " uv:%" FMT_LU "",
            data.size,
            data.state,
            data.status,
            data.error,
            data.firmwareVersion,
            data.startingFirmwareVersion,
            data.updateVersion);
    NCPFW_LOG(INFO, "ua:%d iuc:%d sv:%" FMT_LU " ev:%" FMT_LU " file:%s md5:%s",
            data.updateAvailable,
            data.isUserConfig,
            data.userConfigData.start_version,
            data.userConfigData.end_version,
            data.userConfigData.filename,
            data.userConfigData.md5sum);
}

int SaraNcpFwUpdate::saveSaraNcpFwUpdateData() {
    SaraNcpFwUpdateData tempData = {};
    int result = SystemCache::instance().get(SystemCacheKey::SARA_NCP_FW_UPDATE_DATA, (uint8_t*)&tempData, sizeof(tempData));
    if (result != sizeof(tempData) ||
            /* memcmp(tempData, saraNcpFwUpdateData_, sizeof(tempData) != 0)) { // not reliable */
            tempData.size != saraNcpFwUpdateData_.size ||
            tempData.state  != saraNcpFwUpdateData_.state ||
            tempData.status != saraNcpFwUpdateData_.status ||
            tempData.error != saraNcpFwUpdateData_.error ||
            tempData.firmwareVersion != saraNcpFwUpdateData_.firmwareVersion ||
            tempData.startingFirmwareVersion != saraNcpFwUpdateData_.startingFirmwareVersion ||
            tempData.updateVersion != saraNcpFwUpdateData_.updateVersion ||
            tempData.isUserConfig != saraNcpFwUpdateData_.isUserConfig ||
            tempData.updateAvailable != saraNcpFwUpdateData_.updateAvailable ||
            tempData.userConfigData.start_version != saraNcpFwUpdateData_.userConfigData.start_version ||
            tempData.userConfigData.start_version != saraNcpFwUpdateData_.userConfigData.end_version ||
            strcmp(tempData.userConfigData.filename, saraNcpFwUpdateData_.userConfigData.filename) != 0 ||
            strcmp(tempData.userConfigData.md5sum, saraNcpFwUpdateData_.userConfigData.md5sum) != 0) {
        logSaraNcpFwUpdateData(tempData);
        NCPFW_LOG(INFO, "Writing cached saraNcpFwUpdateData, size: %d", saraNcpFwUpdateData_.size);
        result = SystemCache::instance().set(SystemCacheKey::SARA_NCP_FW_UPDATE_DATA, (uint8_t*)&saraNcpFwUpdateData_, sizeof(saraNcpFwUpdateData_));
    }
    return (result < 0) ? result : SYSTEM_ERROR_NONE;
}

int SaraNcpFwUpdate::recallSaraNcpFwUpdateData() {
    SaraNcpFwUpdateData tempData;
    int result = SystemCache::instance().get(SystemCacheKey::SARA_NCP_FW_UPDATE_DATA, (uint8_t*)&tempData, sizeof(tempData));
    if (result != sizeof(tempData)) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    NCPFW_LOG(INFO, "Reading cached saraNcpFwUpdateData");
    memcpy(&saraNcpFwUpdateData_, &tempData, sizeof(saraNcpFwUpdateData_));
    logSaraNcpFwUpdateData(tempData);
    return SYSTEM_ERROR_NONE;
}

int SaraNcpFwUpdate::deleteSaraNcpFwUpdateData() {
    NCPFW_LOG_DEBUG(INFO, "Deleting cached saraNcpFwUpdateData");
    int result = SystemCache::instance().del(SystemCacheKey::SARA_NCP_FW_UPDATE_DATA);
    return (result < 0) ? result : SYSTEM_ERROR_NONE;
}

int SaraNcpFwUpdate::firmwareUpdateForVersion(uint32_t version) {
    for (size_t i = 0; i < SARA_NCP_FW_UPDATE_CONFIG_SIZE; ++i) {
        if (version == SARA_NCP_FW_UPDATE_CONFIG[i].start_version) {
            return i;
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int SaraNcpFwUpdate::getConfigData(SaraNcpFwUpdateConfig& configData) {
    const int fwIdx = firmwareUpdateForVersion(firmwareVersion_);
    if (isUserConfig_ || fwIdx != SYSTEM_ERROR_NOT_FOUND) {
        if (isUserConfig_) {
            memcpy(&configData, &saraNcpFwUpdateData_.userConfigData, sizeof(configData));
        } else {
            memcpy(&configData, &SARA_NCP_FW_UPDATE_CONFIG[fwIdx], sizeof(configData));
        }
        return SYSTEM_ERROR_NONE;
    }

    return SYSTEM_ERROR_NOT_FOUND;
}

int SaraNcpFwUpdate::process() {
    CHECK_NCPID(PLATFORM_NCP_SARA_R510);
#ifndef UNIT_TEST
    SPARK_ASSERT(initialized_);
#else
    if (!initialized_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
#endif

#ifdef SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING
    // Make state changes more obvious
    if (saraNcpFwUpdateState_ != saraNcpFwUpdateLastState_) {
        NCPFW_LOG(INFO, "======================= saraNcpFwUpdateState: %d", saraNcpFwUpdateState_);
    }
    saraNcpFwUpdateLastState_ = saraNcpFwUpdateState_;
#endif // SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING

    validateSaraNcpFwUpdateData();

    if (inCooldown()) {
        updateCooldown();
        return SYSTEM_ERROR_NONE;
    }

    switch (saraNcpFwUpdateState_) {

    case FW_UPDATE_STATE_QUALIFY_FLAGS:
    {
        bool failedChecks = false;
        // Check NCP version
        if (platform_primary_ncp_identifier() != PLATFORM_NCP_SARA_R510) {
            NCPFW_LOG(ERROR, "PLATFORM_NCP != SARA_R510");
            failedChecks = true;
        }
        // Make sure update is in progress
        if (updateAvailable_ != SYSTEM_NCP_FW_UPDATE_IN_PROGRESS) {
            NCPFW_LOG(ERROR, "Unexpected status updateAvailable_: %d", updateAvailable_);
            failedChecks = true;
        }
        if (failedChecks) {
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_QUALIFY_FLAGS;
            break;
        }

        // If not in Safe Mode, reset now into Safe Mode!
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_SETUP_CLOUD_CONNECT;
        saraNcpFwUpdateData_.state = saraNcpFwUpdateState_;
        saraNcpFwUpdateData_.firmwareVersion = firmwareVersion_;
        saraNcpFwUpdateData_.updateAvailable = updateAvailable_;
        saveSaraNcpFwUpdateData();
        if (!inSafeMode()) {
            NCPFW_LOG(INFO, "Resetting into Safe Mode to update modem firmware");
            delay(200);
            system_reset(SYSTEM_RESET_MODE_SAFE, 0, 0, SYSTEM_RESET_FLAG_NO_WAIT, nullptr);
            // Goodbye!  See you next time through in Safe Mode ;-)
        }
    }
    break;

    case FW_UPDATE_STATE_SETUP_CLOUD_CONNECT:
    {
        if (!saraNcpFwUpdateCallbacks_.spark_cloud_flag_connected()) { // !Particle.connected()
            NCPFW_LOG_DEBUG(INFO, "Connect to Cloud...");
            saraNcpFwUpdateCallbacks_.spark_cloud_flag_connect(); // Particle.connect()
        }
        startTimer_ = millis();
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_SETUP_CLOUD_CONNECTING;
        saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_DOWNLOADING;
        saraNcpFwUpdateStatusDiagnostics_ = saraNcpFwUpdateStatus_; // ready our diagnostics status value before connecting to the cloud
        saraNcpFwUpdateErrorDiagnostics_ = saraNcpFwUpdateError_;
    }
    break;

    case FW_UPDATE_STATE_SETUP_CLOUD_CONNECTING:
    {
        if (millis() - startTimer_ >= NCP_FW_MODEM_CLOUD_CONNECT_TIMEOUT) {
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_CLOUD_CONNECT_ON_ENTRY_TIMEOUT;
        } else if (saraNcpFwUpdateCallbacks_.spark_cloud_flag_connected()) { // Particle.connected()
            NCPFW_LOG_DEBUG(INFO, "Connected to Cloud.");
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_SETUP_CLOUD_CONNECTED;
        } else {
            cooldown(1000);
        }
    }
    break;

    case FW_UPDATE_STATE_SETUP_CLOUD_CONNECTED:
    {
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
        if (saraNcpFwUpdateCallbacks_.spark_cloud_flag_connected()) { // Particle.connected()
            if (saraNcpFwUpdateCallbacks_.publishEvent("spark/device/ncp/update", "started", /*flags PUBLISH_EVENT_FLAG_PRIVATE*/1)) {
                NCPFW_LOG_DEBUG(INFO, "Ready to start download...");
                saraNcpFwUpdateState_ = FW_UPDATE_STATE_DOWNLOAD_CLOUD_DISCONNECT;
            } else {
                saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
                saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_PUBLISH_START;
            }
        } else {
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_PUBLISH_START;
        }
    }
    break;

    case FW_UPDATE_STATE_DOWNLOAD_CLOUD_DISCONNECT:
    {
        if (saraNcpFwUpdateCallbacks_.spark_cloud_flag_connected()) { // Particle.connected()
            saraNcpFwUpdateCallbacks_.spark_cloud_flag_disconnect(); // Particle.disconnect()
        } else {
            NCPFW_LOG_DEBUG(INFO, "Disconnected from Cloud.");
            if (network_ready(0, 0, 0)) { // Cellular.ready()
                NCPFW_LOG_DEBUG(INFO, "Disconnecting Cellular...");
                // network_disconnect() doesn't wait for final OK response from CFUN=0
                // wait for DETACH: +CGEV: ME PDN DEACT 1
                cgevDeactProfile_ = 0;
                cellular_add_urc_handler("+CGEV", [](const char* data, void* context) -> int {
                    const auto self = (SaraNcpFwUpdate*)context;
                    int profile;

                    int r = ::sscanf(data, "+CGEV: ME PDN DEACT %d", &profile);
                    // do not CHECK_TRUE as we intend to ignore +CGEV: ME DETACH
                    if (r >= 1) {
                        self->cgevDeactProfile_ = profile;
                    }

                    return SYSTEM_ERROR_NONE;
                }, this);
                startTimer_ = millis();
                network_disconnect(0, NETWORK_DISCONNECT_REASON_USER, 0);
                saraNcpFwUpdateState_ = FW_UPDATE_STATE_DOWNLOAD_CELL_DISCONNECTING;
            }
        }
    }
    break;

    case FW_UPDATE_STATE_DOWNLOAD_CELL_DISCONNECTING:
    {
        cellular_lock(nullptr); // protect against concurrent access to httpsResp_ & cgevDeactProfile_
        const int tempCgevDeactProfile = cgevDeactProfile_;
        cellular_unlock(nullptr);
        if (tempCgevDeactProfile == NCP_FW_UBLOX_DEFAULT_CID) { // Default CID detached
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING;
            cooldown(1000); // allow other disconnect URCs to pass
        }
        if (millis() - startTimer_ >= NCP_FW_MODEM_CLOUD_DISCONNECT_TIMEOUT) {
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING;
        }
    }
    break;

    case FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING:
    {
        if (!network_ready(0, 0, 0)) { // Cellular.ready()
            NCPFW_LOG_DEBUG(INFO, "Disconnected Cellular. Reconnecting without PPP...");
            cellular_start_ncp_firmware_update(true, nullptr); // ensure we don't connect PPP
            network_connect(0, 0, 0, 0); // Cellular.connect()
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_DOWNLOAD_HTTPS_SETUP;
        } else {
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_SETUP_CELLULAR_STILL_CONNECTED;
        }
    }
    break;

    case FW_UPDATE_STATE_DOWNLOAD_HTTPS_SETUP:
    {
        auto ret = setupHTTPSProperties();
        if (ret < 0) {
            NCPFW_LOG_DEBUG(INFO, "HTTPS setup error: %d", ret);
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
            if (ret == SYSTEM_ERROR_INVALID_STATE) {
                saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_SETUP_CELLULAR_CONNECT_TIMEOUT;
            } else {
                saraNcpFwUpdateError_ = ret;
            }
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
            break;
        }
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_DOWNLOAD_READY;
    }
    break;

    case FW_UPDATE_STATE_DOWNLOAD_READY:
    {
        bool modemFirmwareDownloadComplete = false;
        int foatReady = 0;
        memset(&httpsResp_, 0, sizeof(httpsResp_));

        // There doesn't appear to be a way to test what updatePackage.bin actually IS (MD5 sum or APP version)
        // so if one exists, we must delete it before attempting to upgrade or else the FW INSTALL will
        // basically end early without the right version being applied.
        NCPFW_LOG_DEBUG(INFO, "Existing FOAT file?");
        SEND_COMMAND((_CALLBACKPTR_MDM)cbULSTFILE, (void*)&foatReady, 10000, "AT+ULSTFILE=0,\"FOAT\"\r\n");
        if (foatReady) {
            if (RESP_OK == SEND_COMMAND(nullptr, nullptr, 10000, "AT+UDELFILE=\"updatePackage.bin\",\"FOAT\"\r\n")) {
                NCPFW_LOG_DEBUG(INFO, "updatePackage.bin deleted.");
            }
        }

        cellular_add_urc_handler("+UUHTTPCR", [](const char* data, void* context) -> int {
            const auto self = (SaraNcpFwUpdate*)context;
            int a, b, c;
            char s[40];

            int r = ::sscanf(data, "+UUHTTPCR: %*d,%d,%d,%d,\"%32s\"", &a, &b, &c, s);
            CHECK_TRUE(r >= 3, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

            self->httpsResp_.valid = false; // make following lines atomic
            self->httpsResp_.command = a;
            self->httpsResp_.result = b;
            self->httpsResp_.status_code = c;
            if (r > 3) {
                memcpy(self->httpsResp_.md5_sum, &s, sizeof(s));
            }
            self->httpsResp_.valid = true;

            return SYSTEM_ERROR_NONE;
        }, this);

        NCPFW_LOG_DEBUG(INFO, "Starting download...");
#if SARA_NCP_FW_UPDATE_ENABLE_DOWNLOAD
        SaraNcpFwUpdateConfig configData;
        if (getConfigData(configData) != SYSTEM_ERROR_NOT_FOUND) {
            updateVersion_ = configData.end_version;
            saraNcpFwUpdateData_.updateVersion = updateVersion_;
            saveSaraNcpFwUpdateData();
            // 99010001 -> 02060001 firmware (takes 1.8 - 2.8 minutes)
            int resp = SEND_COMMAND(nullptr, nullptr, NCP_FW_MODEM_DOWNLOAD_TIMEOUT, "AT+UHTTPC=0,100,\"/%s\"\r\n", configData.filename);
            // int resp = RESP_OK;
            if (resp != RESP_ERROR) {
                cellular_lock(nullptr); // protect against concurrent access to httpsResp_ & cgevDeactProfile_
                if (httpsResp_.valid && httpsResp_.command == 100 && httpsResp_.result == 1 && httpsResp_.status_code == 200 && strcmp(httpsResp_.md5_sum, configData.md5sum)) {
                    modemFirmwareDownloadComplete = true;
                }
                cellular_unlock(nullptr);
                if (!modemFirmwareDownloadComplete) {
                    system_tick_t start = millis();
                    while (millis() - start < NCP_FW_MODEM_DOWNLOAD_TIMEOUT && !modemFirmwareDownloadComplete) {
                        cellular_lock(nullptr); // protect against concurrent access to httpsResp_ & cgevDeactProfile_
                        if (httpsResp_.valid && httpsResp_.command == 100 && httpsResp_.result == 1 && httpsResp_.status_code == 200 && !strcmp(httpsResp_.md5_sum, configData.md5sum)) {
                            modemFirmwareDownloadComplete = true;
                        }
                        if (httpsResp_.valid && httpsResp_.command == 100 && httpsResp_.result == 0) {
                            cellular_unlock(nullptr);
                            break;
                        }
                        cellular_unlock(nullptr);
                    }
                }
            }
        }

        cellular_remove_urc_handler("+UUHTTPCR");
#else // SARA_NCP_FW_UPDATE_ENABLE_DOWNLOAD
        // DEBUG
        modemFirmwareDownloadComplete = true;
#endif
        if (modemFirmwareDownloadComplete) {
            cellular_lock(nullptr); // protect against concurrent access to httpsResp_ & cgevDeactProfile_
            NCPFW_LOG_DEBUG(INFO, "command: %d, result: %d, status_code: %d, md5:[%s]",
                    httpsResp_.command, httpsResp_.result, httpsResp_.status_code, httpsResp_.md5_sum);
            NCPFW_LOG_DEBUG(INFO, "Download complete and verified.");
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_INSTALL_CELL_DISCONNECTING;
            NCPFW_LOG_DEBUG(INFO, "Disconnecting Cellular...");
            cgevDeactProfile_ = 0; // waiting for 1
            cellular_unlock(nullptr);
            startTimer_ = millis();
            network_disconnect(0, NETWORK_DISCONNECT_REASON_USER, 0);
        } else {
            NCPFW_LOG_DEBUG(INFO, "Download failed!!");
            HTTPSerror httpsError = {};
            SEND_COMMAND((_CALLBACKPTR_MDM)cbUHTTPER, (void*)&httpsError, 10000, "AT+UHTTPER=0\r\n");
            NCPFW_LOG_DEBUG(INFO, "UHTTPER class: %d, code: %d",
            httpsError.err_class, httpsError.err_code);
            if (++downloadRetries_ >= 3) {
                saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
                saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
                saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_DOWNLOAD_RETRY_MAX;
            }
        }
    }
    break;

    case FW_UPDATE_STATE_INSTALL_CELL_DISCONNECTING:
    {
        cellular_lock(nullptr); // protect against concurrent access to httpsResp_ & cgevDeactProfile_
        const int tempCgevDeactProfile = cgevDeactProfile_;
        cellular_unlock(nullptr);
        if (tempCgevDeactProfile == NCP_FW_UBLOX_DEFAULT_CID) { // Default CID detached
            cellular_unlock(nullptr);
            NCPFW_LOG_DEBUG(INFO, "Disconnected Cellular.");
            cellular_remove_urc_handler("+CGEV");
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_INSTALL_STARTING;
            cooldown(1000); // allow other disconnect URCs to pass
        }
        if (millis() - startTimer_ >= NCP_FW_MODEM_CLOUD_DISCONNECT_TIMEOUT) {
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_INSTALL_STARTING;
        }
    }
    break;

    case FW_UPDATE_STATE_INSTALL_STARTING:
    {
        atOkCheckTimer_ = millis();
        NCPFW_LOG_DEBUG(INFO, "Installing firmware, prepare to wait 25 - 32 minutes...");
        saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_UPDATING;

#if SARA_NCP_FW_UPDATE_ENABLE_INSTALL
        if (RESP_OK == SEND_COMMAND(nullptr, nullptr, 60000, "AT+UFWINSTALL=1,115200\r\n")) {
            saraNcpFwUpdateData_.state = FW_UPDATE_STATE_INSTALL_WAITING;
            saraNcpFwUpdateData_.status = saraNcpFwUpdateStatus_;
            saraNcpFwUpdateData_.error = saraNcpFwUpdateError_;
            saveSaraNcpFwUpdateData();
            // Wait for AT interface to become unresponsive, since we need to rely on that to break out of our install later
            bool atResponsive = true;
            while (atResponsive && millis() - atOkCheckTimer_ < NCP_FW_MODEM_INSTALL_START_TIMEOUT) {
                atResponsive = (RESP_OK == SEND_COMMAND(nullptr, nullptr, 3000, "AT\r\n"));
                if (atResponsive) {
                    delay(3000);
                }
            }
            if (atResponsive) {
                NCPFW_LOG_DEBUG(ERROR, "5 minute timeout waiting for INSTALL to start");
                saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_POWER_OFF;
                saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
                saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_START_INSTALL_TIMEOUT;
            } else {
                cellular_start_ncp_firmware_update(true, nullptr);
                saraNcpFwUpdateState_ = FW_UPDATE_STATE_INSTALL_WAITING;
            }
        } else {
            NCPFW_LOG_DEBUG(ERROR, "AT+UFWINSTALL failed to respond");
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_POWER_OFF;
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_INSTALL_AT_ERROR;
        }
        cooldown(10000);
#else // SARA_NCP_FW_UPDATE_ENABLE_INSTALL
        // DEBUG
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_INSTALL_WAITING;
        saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
        saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_START_INSTALL_TIMEOUT;
#endif
    }
    break;

    case FW_UPDATE_STATE_INSTALL_WAITING:
    {
        startingFirmwareVersion_ = firmwareVersion_;
        saraNcpFwUpdateData_.startingFirmwareVersion = startingFirmwareVersion_;
        saraNcpFwUpdateData_.state = saraNcpFwUpdateState_;
        saraNcpFwUpdateData_.status = saraNcpFwUpdateStatus_;
        saraNcpFwUpdateData_.error = saraNcpFwUpdateError_;
        saveSaraNcpFwUpdateData();
        cellular_start_ncp_firmware_update(true, nullptr); // ensure lower ncp layers play nice
        startTimer_ = millis();
        atOkCheckTimer_ = millis();
        int atResp = RESP_ERROR;
        // Wait for the install to take place, 18 - 20 or more minutes!
        while (true) {
            if (millis() - atOkCheckTimer_ >= NCP_FW_MODEM_INSTALL_ATOK_INTERVAL) {
                atOkCheckTimer_ = millis();
                atResp = SEND_COMMAND(nullptr, nullptr, 1000, "AT\r\n");
            }
            if (atResp == RESP_OK) {
                // We appear to be responsive again, if we missed the firmware UUFWINSTALL: 128 URC
                // See if we can break out of here with verifying an updated or same as before FW revision
                firmwareVersion_ = getNcpFirmwareVersion();
                NCPFW_LOG_DEBUG(INFO, "App fV: %" FMT_LU ", sfV: %" FMT_LU ", uV: %" FMT_LU, firmwareVersion_, startingFirmwareVersion_, updateVersion_);
                if (firmwareVersion_ == updateVersion_) {
                    saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_SUCCESS; // force a FW update success status
                    break;
                } else if (firmwareVersion_ == startingFirmwareVersion_) {
                    saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
                    saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_SAME_VERSION;
                    break; // fail early
                }
                delay(20000); // slow down the log output
            }
            if (millis() - startTimer_ >= NCP_FW_MODEM_INSTALL_FINISH_TIMEOUT) {
                NCPFW_LOG_DEBUG(ERROR, "Install process timed out!");
                saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
                saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_INSTALL_TIMEOUT;
                break;
            }
        }

        if (saraNcpFwUpdateStatus_ == FW_UPDATE_STATUS_SUCCESS) {
            NCPFW_LOG_DEBUG(INFO, "Firware update success.");
        } else {
            NCPFW_LOG_DEBUG(ERROR, "Firmware update failed!!");
        }
        // Power cycle the modem... who know's what odd state we are in.
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_POWER_OFF;
    }
    break;

    case FW_UPDATE_STATE_FINISHED_POWER_OFF:
    {
        NCPFW_LOG_DEBUG(INFO, "Power cycling modem and reconnecting to Cloud...");
        saraNcpFwUpdateData_.state = FW_UPDATE_STATE_FINISHED_POWER_OFF;
        saraNcpFwUpdateData_.status = saraNcpFwUpdateStatus_;
        saraNcpFwUpdateData_.error = saraNcpFwUpdateError_;
        saveSaraNcpFwUpdateData();
        cellular_start_ncp_firmware_update(false, nullptr);
        network_off(NETWORK_INTERFACE_CELLULAR, 0, 0, nullptr); // Cellular.off()
        startTimer_ = millis();
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_POWERING_OFF;
    }
    break;

    case FW_UPDATE_STATE_FINISHED_POWERING_OFF:
    {
        if (millis() - startTimer_ >= NCP_FW_MODEM_POWER_OFF_TIMEOUT) {
            NCPFW_LOG_DEBUG(ERROR, "Powering modem off timed out!");
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_POWER_OFF_TIMEOUT;
        }
        if (network_is_off(NETWORK_INTERFACE_CELLULAR, nullptr)) {
            NCPFW_LOG_DEBUG(INFO, "Powering modem off success");
            startTimer_ = millis();
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING;
            saraNcpFwUpdateData_.state = saraNcpFwUpdateState_;
            saraNcpFwUpdateData_.status = saraNcpFwUpdateStatus_;
            saraNcpFwUpdateData_.error = saraNcpFwUpdateError_;
            saveSaraNcpFwUpdateData();
        }
    }
    break;

    case FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING:
    {
        if (millis() - startTimer_ >= NCP_FW_MODEM_CLOUD_CONNECT_TIMEOUT) {
            if (++finishedCloudConnectingRetries_ < 2) {
                saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_POWER_OFF;
            } else {
                saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
                saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
                saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_CLOUD_CONNECT_ON_EXIT_TIMEOUT;
            }
        } else if (saraNcpFwUpdateCallbacks_.spark_cloud_flag_connected()) { // Particle.connected()
            saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED;
        } else {
            saraNcpFwUpdateStatusDiagnostics_ = saraNcpFwUpdateStatus_; // ready our diagnostics status value before connecting to the cloud
            saraNcpFwUpdateErrorDiagnostics_ = saraNcpFwUpdateError_;
            saraNcpFwUpdateCallbacks_.spark_cloud_flag_connect(); // Particle.connect()
            cooldown(1000);
        }
    }
    break;

    case FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED:
    {
        if (saraNcpFwUpdateCallbacks_.spark_cloud_flag_connected()) { // Particle.connected()
            if (!saraNcpFwUpdateCallbacks_.publishEvent("spark/device/ncp/update", (saraNcpFwUpdateStatus_ == FW_UPDATE_STATUS_SUCCESS) ? "success" : "failed", /*flags PUBLISH_EVENT_FLAG_PRIVATE*/1)) {
                saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
                saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_PUBLISH_RESULT;
            }
        } else {
            saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_FAILED;
            saraNcpFwUpdateError_ = SYSTEM_ERROR_SARA_NCP_FW_UPDATE_PUBLISH_RESULT;
        }
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_FINISHED_IDLE;
    }
    break;

    case FW_UPDATE_STATE_FINISHED_IDLE:
    {
        saraNcpFwUpdateData_.state = FW_UPDATE_STATE_FINISHED_IDLE;
        saraNcpFwUpdateData_.status = saraNcpFwUpdateStatus_;
        saraNcpFwUpdateData_.error = saraNcpFwUpdateError_;
        saraNcpFwUpdateData_.updateAvailable = SYSTEM_NCP_FW_UPDATE_UNKNOWN; // clear persistence after attempting an update
        saraNcpFwUpdateData_.isUserConfig = false; // clear persistence after attempting an update
        saveSaraNcpFwUpdateData();
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_IDLE;
    }
    break;

    case FW_UPDATE_STATE_IDLE:
    default:
    {
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_IDLE;
        if (saraNcpFwUpdateStatus_ != FW_UPDATE_STATUS_IDLE && inSafeMode()) {
            NCPFW_LOG_DEBUG(INFO, "Resetting out of Safe Mode!");
            delay(200);
            // Reset with graceful disconnect from Cloud
            system_reset(SYSTEM_RESET_MODE_NORMAL, 0, 0, 0, nullptr);
        }
    }
    break;
    } // switch end

    return SYSTEM_ERROR_NONE;
}

// static
int SaraNcpFwUpdate::cbUHTTPER(int type, const char* buf, int len, HTTPSerror* data)
{
    int err_class = 0;
    int err_code = 0;
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        // +UHTTPER: 0,10,22
        if (sscanf(buf, "%*[\r\n]+UHTTPER: %*d,%d,%d", &err_class, &err_code) >= 2) {
            data->err_class = err_class;
            data->err_code = err_code;
        }
    }
    return WAIT;
}

// static
int SaraNcpFwUpdate::cbULSTFILE(int type, const char* buf, int len, int* data)
{
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        if (strstr(buf, "+ULSTFILE: \"updatePackage.bin\"")) {
            *data = 1;
        }
    }
    return WAIT;
}

uint32_t SaraNcpFwUpdate::getNcpFirmwareVersion() {
    uint32_t version = 0;
    if (cellular_get_ncp_firmware_version(&version, nullptr) != SYSTEM_ERROR_NONE) {
        return 0; // specifically 0 here for version error
    }

    return version;
}

system_error_t SaraNcpFwUpdate::setupHTTPSProperties() {
    return setupHTTPSProperties_impl();
}

void SaraNcpFwUpdate::cooldown(system_tick_t timer) {
    cooldownTimeout_ = timer;
    cooldownTimer_ = millis();
}
void SaraNcpFwUpdate::updateCooldown() {
    if (cooldownTimer_ && millis() - cooldownTimer_ >= cooldownTimeout_) {
        cooldownTimer_ = 0;
        cooldownTimeout_ = 0;
    }
}
bool SaraNcpFwUpdate::inCooldown() {
    return cooldownTimer_ > 0;
}

} // namespace services

} // namespace particle

int sara_ncp_fw_update_config(const SaraNcpFwUpdateConfig* userConfigData, void* reserved) {
    return particle::services::SaraNcpFwUpdate::instance()->setConfig(userConfigData);
}

#else // #if HAL_PLATFORM_NCP_FW_UPDATE

int sara_ncp_fw_update_config(const SaraNcpFwUpdateConfig* userConfigData, void* reserved) {
    return SYSTEM_ERROR_NONE;
}

#endif // #if HAL_PLATFORM_NCP_FW_UPDATE

#endif // #if MODULE_FUNCTION != 2