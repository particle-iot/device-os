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

/*
    R510 Firmware Update:
    =====================
    1.  Check ncpId() == PLATFORM_NCP_SARA_R510
    2.  Is System.enableUpdates() == true?
    3.  Check modem firmware version - compatible upgrade version will be baked into Device OS with MD5 sum.
    4.  Reboot into Safe Mode to start the update process
    5.  Is Particle.connected()?
    6.  Setup HTTPS security options
    7.  When ready to start update process, publish a "spark/device/ncp/update" system event that Device OS has "started" a modem update.
        Only try to update once per hour if something should fail the first time, so we're not stuck in a hard
        loop offline?
    8.  Disconnect the Cloud
    9.  If an existing FOAT file is present, delete it since there is no way to validate what it is after it's present.
    10. Start the download based on step 3, keep track of update version
    11. Download is only complete when the the MD5SUM URC of the file is received and verified
    12. Disconnect from the Cellular network
    13. Apply the firmware update
    14. Sit in a tight loop it will take about 18 to 20 minutes
        a. waiting for final INSTALL URC of 128
        b. polling for AT/OK every 10 seconds
        c. monitoring a timeout counter of 40 minutes
    15. Save the download/install result to be published once connected to the Cloud again
    16. Power cycle the modem by switching it off, and..
    17. Connect to the Cloud
    18. Publish a "spark/device/ncp/update" system event that Device OS has finished with "success" or "failed"
    19. Reset the system to exit Safe Mode
    20. Add result status to device diagnostics

    TODO:
    ====================
    Done - Implement ncpId() check for R510
    Done - Implement System feature flag
    Done - Implement reboot/exit Safe Mode
    Done - Move ncp_fw_update.cpp to services/ instead of system/ since we have more space available there
    Done - Add correct cipher settings
    Done - Save state_, status_, firmwareVersion_ variables in retained system memory to ensure
            Done - we complete step 11/12
            Done - we do enter and exit safe mode fw updating cleanly
            Done - we do not get stuck in a fail-retry loop
    Done - Refactor retained structure to use systemCache instead
         - New system events must be worked into Device Service, add a story for this.
    Done - Adjust the 40 minute timeout dynamically based on the progress of the INSTALL URCs?
            Done - Gen 3, extends timer 5 minutes for every received URC.
    Done - When do we retry failures?  Quick loops can retry a few times, but long ones shouldn't
    Done - Create a database of starting firmware version / desired firmware version / filename / MD5SUM value, similar to APN DB.
         - Minimize logging output, move some statements to LOG_DEBUG
    Done - Make sure there is a final status log
    Done - Allow user firmware to pass a database entry that overrides the system firmware update database
    Done - Add final status to Device Diagnostics
         - add modem version to Device Diagnostics?
         - Make publishEvent's an async call with timeout?
         - When do we clear the g_ncpFwUpdateRetained.state == FW_UPDATE_FINISHED_IDLE_STATE to kickoff a new update attempt?
    Done - Remove callbacks
    Done - Remove Gen 2 code
    Done - Gen 3
            Done - allow normal connection at first for publishing
            Done - avoid connecting PPP for downloading
            Done - enable PDP context with a few AT commands for downloading
    Done - Remove System feature flag
         - implement background update check, only perform checks if modem is on.
         - ncp_fw_udpate_check should drive an update check, implement no argument as system check
         - implement Cellular.updatesPending() API
         - implement Cellular.startUpdate() API

*/

#include "logging.h"
LOG_SOURCE_CATEGORY("system.ncp.update");

#include "ncp_fw_update.h"
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
#include "platform_headers.h"
#include "system_network.h"

#if HAL_PLATFORM_NCP_FW_UPDATE

namespace particle {

namespace services {

namespace {

HTTPSresponse g_httpsResp = {};
int g_respCode = 0;
NcpFwUpdateDiagnostics g_ncpFwUpdateDiagnostics;

} // namespace

NcpFwUpdate::NcpFwUpdate() {
    foatReady_ = 0;
    startInstallTimer_ = 0;
    atOkCheckTimer_ = 0;
    g_respCode = -1;
    lastRespCode_ = -1;
    atResponsive_ = -1;
    atResp_ = 0;
    startingFirmwareVersion_ = -1;  // Keep these initialized differently to prevent
    firmwareVersion_ = -2;          // code from thinking an update was successful
    updateVersion_ = -3;            //  |
    isUserConfig_ = false;
    cooldownTimer_ = 0;
    cooldownTimeout_ = 0;
    initialized_ = false;
    ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
    ncpFwUpdateStatus_ = FW_UPDATE_IDLE_STATUS;
    ncpFwUpdateStatusDiagnostics_ = FW_UPDATE_NONE_STATUS; // initialize to none, only set when update process complete.
}

void NcpFwUpdate::init(NcpFwUpdateCallbacks* callbacks) {
    this->ncpFwUpdateCallbacks_ = callbacks;
    memset(ncpFwUpdateData_.data, 0, sizeof(ncpFwUpdateData_.data));
    recallNcpFwUpdateData_();
    validateNcpFwUpdateData_();
    if (system_mode() != System_Mode_TypeDef::SAFE_MODE) {
        // If not in safe mode, make sure to reset the firmware update state.
        if (ncpFwUpdateData_.var.state == FW_UPDATE_FINISHED_IDLE_STATE) {
            ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
            if (ncpFwUpdateData_.var.status != FW_UPDATE_IDLE_STATUS) {
                LOG(INFO, "Firmware update finished, %s status: %d", (ncpFwUpdateData_.var.status == FW_UPDATE_SUCCESS_STATUS) ? "success" : "failed", ncpFwUpdateData_.var.status);
                
                ncpFwUpdateStatusDiagnostics_ = ncpFwUpdateData_.var.status;
                
                ncpFwUpdateData_.var.status = FW_UPDATE_IDLE_STATUS;
                saveNcpFwUpdateData_();
            }
        } else if (ncpFwUpdateData_.var.state == FW_UPDATE_INSTALL_STATE_WAITING) {
            LOG(INFO, "Resuming update in Safe Mode!");
            HAL_Delay_Milliseconds(200);
            system_reset(SYSTEM_RESET_MODE_SAFE, 0, 0, SYSTEM_RESET_FLAG_NO_WAIT, nullptr);
        } else {
            ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE; // default to disable updates
            ncpFwUpdateStatus_ = FW_UPDATE_IDLE_STATUS;
        }
    } else {
        // Ensure we recall the previously set state
        ncpFwUpdateState_ = ncpFwUpdateData_.var.state;
        ncpFwUpdateStatus_ = ncpFwUpdateData_.var.status;
        firmwareVersion_ = ncpFwUpdateData_.var.firmwareVersion;
        startingFirmwareVersion_ = ncpFwUpdateData_.var.startingFirmwareVersion;
        updateVersion_ = ncpFwUpdateData_.var.updateVersion;
        isUserConfig_ = ncpFwUpdateData_.var.isUserConfig;
    }
    initialized_ = true;
}

int NcpFwUpdate::getStatusDiagnostics() {
    return ncpFwUpdateStatusDiagnostics_;
}

NcpFwUpdate* NcpFwUpdate::instance() {
    static NcpFwUpdate instance;
    return &instance;
}

NcpFwUpdate::~NcpFwUpdate() {
    // clean up?
}

int NcpFwUpdate::checkUpdate(const NcpFwUpdateConfig* userConfigData) {
    if (!userConfigData) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (ncpFwUpdateStatus_ != FW_UPDATE_IDLE_STATUS) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    validateNcpFwUpdateData_();
    LOG(INFO, "NcpFwUpdateConfig sv:%lu ev:%lu file:%s md5:%s",
                userConfigData->start_version,
                userConfigData->end_version,
                userConfigData->filename,
                userConfigData->md5sum);
    memcpy(&ncpFwUpdateData_.var.userConfigData, userConfigData, sizeof(NcpFwUpdateConfig));
    LOG(INFO, "NcpFwUpdateConfig sv:%lu ev:%lu file:%s md5:%s",
                ncpFwUpdateData_.var.userConfigData.start_version,
                ncpFwUpdateData_.var.userConfigData.end_version,
                ncpFwUpdateData_.var.userConfigData.filename,
                ncpFwUpdateData_.var.userConfigData.md5sum);
    ncpFwUpdateData_.var.isUserConfig = true;
    isUserConfig_ = true;
    cooldownTimer_ = 0;
    cooldownTimeout_ = 0;
    ncpFwUpdateState_ = FW_UPDATE_QUALIFY_FLAGS_STATE;

    return 0;
}

void NcpFwUpdate::validateNcpFwUpdateData_() { 
    if (ncpFwUpdateData_.var.header != NCP_FW_DATA_HEADER || ncpFwUpdateData_.var.footer != NCP_FW_DATA_FOOTER) {
        memset(ncpFwUpdateData_.data, 0, sizeof(ncpFwUpdateData_.data));
        ncpFwUpdateData_.var.header = NCP_FW_DATA_HEADER;
        ncpFwUpdateData_.var.state = FW_UPDATE_IDLE_STATE;
        ncpFwUpdateData_.var.status = FW_UPDATE_IDLE_STATUS;
        ncpFwUpdateData_.var.footer = NCP_FW_DATA_FOOTER;

        saveNcpFwUpdateData_();

        isUserConfig_ = false;
        ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
        ncpFwUpdateStatus_ = FW_UPDATE_IDLE_STATUS;
    }
}

int NcpFwUpdate::saveNcpFwUpdateData_() {
    union {
        NcpFwUpdateData var;
        uint8_t data[sizeof(NcpFwUpdateData)];
    } tempData;
    int result = SystemCache::instance().get(SystemCacheKey::NCP_FW_UPDATE_DATA, tempData.data, sizeof(tempData.data));
    if (result != sizeof(tempData.data) || /* memcmp(tempData.data, ncpFwUpdateData_.data, sizeof(tempData.data) != 0)) { // better but not working */
            tempData.var.header != ncpFwUpdateData_.var.header || 
            tempData.var.footer != ncpFwUpdateData_.var.footer ||
            tempData.var.state  != ncpFwUpdateData_.var.state ||
            tempData.var.status != ncpFwUpdateData_.var.status ||
            tempData.var.firmwareVersion != ncpFwUpdateData_.var.firmwareVersion ||
            tempData.var.startingFirmwareVersion != ncpFwUpdateData_.var.startingFirmwareVersion ||
            tempData.var.updateVersion != ncpFwUpdateData_.var.updateVersion ||
            tempData.var.isUserConfig != ncpFwUpdateData_.var.isUserConfig ||
            tempData.var.userConfigData.start_version != ncpFwUpdateData_.var.userConfigData.start_version ||
            tempData.var.userConfigData.start_version != ncpFwUpdateData_.var.userConfigData.end_version ||
            strcmp(tempData.var.userConfigData.filename, ncpFwUpdateData_.var.userConfigData.filename) != 0 ||
            strcmp(tempData.var.userConfigData.md5sum, ncpFwUpdateData_.var.userConfigData.md5sum) != 0) {
        LOG(INFO, "Reading ncpFwUpdateData header:%x footer:%x state:%d status:%d fv:%lu sfv:%lu uv:%lu",
                tempData.var.header,
                tempData.var.footer,
                tempData.var.state,
                tempData.var.status,
                tempData.var.firmwareVersion,
                tempData.var.startingFirmwareVersion,
                tempData.var.updateVersion);
        LOG(INFO, "iuc:%d sv:%lu ev:%lu file:%s md5:%s",
                tempData.var.isUserConfig,
                tempData.var.userConfigData.start_version,
                tempData.var.userConfigData.end_version,
                tempData.var.userConfigData.filename,
                tempData.var.userConfigData.md5sum);
        LOG(INFO, "Writing cached ncpFwUpdateData, size: %d", result);
        result = SystemCache::instance().set(SystemCacheKey::NCP_FW_UPDATE_DATA, ncpFwUpdateData_.data, sizeof(ncpFwUpdateData_.data));
    }
    return (result < 0) ? result : 0;
}

int NcpFwUpdate::recallNcpFwUpdateData_() {
    union {
        NcpFwUpdateData var;
        uint8_t data[sizeof(NcpFwUpdateData)];
    } tempData;
    int result = SystemCache::instance().get(SystemCacheKey::NCP_FW_UPDATE_DATA, tempData.data, sizeof(tempData.data));
    if (result != sizeof(tempData.data)) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    LOG(INFO, "Reading cached ncpFwUpdateData");
    memcpy(ncpFwUpdateData_.data, tempData.data, sizeof(ncpFwUpdateData_.data));
    LOG(INFO, "Reading ncpFwUpdateData header:%x footer:%x state:%d status:%d fv:%lu sfv:%lu uv:%lu",
            tempData.var.header,
            tempData.var.footer,
            tempData.var.state,
            tempData.var.status,
            tempData.var.firmwareVersion,
            tempData.var.startingFirmwareVersion,
            tempData.var.updateVersion);
    LOG(INFO, "iuc:%d sv:%lu ev:%lu file:%s md5:%s",
            tempData.var.isUserConfig,
            tempData.var.userConfigData.start_version,
            tempData.var.userConfigData.end_version,
            tempData.var.userConfigData.filename,
            tempData.var.userConfigData.md5sum);
    return 0;
}

int NcpFwUpdate::deleteNcpFwUpdateData_() {
    LOG(INFO, "Deleting cached ncpFwUpdateData");
    int result = SystemCache::instance().del(SystemCacheKey::NCP_FW_UPDATE_DATA);
    return (result < 0) ? result : 0;
}

int NcpFwUpdate::firmwareUpdateForVersion_(const int version) {
    for (size_t i = 0; i < NCP_FW_UPDATE_CONFIG_SIZE; ++i) {
        if (version == NCP_FW_UPDATE_CONFIG[i].start_version) {
            return i;
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int NcpFwUpdate::process() {
    SPARK_ASSERT(initialized_);

    // Make state changes more obvious
    static NcpFwUpdateState lastState = FW_UPDATE_IDLE_STATE;
    if (ncpFwUpdateState_ != lastState) {
        LOG(INFO, "=========================== ncpFwUpdateState_: %d", ncpFwUpdateState_);
    }
    lastState = ncpFwUpdateState_;

    validateNcpFwUpdateData_();

    if (inCooldown_()) {
        updateCooldown_();
    } else {
    switch (ncpFwUpdateState_) {

        case FW_UPDATE_QUALIFY_FLAGS_STATE:
        {
            ncpFwUpdateState_ = FW_UPDATE_QUALIFY_MODEM_ON_STATE;
            // Check NCP version
            if (platform_primary_ncp_identifier() != PLATFORM_NCP_SARA_R510) {
                LOG(ERROR, "PLATFORM_NCP != SARA_R510");
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                break;
            }
            LOG(INFO, "PLATFORM_NCP == SARA_R510");
            // Check system feature is enabled
            if (!HAL_Feature_Get(FEATURE_NCP_FW_UPDATES)) {
                LOG(ERROR, "FEATURE_NCP_FW_UPDATES disabled");
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                break;
            }
            LOG(INFO, "FEATURE_NCP_FW_UPDATES enabled");
            // Check if System.updatesEnabled()
            uint8_t updatesEnabled = 0;
            ncpFwUpdateCallbacks_->system_get_flag(SYSTEM_FLAG_OTA_UPDATE_ENABLED, &updatesEnabled, nullptr);
            if (!updatesEnabled) {
                LOG(ERROR, "System updates disabled");
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                break;
            }
            LOG(INFO, "System updates enabled");
            // Make sure device is on
            if (!network_is_on(NETWORK_INTERFACE_CELLULAR, nullptr)) { // !Cellular.isOn()
                LOG(INFO, "Turning Modem ON...");
                network_on(NETWORK_INTERFACE_CELLULAR, 0, 0, nullptr); // Cellular.on()
            }
            startInstallTimer_ = HAL_Timer_Get_Milli_Seconds();
        }
        break;

        case FW_UPDATE_QUALIFY_MODEM_ON_STATE:
        {
            if (network_is_on(NETWORK_INTERFACE_CELLULAR, nullptr)) { // Cellular.isOn()
                LOG(INFO, "Modem is on!");
                // Check firmware version requires an upgrade
                firmwareVersion_ = getAppFirmwareVersion_();
                LOG(INFO, "App firmware: %d", firmwareVersion_);
                if (firmwareVersion_ == 0) {
                    LOG(ERROR, "Modem has unknown firmware version");
                    ncpFwUpdateState_ = FW_UPDATE_QUALIFY_RETRY_STATE;
                    break;
                }
                if ((!isUserConfig_ && firmwareUpdateForVersion_(firmwareVersion_) == SYSTEM_ERROR_NOT_FOUND) ||
                    (isUserConfig_ && firmwareVersion_ != ncpFwUpdateData_.var.userConfigData.start_version)) {
                    LOG(INFO, "No firmware update available");
                    ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                    break;
                }
                LOG(INFO, "Setup Fully Qualified!");
                // If not in Safe Mode, reset now into Safe Mode!
                ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECT_STATE;
                ncpFwUpdateData_.var.state = FW_UPDATE_SETUP_CLOUD_CONNECT_STATE;
                ncpFwUpdateData_.var.firmwareVersion = firmwareVersion_;
                saveNcpFwUpdateData_();
                if (system_mode() != System_Mode_TypeDef::SAFE_MODE) {
                    LOG(INFO, "Resetting into Safe Mode!");
                    HAL_Delay_Milliseconds(200);
                    system_reset(SYSTEM_RESET_MODE_SAFE, 0, 0, SYSTEM_RESET_FLAG_NO_WAIT, nullptr);
                    // Goodbye!  See you next time through in Safe Mode ;-)
                }
            } else if (HAL_Timer_Get_Milli_Seconds() - startInstallTimer_ >= NCP_FW_MODEM_POWER_ON_TIMEOUT) {
                LOG(ERROR, "Modem did not power on!");
                ncpFwUpdateState_ = FW_UPDATE_QUALIFY_RETRY_STATE;
            }
        }
        break;

        case FW_UPDATE_QUALIFY_RETRY_STATE:
        {
            static int qualify_retries = 0;
            if (++qualify_retries < 2) {
                ncpFwUpdateState_ = FW_UPDATE_QUALIFY_FLAGS_STATE;
            } else {
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
            }
        }
        break;

        case FW_UPDATE_SETUP_CLOUD_CONNECT_STATE:
        {
            if (!ncpFwUpdateCallbacks_->spark_cloud_flag_connected()) { // !Particle.connected()
                LOG(INFO, "Connect to Cloud...");
                ncpFwUpdateCallbacks_->spark_cloud_flag_connect(); // Particle.connect()
            }
            ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE;
            ncpFwUpdateStatus_ = FW_UPDATE_DOWNLOADING_STATUS;
            ncpFwUpdateStatusDiagnostics_ = ncpFwUpdateStatus_; // ready our diagnostics status value before connecting to the cloud
        }
        break;

        case FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE:
        {
            if (ncpFwUpdateCallbacks_->spark_cloud_flag_connected()) { // Particle.connected()
                LOG(INFO, "Connected to Cloud.");
                ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTED_STATE;
            } else {
                ncpFwUpdateCallbacks_->spark_cloud_flag_connect(); // Particle.connect()
            }
        }
        break;

        case FW_UPDATE_SETUP_CLOUD_CONNECTED_STATE:
        {
            if (ncpFwUpdateCallbacks_->spark_cloud_flag_connected()) { // Particle.connected()
                LOG(INFO, "publishEvent");
                if (ncpFwUpdateCallbacks_->publishEvent("spark/device/ncp/update", "started", /*flags PUBLISH_EVENT_FLAG_PRIVATE*/1)) {
                    ncpFwUpdateState_ = FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECT_STATE;
                    LOG(INFO, "Ready to start download...");
                    cooldown_(20000);
                } else {
                    static int publish_start_retries = 0;
                    if (++publish_start_retries < 2) {
                        ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE;
                    } else {
                        ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                    }
                    cooldown_(1000);
                }
            } else {
                ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE;
            }
        }
        break;

        case FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECT_STATE:
        {
            if (ncpFwUpdateCallbacks_->spark_cloud_flag_connected()) { // Particle.connected()
                ncpFwUpdateCallbacks_->spark_cloud_flag_disconnect(); // Particle.disconnect()
            } else {
                LOG(INFO, "Disconnected from Cloud.");
                if (network_ready(0, 0, 0)) { // Cellular.ready()
                    LOG(INFO, "Disconnecting Cellular...");
                    network_disconnect(0, NETWORK_DISCONNECT_REASON_USER, 0);
                    ncpFwUpdateState_ = FW_UPDATE_DOWNLOAD_CELL_DISCONNECTING_STATE;
                    cooldown_(10000);
                }
            }
        }
        break;

        case FW_UPDATE_DOWNLOAD_CELL_DISCONNECTING_STATE:
        {
            if (!network_ready(0, 0, 0)) { // Cellular.ready()
                LOG(INFO, "Disconnected Cellular.");
                ncpFwUpdateState_ = FW_UPDATE_DOWNLOAD_CELL_CONNECTING_STATE;
            }
        }
        break;

        case FW_UPDATE_DOWNLOAD_CELL_CONNECTING_STATE:
        {
            if (!network_ready(0, 0, 0)) { // Cellular.ready()
                LOG(INFO, "Connecting Cellular...");
                cellular_start_ncp_firmware_update(true, nullptr); // ensure we don't connect PPP
                network_connect(0, 0, 0, 0); // Cellular.connect()
                ncpFwUpdateState_ = FW_UPDATE_DOWNLOAD_HTTPS_SETUP_STATE;
            }
        }
        break;

        case FW_UPDATE_DOWNLOAD_HTTPS_SETUP_STATE:
        {
            static int https_setup_retries = 0;
            int ret = setupHTTPSProperties_();
            if (ret) {
                LOG(INFO, "HTTPS setup error: %d", ret);
                if (++https_setup_retries > 2) {
                    ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                }
                cooldown_(10000);
                break;
            }
            ncpFwUpdateState_ = FW_UPDATE_DOWNLOAD_READY_STATE;
        }
        break;

        case FW_UPDATE_DOWNLOAD_READY_STATE:
        {
            bool modemFirmwareDownloadComplete = false;
            foatReady_ = 0;
            memset(&g_httpsResp, 0, sizeof(g_httpsResp));

/*
a2773f2abb80df2886dd29b07f089504  SARA-R510S-00B-00_FW02.05_A00.01_IP_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
48b2d022041ea85899a15351c06a18d2  SARA-R510S-00B-01_FW02.06_A00.01_IP.dof
252ea04a324e9aab8a69678cfe097465  SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
ccfdc48c0a45198d6e168b30d0740959  SARA-R510S-00B-01_FW02.06_A00.01_IP_SARA-R510S-00B-01_FW99.01_A00.01.upd
5fd6c0d3d731c097605895b86f28c2cf  SARA-R510S-00B-01_FW99.01_A00.01_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
09c1a98d03c761bcbea50355f9b2a50f  SARA-R510S-01B-00-ES-0314A0001-005K00_SARA-R510S-01B-00-XX-0314ENG0099A0001-005K00.upd
09c1a98d03c761bcbea50355f9b2a50f  SARA-R510S-01B-00-ES-0314A0001_SARA-R510S-01B-00-XX-0314ENG0099A0001.upd
136caf2883457093c9e41fda3c6a44e3  SARA-R510S-01B-00-XX-0314ENG0099A0001-005K00_SARA-R510S-01B-00-ES-0314A0001-005K00.upd
136caf2883457093c9e41fda3c6a44e3  SARA-R510S-01B-00-XX-0314ENG0099A0001_SARA-R510S-01B-00-ES-0314A0001.upd

*/
            // There doesn't appear to be a way to test what updatePackage.bin actually IS (MD5 sum or APP version)
            // so if one exists, we must delete it before attempting to upgrade or else the FW INSTALL will
            // basically end early without the right version being applied.
            LOG(INFO, "Existing FOAT file?");
            foatReady_ = 0;
            cellular_command((_CALLBACKPTR_MDM)cbULSTFILE_, (void*)&foatReady_, 10000, "AT+ULSTFILE=0,\"FOAT\"\r\n");
            if (foatReady_) {
                if (RESP_OK == cellular_command(nullptr, nullptr, 10000, "AT+UDELFILE=\"updatePackage.bin\",\"FOAT\"\r\n")) {
                    LOG(INFO, "updatePackage.bin deleted.");
                }
            }

            auto mgr = cellularNetworkManager();
            auto client = mgr->ncpClient();
            auto parser = client->atParser();
            // +UUHTTPCR: 0,100,1,200,"ccfdc48c0a45198d6e168b30d0740959"
            parser->addUrcHandler("+UUHTTPCR", httpRespCallback_, client);

            LOG(INFO, "Starting download...");
// DEBUG set to 0 to disable download for testing
#if 1
            const int fwIdx = firmwareUpdateForVersion_(firmwareVersion_);
            if (isUserConfig_ || fwIdx != SYSTEM_ERROR_NOT_FOUND) {
                NcpFwUpdateConfig* configData;
                if (isUserConfig_) {
                    configData = &ncpFwUpdateData_.var.userConfigData;
                } else {
                    configData = (NcpFwUpdateConfig*)&NCP_FW_UPDATE_CONFIG[fwIdx];
                }
                updateVersion_ = configData->end_version;
                ncpFwUpdateData_.var.updateVersion = updateVersion_;
                saveNcpFwUpdateData_();
                // 99010001 -> 02060001 firmware (takes 1.8 - 2.8 minutes)
                int resp = cellular_command((_CALLBACKPTR_MDM)cbUUHTTPCR_, (void*)&g_httpsResp, 5*60000, "AT+UHTTPC=0,100,\"/%s\"\r\n", configData->filename);
                if (resp != RESP_ERROR) {
                    if (g_httpsResp.command == 100 && g_httpsResp.result == 1 && g_httpsResp.status_code == 200 && strcmp(g_httpsResp.md5_sum, configData->md5sum)) {
                        modemFirmwareDownloadComplete = true;
                    }
                    if (!modemFirmwareDownloadComplete) {
                        system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                        while (HAL_Timer_Get_Milli_Seconds() - start < 300000 && !modemFirmwareDownloadComplete) {
                            if (g_httpsResp.command == 100 && g_httpsResp.result == 1 && g_httpsResp.status_code == 200 && !strcmp(g_httpsResp.md5_sum, configData->md5sum)) {
                                modemFirmwareDownloadComplete = true;
                            }
                            HAL_Delay_Milliseconds(100);
                            if (g_httpsResp.command == 100 && g_httpsResp.result == 0) {
                                break;
                            }
                        }
                    }
                }
            }
#else
            // DEBUG
            modemFirmwareDownloadComplete = true;
#endif
            if (modemFirmwareDownloadComplete) {
                LOG(INFO, "command: %d, result: %d, status_code: %d, md5:[%s]",
                        g_httpsResp.command, g_httpsResp.result, g_httpsResp.status_code, g_httpsResp.md5_sum);
                LOG(INFO, "Download complete and verified.");
                ncpFwUpdateState_ = FW_UPDATE_INSTALL_CELL_DISCONNECTING_STATE;
                LOG(INFO, "Disconnecting Cellular...");
                network_disconnect(0, NETWORK_DISCONNECT_REASON_USER, 0);
            } else {
                LOG(INFO, "Download failed!!");
                cellular_command((_CALLBACKPTR_MDM)cbUHTTPER_, (void*)&g_httpsResp, 10000, "AT+UHTTPER=0\r\n");
                LOG(INFO, "UHTTPER class: %d, code: %d",
                g_httpsResp.err_class, g_httpsResp.err_code);
                static int download_retries = 0;
                if (++download_retries >= 3) {
                    ncpFwUpdateState_ = FW_UPDATE_FINISHED_IDLE_STATE;
                    ncpFwUpdateStatus_ = FW_UPDATE_FAILED_DOWNLOAD_RETRY_MAX_STATUS;
                }
            }
            cooldown_(10000);
        }
        break;

        case FW_UPDATE_INSTALL_CELL_DISCONNECTING_STATE:
        {
            // FIXME: This needs to wait for 
            // 0000091895 [ncp.at] TRACE: < +CGEV: ME PDN DEACT 1
            // 0000091896 [ncp.at] TRACE: < +CGEV: ME DETACH
            // 0000091896 [ncp.at] TRACE: < +UUPSDD: 0
            // or
            // COPS: 0  (no ACT)
            if (!network_ready(0, 0, 0)) { // Cellular.ready()
                LOG(INFO, "Disconnected Cellular.");
                ncpFwUpdateState_ = FW_UPDATE_INSTALL_STATE_STARTING;
            }
        }
        break;

        case FW_UPDATE_INSTALL_STATE_STARTING:
        {
            atOkCheckTimer_ = HAL_Timer_Get_Milli_Seconds();
            g_respCode = -1;
            lastRespCode_ = -1;
            LOG(INFO, "Installing firmware, prepare to wait 25 - 32 minutes...");
            ncpFwUpdateStatus_ = FW_UPDATE_UPDATING_STATUS;

// DEBUG set to 0 to disable update command
#if 1
            if (RESP_OK == cellular_command(nullptr, nullptr, 60000, "AT+UFWINSTALL=1,115200\r\n")) {
                ncpFwUpdateData_.var.state = FW_UPDATE_INSTALL_STATE_WAITING;
                ncpFwUpdateData_.var.status = ncpFwUpdateStatus_;
                saveNcpFwUpdateData_();                
                // Wait for AT interface to become unresponsive, since we need to rely on that to break out of our install later
                atResponsive_ = 1;
                while (atResponsive_ && HAL_Timer_Get_Milli_Seconds() - atOkCheckTimer_ < NCP_FW_MODEM_INSTALL_START_TIMEOUT) {
                    atResponsive_ = (RESP_OK == cellular_command(nullptr, nullptr, 3000, "AT\r\n"));
                    if (atResponsive_) {
                        HAL_Delay_Milliseconds(3000);
                    }
                }
                if (atResponsive_) {
                    LOG(ERROR, "5 minute timeout waiting for INSTALL to start");
                    ncpFwUpdateStatus_ = FW_UPDATE_FAILED_START_INSTALL_TIMEOUT_STATUS;
                    ncpFwUpdateState_ = FW_UPDATE_FINISHED_POWER_OFF_STATE;
                } else {
                    cellular_start_ncp_firmware_update(true, nullptr);
                    ncpFwUpdateState_ = FW_UPDATE_INSTALL_STATE_WAITING;
                }
            } else {
                LOG(ERROR, "AT+UFWINSTALL failed to respond");
                ncpFwUpdateStatus_ = FW_UPDATE_FAILED_INSTALL_AT_ERROR_STATUS;
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_POWER_OFF_STATE;
            }
            cooldown_(10000);
#else
            // DEBUG
            ncpFwUpdateState_ = FW_UPDATE_INSTALL_STATE_WAITING;
            ncpFwUpdateStatus_ = FW_UPDATE_FAILED_STATUS;
#endif
        }
        break;

        case FW_UPDATE_INSTALL_STATE_WAITING:
        {
            startingFirmwareVersion_ = firmwareVersion_;
            ncpFwUpdateData_.var.startingFirmwareVersion = startingFirmwareVersion_;
            ncpFwUpdateData_.var.state = ncpFwUpdateState_;
            ncpFwUpdateData_.var.status = ncpFwUpdateStatus_;
            saveNcpFwUpdateData_();
            cellular_start_ncp_firmware_update(true, nullptr); // ensure lower ncp layers play nice
            startInstallTimer_ = HAL_Timer_Get_Milli_Seconds();
            atOkCheckTimer_ = HAL_Timer_Get_Milli_Seconds();
            g_respCode = -1;
            lastRespCode_ = -1;
            // Wait for the install to take place, 18 - 20 or more minutes!
            while (true) {
                if (HAL_Timer_Get_Milli_Seconds() - atOkCheckTimer_ >= 10000) {
                    atOkCheckTimer_ = HAL_Timer_Get_Milli_Seconds();
                    atResp_ = cellular_command(nullptr, nullptr, 1000, "AT\r\n");
                }
                if (g_respCode != lastRespCode_) {
                    // Extend our timeout if we are receiving response updates
                    startInstallTimer_ = HAL_Timer_Get_Milli_Seconds();
                    LOG(INFO, "INSTALL: %d%%", g_respCode);
                    lastRespCode_ = g_respCode;
                    if (g_respCode == NCP_FW_UUFWINSTALL_COMPLETE) {
                        ncpFwUpdateStatus_ = FW_UPDATE_SUCCESS_STATUS;
                        break;
                    }
                }
                if (atResp_ == RESP_OK) {
                    // We appear to be responsive again, if we missed the firmware UUFWINSTALL: 128 URC
                    // See if we can break out of here with verifying an updated or same as before FW revision
                    firmwareVersion_ = getAppFirmwareVersion_();
                    LOG(INFO, "App fV: %d, sfV: %d, uV: %d", firmwareVersion_, startingFirmwareVersion_, updateVersion_);
                    if (firmwareVersion_ == updateVersion_) {
                        ncpFwUpdateStatus_ = FW_UPDATE_SUCCESS_STATUS; // force a FW update success status
                        break;
                    } else if (firmwareVersion_ == startingFirmwareVersion_) {
                        ncpFwUpdateStatus_ = FW_UPDATE_FAILED_SAME_VERSION_STATUS;
                        break; // fail early
                    }
                    HAL_Delay_Milliseconds(20000); // slow down the log output
                }
                if (HAL_Timer_Get_Milli_Seconds() - startInstallTimer_ >= NCP_FW_MODEM_INSTALL_FINISH_TIMEOUT) {
                    LOG(ERROR, "Install process timed out!");
                    ncpFwUpdateStatus_ = FW_UPDATE_FAILED_INSTALL_TIMEOUT_STATUS;
                }
            }

            if (ncpFwUpdateStatus_ == FW_UPDATE_SUCCESS_STATUS) {
                LOG(INFO, "Firware update success.");
            } else {
                LOG(ERROR, "Firmware update failed!!");
            }
            // Power cycle the modem... who know's what odd state we are in.
            ncpFwUpdateState_ = FW_UPDATE_FINISHED_POWER_OFF_STATE;
        }
        break;

        case FW_UPDATE_FINISHED_POWER_OFF_STATE:
        {
            LOG(INFO, "Power cycling modem and reconnecting to Cloud...");
            ncpFwUpdateData_.var.state = FW_UPDATE_FINISHED_POWER_OFF_STATE;
            ncpFwUpdateData_.var.status = ncpFwUpdateStatus_;
            saveNcpFwUpdateData_();
            cellular_start_ncp_firmware_update(false, nullptr);
            network_off(NETWORK_INTERFACE_CELLULAR, 0, 0, nullptr); // Cellular.off()
            startInstallTimer_ = HAL_Timer_Get_Milli_Seconds();
            ncpFwUpdateState_ = FW_UPDATE_FINISHED_POWERING_OFF_STATE;
        }
        break;

        case FW_UPDATE_FINISHED_POWERING_OFF_STATE:
        {
            if (HAL_Timer_Get_Milli_Seconds() - startInstallTimer_ > 60000) {
                LOG(ERROR, "Powering modem off timed out!");
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_IDLE_STATE;
                ncpFwUpdateStatus_ = FW_UPDATE_FAILED_POWER_OFF_TIMEOUT_STATUS;
            }
            if (network_is_off(NETWORK_INTERFACE_CELLULAR, nullptr)) {
                LOG(INFO, "Powering modem off success");
                startInstallTimer_ = HAL_Timer_Get_Milli_Seconds();
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_CLOUD_CONNECTING_STATE;
                ncpFwUpdateData_.var.state = FW_UPDATE_FINISHED_CLOUD_CONNECTING_STATE;
                ncpFwUpdateData_.var.status = ncpFwUpdateStatus_;
                saveNcpFwUpdateData_();
            }
        }
        break;

        case FW_UPDATE_FINISHED_CLOUD_CONNECTING_STATE:
        {
            if (HAL_Timer_Get_Milli_Seconds() - startInstallTimer_ >= NCP_FW_MODEM_CLOUD_CONNECT_TIMEOUT) {
                static int cloud_connecting_retries = 0;
                if (++cloud_connecting_retries < 2) {
                    ncpFwUpdateState_ = FW_UPDATE_FINISHED_POWER_OFF_STATE;
                } else {
                    ncpFwUpdateState_ = FW_UPDATE_FINISHED_IDLE_STATE;
                    ncpFwUpdateStatus_ = FW_UPDATE_FAILED_CLOUD_CONNECT_TIMEOUT_STATUS;
                }
            } else if (ncpFwUpdateCallbacks_->spark_cloud_flag_connected()) { // Particle.connected()
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_CLOUD_CONNECTED_STATE;
                cooldown_(5000);
            } else {
                ncpFwUpdateStatusDiagnostics_ = ncpFwUpdateStatus_; // ready our diagnostics status value before connecting to the cloud
                ncpFwUpdateCallbacks_->spark_cloud_flag_connect(); // Particle.connect()
                cooldown_(1000);
            }
        }
        break;

        case FW_UPDATE_FINISHED_CLOUD_CONNECTED_STATE:
        {
            if (ncpFwUpdateCallbacks_->spark_cloud_flag_connected()) { // Particle.connected()
                if (!ncpFwUpdateCallbacks_->publishEvent("spark/device/ncp/update", (ncpFwUpdateStatus_ == FW_UPDATE_SUCCESS_STATUS) ? "success" : "failed", /*flags PUBLISH_EVENT_FLAG_PRIVATE*/1)) {
                    ncpFwUpdateStatus_ = FW_UPDATE_FAILED_PUBLISH_RESULT_STATUS;
                }
            } else {
                ncpFwUpdateStatus_ = FW_UPDATE_FAILED_PUBLISH_RESULT_STATUS;
            }
            ncpFwUpdateState_ = FW_UPDATE_FINISHED_IDLE_STATE;
        }
        break;

        case FW_UPDATE_FINISHED_IDLE_STATE:
        {
            ncpFwUpdateData_.var.state = FW_UPDATE_FINISHED_IDLE_STATE;
            ncpFwUpdateData_.var.status = ncpFwUpdateStatus_;
            saveNcpFwUpdateData_();
            ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
        }
        break;

        case FW_UPDATE_IDLE_STATE:
        default:
        {
            ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
            if (ncpFwUpdateStatus_ != FW_UPDATE_IDLE_STATUS &&
                    system_mode() == System_Mode_TypeDef::SAFE_MODE) {
                ncpFwUpdateData_.var.isUserConfig = false;
                isUserConfig_ = false;
                saveNcpFwUpdateData_();
                LOG(INFO, "Resetting out of Safe Mode!");
                HAL_Delay_Milliseconds(200);
                // Reset with graceful disconnect from Cloud
                system_reset(SYSTEM_RESET_MODE_NORMAL, 0, 0, 0, nullptr);
            }
        }
        break;
    } // switch end
    } // Not in cooldown

    return SYSTEM_ERROR_NONE;
}

// static
int NcpFwUpdate::cbUUHTTPCR_(int type, const char* buf, int len, HTTPSresponse* data)
{
    int command = 0;
    int result = 0;
    int status_code = 0;
    char md5_sum[40] = {0};
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        // +UUHTTPCR: 0,100,1,200,"ccfdc48c0a45198d6e168b30d0740959"
        if (sscanf(buf, "%*[\r\n]+UUHTTPCR: %*d,%d,%d,%d,\"%32s\"", &command, &result, &status_code, md5_sum) >= 3) {
            data->command = command;
            data->result = result;
            data->status_code = status_code;
            memcpy(data->md5_sum, &md5_sum, sizeof(md5_sum));
        }
    }
    return WAIT;
}

// static
int NcpFwUpdate::cbUHTTPER_(int type, const char* buf, int len, HTTPSresponse* data)
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
int NcpFwUpdate::cbULSTFILE_(int type, const char* buf, int len, int* data)
{
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        if (strstr(buf, "+ULSTFILE: \"updatePackage.bin\"")) {
            *data = 1;
        }
    }
    return WAIT;
}

// static
int NcpFwUpdate::cbATI9_(int type, const char* buf, int len, int* val)
{
    int major1 = 0;
    int minor1 = 0;
    int major2 = 0;
    int minor2 = 0;
    char eng[10] = {0};
    if (val && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        if (sscanf(buf, "%*[\r\nL0.0.00.00.]%d.%d%*[,A.]%d.%d", &major1, &minor1, &major2, &minor2) == 4) {
            *val = major1 * 1000000 + minor1 * 10000 + major2 * 100 + minor2;
        } else if (sscanf(buf, "%*[\r\nL0.0.00.00.]%d.%d%8[^,]%*[,A.]%d.%d", &major1, &minor1, eng, &major2, &minor2) == 5) {
            *val = major1 * 1000000 + minor1 * 10000 + major2 * 100 + minor2;
            if (strstr(eng, "_ENG")) {
                *val += NCP_FW_UBLOX_R510_ENG_VERSION; // Add leading 1 for _ENGxxxx firmware
            }
        }
    }
    return WAIT;
}

// static
int NcpFwUpdate::cbUPSND_(int type, const char* buf, int len, int* data)
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

// static
int NcpFwUpdate::cbCOPS_(int type, const char* buf, int len, int* data)
{
    int act;
    // +COPS: 0,2,"310410",7
    // +COPS: 0,0,"AT&T",7
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        if (sscanf(buf, "\r\n+COPS: %*d,%*d,\"%*6[0-9]\",%d", &act) >= 1) {
            if (act >= 7) {
                *data = 1;
            }
        } else if (sscanf(buf, "\r\n+COPS: %*d,%*d,\"%*[^\"]\",%d", &act) >= 1) {
            if (act >= 7) {
                *data = 1;
            }
        }
    }
    return WAIT;
}

int NcpFwUpdate::getAppFirmwareVersion_() {
    // ATI9 (get version and app version)
    // example output
    // "02.05,A00.01" R510 (older)               - v2050001
    // "02.06,A00.01" R510 (newer)               - v2060001
    // "03.15_ENG0001,A00.01" (engineering)      - v103150001
    // "03.15,A00.01" (newest)                   - v3150001
    // "08.70,A00.02" G350 (older)               - v8700002
    // "08.90,A01.13" G350 (newer)               - v8900113
    // "L0.0.00.00.05.06,A.02.00" (memory issue) - v5060200
    // "L0.0.00.00.05.07,A.02.02" (demonstrator) - v5070202
    // "L0.0.00.00.05.08,A.02.04" (maintenance)  - v5080204
    int appVer = 0;
    cellular_command((_CALLBACKPTR_MDM)cbATI9_, (void*)&appVer, 10000, "ATI9\r\n");
    return appVer;
}

int NcpFwUpdate::setupHTTPSProperties_() {
    // Wait for registration, using COPS AcT to determine this instead of CEREG.
    unsigned int registered = 0;
    uint32_t start = HAL_Timer_Get_Milli_Seconds();
    do {
        cellular_command((_CALLBACKPTR_MDM)cbCOPS_, (void*)&registered, 10000, "AT+COPS?\r\n");
        if (!registered) HAL_Delay_Milliseconds(15000);
    } while (!registered && HAL_Timer_Get_Milli_Seconds() - start < 15 * 60 * 1000);

    cellular_command(nullptr, nullptr, 10000, "AT+CMEE=2\r\n");
    cellular_command(nullptr, nullptr, 10000, "AT+CGPADDR=0\r\n");
    // Activate data connection
    int actVal = 0;
    cellular_command((_CALLBACKPTR_MDM)cbUPSND_, (void*)&actVal, 10000, "AT+UPSND=0,8\r\n");
    if (actVal != 1) {
        cellular_command(nullptr, nullptr, 10000, "AT+UPSD=0,100,1\r\n");
        cellular_command(nullptr, nullptr, 10000, "AT+UPSD=0,0,0\r\n");
        int r = cellular_command(nullptr, nullptr, 180000, "AT+UPSDA=0,3\r\n");
        if (r == RESP_ERROR) {
            cellular_command(nullptr, nullptr, 10000, "AT+CEER\r\n");
        }
    }

    // Setup security settings
    LOG(INFO, "setupHTTPSProperties_");
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,0,3\r\n")) return -1; // Highest level (3) root cert checks
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,1,3\r\n")) return -2; // Minimum TLS v1.2
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,2,99,\"C0\",\"2F\"\r\n")) return -3; // Cipher suite
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,3,\"ubx_digicert_global_root_ca\"\r\n")) return -4; // Cert name
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,4,\"fw-ftp.staging.particle.io\"\r\n")) return -5; // Expected server hostname
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,10,\"fw-ftp.staging.particle.io\"\r\n")) return -6; // SNI (Server Name Indication)
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,1,\"fw-ftp.staging.particle.io\"\r\n")) return -7;
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,5,443\r\n")) return -8;
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,6,1,2\r\n")) return -9;
    return 0;
}

// static
int NcpFwUpdate::httpRespCallback_(AtResponseReader* reader, const char* prefix, void* data) {
    // const auto self = (SaraNcpClient*)data;
    int a, b, c;
    char s[40];
    char atResponse[64] = {};
    // FIXME: Can't get CHECK_PARSER_URC to work, do we need self->parserError(_r); ?
    const auto resp = reader->readLine(atResponse, sizeof(atResponse));
    if (resp < 0) {
        return resp;
    }
    int r = ::sscanf(atResponse, "+UUHTTPCR: %*d,%d,%d,%d,\"%32s\"", &a, &b, &c, s);
    CHECK_TRUE(r >= 3, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

    g_httpsResp.command = a;
    g_httpsResp.result = b;
    g_httpsResp.status_code = c;
    memcpy(g_httpsResp.md5_sum, &s, sizeof(s));
    LOG(INFO, "UUHTTPCR matched");

    return SYSTEM_ERROR_NONE;
}

void NcpFwUpdate::cooldown_(system_tick_t timer) {
    cooldownTimeout_ = timer;
    cooldownTimer_ = HAL_Timer_Get_Milli_Seconds();
}
void NcpFwUpdate::updateCooldown_() {
    if (cooldownTimer_ && HAL_Timer_Get_Milli_Seconds() - cooldownTimer_ >= cooldownTimeout_) {
        cooldownTimer_ = 0;
        cooldownTimeout_ = 0;
    }
}
bool NcpFwUpdate::inCooldown_() {
    return cooldownTimer_ > 0;
}

} // namespace services

} // namespace particle

int ncp_fw_udpate_check(const NcpFwUpdateConfig* userConfigData, void* reserved) {
    // LOG(INFO,"CHECK");
    return particle::services::NcpFwUpdate::instance()->checkUpdate(userConfigData);
}

#else // #if HAL_PLATFORM_NCP_FW_UPDATE

int ncp_fw_udpate_check(const NcpFwUpdateConfig* userConfigData, void* reserved) {
    return 0;
}

#endif // #if HAL_PLATFORM_NCP_FW_UPDATE
