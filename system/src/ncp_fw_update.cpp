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
    2.  Check modem firmware version - compatible upgrade version will be baked into Device OS with MD5 sum.
    3.  Is System feature for NCP updates enabled?
    4.  Is System.enableUpdates() == true?
    5.  Reboot into Safe Mode to start the update process
    6.  Is Particle.connected()?
    7.  Setup HTTPS security options
    8.  When ready to start update process, publish a "spark/device/ncp/update" system event that Device OS has "started" a modem update.
        Only try to update once per hour if something should fail the first time, so we're not stuck in a hard
        loop offline?
    9.  Disconnect the Cloud
    10.  If an existing FOAT file is present, delete it since there is no way to validate what it is after it's present.
    11.  Start the download based on step 1, keep track of update version
    12.  Download is only complete when the the MD5SUM URC of the file is received and verified
    13. Disconnect from the Cellular network
    14. Apply the firmware update (SWEAT)
    15. Sit in a tight loop it will take about 25 to 32 minutes (SWEAT)
        a. waiting for final INSTALL URC of 128
        b. polling for AT/OK every 10 seconds
        c. monitoring a timeout counter of 40 minutes
    16. Save the install result to be published once connected to the Cloud again
    17. Factory reset the modem pass or fail since we don't know what state it's in
    18. Power cycle the modem by switching it off, and..
    19. Connect to the Cloud
    20. Publish a "spark/device/ncp/update" system event that Device OS has finished with "success" or "failed"
    21. Reset the system to exit Safe Mode

    TODO:
    ====================
    Done - Implement ncpId() check for R510
    Done - Implement System feature flag
    Done - Implement reboot/exit Safe Mode
         - Move ncp_fw_update.cpp to services/ instead of system/ since we have more space available there
    Done - Add correct cipher settings
       ? - Save state_ and status_ variables in retained system memory to ensure we complete step 11/12
         - New system events must be worked into Device Service, add a story for this.
         - Adjust the 40 minute timeout dynamically based on the progress of the INSTALL URCs?
            Done - Gen 3, extends timer 5 minutes for every received URC.
                 - Gen 2, pull in same mechanism from Gen 3 here in NcpFwUpdate::process()
         - When do we retry failures?  Quick loops can retry a few times, but long ones shouldn't
         - Minimize logging output, move some statements to LOG_DEBUG
    Done - Gen 3
            Done - allow normal connection first
            Don't need - disconnect PPP
            Done - enable PDP context with a few AT commands
            Done - try some AT+HTTP commands there
            Done - integrated into NcpFwUpdate
            Don't need - ncpEventHandlerCb

*/


#include "logging.h"

LOG_SOURCE_CATEGORY("system.ncp.update");

#include "ncp_fw_update.h"
#include "system_cloud_connection.h"
#include "cellular_hal.h"
#include "cellular_enums_hal.h"
#include "network/ncp/cellular/cellular_network_manager.h"
#include "network/ncp/cellular/cellular_ncp_client.h"
#include "network/ncp/cellular/ncp.h"
#include "check.h"
#include "delay_hal.h"
#include "system_network.h"
#include "system_cloud_internal.h"

#if HAL_PLATFORM_CELLULAR

namespace particle {

namespace system {

namespace {

/*
#define CHECK_PARSER_URC(_expr) \
        ({ \
            const auto _r = _expr; \
            if (_r < 0) { \
                self->parserError(_r); \
                return _r; \
            } \
            _r; \
        })
*/

HTTPSresponse g_httpsResp = {0};
int g_respCode = 0;
const system_tick_t NCP_FW_MODEM_POWER_ON_TIMEOUT = 60000;
const uint32_t NCP_FW_UBLOX_R510_LATEST_VERSION = 99010001; // 2060001 or 99010001

} // namespace

NcpFwUpdate::NcpFwUpdate() {
    // ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
    ncpFwUpdateState_ = FW_UPDATE_QUALIFY_FLAGS_STATE; // DEBUG: uncomment this one to kick off update on boot
    ncpFwUpdateStatus_ = FW_UPDATE_NONE_STATUS;
    foatReady_ = 0;
    startInstallTimer_ = 0;
    atOkCheckTimer_ = 0;
    g_respCode = -1;
    lastRespCode_ = -1;
    atResponsive_ = -1;
    atResp_ = 0;

    startingFirmwareVersion_ = -1; // Keep these initialized differently to prevent
    firmwareVersion_ = -2;         // code from thinking an update was successful
    updateVersion_ = -3;           //  |

    cooldownTimer_ = 0;
    cooldownTimeout_ = 0;
}

NcpFwUpdate* NcpFwUpdate::instance() {
    static NcpFwUpdate instance;
    return &instance;
}

NcpFwUpdate::~NcpFwUpdate() {
    // clean up?
}

void NcpFwUpdate::checkUpdate() {
    if (ncpFwUpdateStatus_ == FW_UPDATE_NONE_STATUS) {
        ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE;
        cooldownTimer_ = 0;
        cooldownTimeout_ = 0;
    }
}

NcpFWUpdateStatus NcpFwUpdate::process() {
    if (inCooldown_()) {
        updateCooldown_();
    } else {
    switch (ncpFwUpdateState_) {

        case FW_UPDATE_QUALIFY_FLAGS_STATE:
        {
            ncpFwUpdateState_ = FW_UPDATE_QUALIFY_MODEM_ON_STATE;
            // Check NCP version
            if (platform_primary_ncp_identifier() != PLATFORM_NCP_SARA_R510) {
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                break;
            }
            LOG(INFO, "PLATFORM_NCP == SARA_R510");
            // Check system feature is enabled
            if (!HAL_Feature_Get(FEATURE_NCP_FW_UPDATES)) {
                LOG(INFO, "FEATURE_NCP_FW_UPDATES disabled");
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                break;
            }
            LOG(INFO, "FEATURE_NCP_FW_UPDATES enabled");
            // Check if System.updatesEnabled()
            uint8_t updatesEnabled = 0;
            system_get_flag(SYSTEM_FLAG_OTA_UPDATE_ENABLED, &updatesEnabled, nullptr);
            if (!updatesEnabled) {
                LOG(INFO, "System updates disabled");
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                break;
            }
            LOG(INFO, "System updates enabled");
            // Make sure device is on
            if (!network_is_on(NETWORK_INTERFACE_CELLULAR, nullptr)) { // !Cellular.isOn()
                LOG(INFO, "Turning Modem ON...");
                network_on(NETWORK_INTERFACE_CELLULAR, 0, 0, nullptr); // Cellular.on()
            }
            startInstallTimer_ = millis();
        }
        break;

        case FW_UPDATE_QUALIFY_MODEM_ON_STATE:
        {
            if (network_is_on(NETWORK_INTERFACE_CELLULAR, nullptr)) { // Cellular.isOn()
                LOG(INFO, "Modem is on!");
                // Check firmware version requires an upgrade
                firmwareVersion_ = getAppFirmwareVersion_();
                LOG(INFO, "App firmware: %d", firmwareVersion_);
                // if (firmwareVersion_ != 2060001 && firmwareVersion_ != 99010001) {
                if (firmwareVersion_ == NCP_FW_UBLOX_R510_LATEST_VERSION) {
                    LOG(INFO, "Modem has latest firmware version :)");
                    ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                    break;
                }
                LOG(INFO, "Setup Fully Qualified!");
                // If not in Safe Mode, reset now into Safe Mode!
                if (system_mode() != System_Mode_TypeDef::SAFE_MODE) {
                    LOG(INFO, "Resetting into Safe Mode!");
                    delay(200);
                    system_reset(SYSTEM_RESET_MODE_SAFE, 0, 0, SYSTEM_RESET_FLAG_NO_WAIT, nullptr);
                    // Goodbye!  See you next time through in Safe Mode ;-)
                }
                ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECT_STATE;
            } else if (millis() - startInstallTimer_ >= NCP_FW_MODEM_POWER_ON_TIMEOUT) {
                LOG(ERROR, "Modem did not power on!");
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
            }
        }
        break;

        case FW_UPDATE_SETUP_CLOUD_CONNECT_STATE:
        {
            if (!spark_cloud_flag_connected()) { // !Particle.connected()
                LOG(INFO, "Connect to Cloud...");
                spark_cloud_flag_connect(); // Particle.connect()
            }
            ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE;
        }
        break;

        case FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE:
        {
            if (spark_cloud_flag_connected()) { // Particle.connected()
                LOG(INFO, "Connected to Cloud.");
                ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTED_STATE;
            } else {
                spark_cloud_flag_connect(); // Particle.connect()
            }
        }
        break;

        case FW_UPDATE_SETUP_CLOUD_CONNECTED_STATE:
        {
            if (spark_cloud_flag_connected()) { // Particle.connected()
                // Setup HTTPS
                int ret = setupHTTPSProperties_();
                if (!ret) {
                    LOG(INFO, "HTTPS setup error: %d", ret);
                    cooldown_(1000);
                    break;
                }
                if (publishEvent(/*spark/device/*/"ncp/update", "started")) {
                    ncpFwUpdateState_ = FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECT_STATE;
                    LOG(INFO, "Ready to start download...");
                    cooldown_(20000);
                } else {
                    ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE;
                    cooldown_(1000);
                }
            } else {
                ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECTING_STATE;
            }
        }
        break;

        case FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECT_STATE:
        {
            if (spark_cloud_flag_connected()) { // Particle.connected()
                spark_cloud_flag_disconnect(); // Particle.disconnect()
            } else {
                ncpFwUpdateState_ = FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECTED_STATE;
                cellular_at_response_handler_set(ATResponseOut_, nullptr, nullptr);
                cooldown_(20000);
            }
        }
        break;

        case FW_UPDATE_DOWNLOAD_CLOUD_DISCONNECTED_STATE:
        {
            if (network_ready(0, 0, 0)) { // Cellular.ready()
                ncpFwUpdateStatus_ = FW_UPDATE_DOWNLOADING_STATUS;
                bool modemFirmwareDownloadComplete = false;
                foatReady_ = 0;
                memset(&g_httpsResp, 0, sizeof(g_httpsResp));

/*
a2773f2abb80df2886dd29b07f089504  SARA-R510S-00B-00_FW02.05_A00.01_IP_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
48b2d022041ea85899a15351c06a18d2  SARA-R510S-00B-01_FW02.06_A00.01_IP.dof
252ea04a324e9aab8a69678cfe097465  SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
ccfdc48c0a45198d6e168b30d0740959  SARA-R510S-00B-01_FW02.06_A00.01_IP_SARA-R510S-00B-01_FW99.01_A00.01.upd
5fd6c0d3d731c097605895b86f28c2cf  SARA-R510S-00B-01_FW99.01_A00.01_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
*/
                // TODO: Should we test the data connection this way or use the dns_client.cpp, or just not test this at all?
                // cellular_command(nullptr, nullptr, 130000,"AT+UDNSRN=0,\"fw-ftp.staging.particle.io\"\r\n");

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

#ifdef HAL_PLATFORM_NCP
                auto mgr = cellularNetworkManager();
                // CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
                auto client = mgr->ncpClient();
                // CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
                auto parser = client->atParser();
                // CHECK_TRUE(parser, SYSTEM_ERROR_UNKNOWN);
                // +UUHTTPCR: 0,100,1,200,"ccfdc48c0a45198d6e168b30d0740959"
                parser->addUrcHandler("+UUHTTPCR", httpRespCallback_, client);

#endif // HAL_PLATFORM_NCP

                LOG(INFO, "Starting download...");
                if (firmwareVersion_ == 99010001) {
                    updateVersion_ = 2060001;
                    // 99010001 -> 02060001 firmware (takes 1.8 - 2.8 minutes)
                    // Must factory reset after installing
                    cellular_command((_CALLBACKPTR_MDM)cbUUHTTPCR_, (void*)&g_httpsResp, 5*60000, "AT+UHTTPC=0,100,\"/SARA-R510S-00B-01_FW99.01_A00.01_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd\"\r\n");
                    if (g_httpsResp.command == 100 && g_httpsResp.result == 1 && g_httpsResp.status_code == 200 && strcmp(g_httpsResp.md5_sum,"5fd6c0d3d731c097605895b86f28c2cf")) {
                        modemFirmwareDownloadComplete = true;
                    }
                    if (!modemFirmwareDownloadComplete) {
                        system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                        while (HAL_Timer_Get_Milli_Seconds() - start < 300000 && !modemFirmwareDownloadComplete) {
                            cellular_urcs_get(nullptr);
                            if (g_httpsResp.command == 100 && g_httpsResp.result == 1 && g_httpsResp.status_code == 200 && !strcmp(g_httpsResp.md5_sum,"5fd6c0d3d731c097605895b86f28c2cf")) {
                                modemFirmwareDownloadComplete = true;
                            }
                            HAL_Delay_Milliseconds(100);
                            // TODO: Break out on HTTPC error
                        }
                    }
                } else if (firmwareVersion_ == 2060001) {
                    updateVersion_ = 99010001;
                    // 02060001 -> 99010001 firmware (takes 1.8 - 2.8 minutes)
                    // Must factory reset after installing
                    cellular_command((_CALLBACKPTR_MDM)cbUUHTTPCR_, (void*)&g_httpsResp, 5*60000, "AT+UHTTPC=0,100,\"/SARA-R510S-00B-01_FW02.06_A00.01_IP_SARA-R510S-00B-01_FW99.01_A00.01.upd\"\r\n");
                    if (g_httpsResp.command == 100 && g_httpsResp.result == 1 && g_httpsResp.status_code == 200 && !strcmp(g_httpsResp.md5_sum,"ccfdc48c0a45198d6e168b30d0740959")) {
                        modemFirmwareDownloadComplete = true;
                    }
                    if (!modemFirmwareDownloadComplete) {
                        system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                        while (HAL_Timer_Get_Milli_Seconds() - start < 300000 && !modemFirmwareDownloadComplete) {
                            cellular_urcs_get(nullptr);
                            if (g_httpsResp.command == 100 && g_httpsResp.result == 1 && g_httpsResp.status_code == 200 && !strcmp(g_httpsResp.md5_sum,"ccfdc48c0a45198d6e168b30d0740959")) {
                                modemFirmwareDownloadComplete = true;
                            }
                            HAL_Delay_Milliseconds(100);
                            // TODO: Break out on HTTPC error
                        }
                    }
                }

                if (modemFirmwareDownloadComplete) {
                    LOG(INFO, "command: %d, result: %d, status_code: %d, md5:[%s]",
                            g_httpsResp.command, g_httpsResp.result, g_httpsResp.status_code, g_httpsResp.md5_sum);
                    LOG(INFO, "Download complete and verified.");
                    ncpFwUpdateState_ = FW_UPDATE_INSTALL_CELL_DISCONNECTING_STATE;
                } else {
                    LOG(INFO, "Download failed!!");
                    cellular_command((_CALLBACKPTR_MDM)cbUHTTPER_, (void*)&g_httpsResp, 10000, "AT+UHTTPER=0\r\n");
                    LOG(INFO, "UHTTPER class: %d, code: %d",
                    g_httpsResp.err_class, g_httpsResp.err_code);
                    static int download_retries = 0;
                    if (++download_retries < 3) {
                        cooldown_(10000);
                        ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECT_STATE;
                    } else {
                        ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                    }
                }
            } else {
                network_connect(0, 0, 0, 0); // Cellular.connect()
                LOG(INFO, "Connecting Cellular...");
                cooldown_(1000);
            }
        }
        break;

        case FW_UPDATE_INSTALL_CELL_DISCONNECTING_STATE:
        {
            if (network_ready(0, 0, 0)) { // Cellular.ready()
                LOG(INFO, "Disconnecting Cellular...");
                network_disconnect(0, NETWORK_DISCONNECT_REASON_USER, 0);
                cooldown_(20000);
            } else {
                ncpFwUpdateState_ = FW_UPDATE_INSTALL_STATE_STARTING;
            }
        }
        break;

        case FW_UPDATE_INSTALL_STATE_STARTING:
        {
            // TODO: make ncpFwUpdateStatus_ and ncpFwUpdateState_ retained variables and if we start up in 
            //       FW_UPDATE_UPDATING_STATUS, we need to resume waiting and prevent the mdm_hal/sara_ncp_client from
            //       resetting the modem.
            atOkCheckTimer_ = HAL_Timer_Get_Milli_Seconds();
            g_respCode = -1;
            lastRespCode_ = -1;
            LOG(INFO, "Installing firmware, prepare to wait 25 - 32 minutes...");
// DEBUG set to 0 to disable update command
#if 1
            // TODO: Increase this timeout dynamically based on the progress of the INSTALL URCs?
            if (RESP_OK == cellular_command(nullptr, nullptr, 60000, "AT+UFWINSTALL=1,115200\r\n")) {
                ncpFwUpdateStatus_ = FW_UPDATE_UPDATING_STATUS;
                // Wait for AT interface to become unresponsive, since we need to rely on that to break out of our install later
                atResponsive_ = 1;
                while (atResponsive_ && HAL_Timer_Get_Milli_Seconds() - atOkCheckTimer_ < 5 * 60000) {
                    atResponsive_ = (RESP_OK == cellular_command((_CALLBACKPTR_MDM)cbUUFWINSTALL_, (void*)&g_respCode, 3000, "AT\r\n"));
                    if (atResponsive_) {
                        HAL_Delay_Milliseconds(3000);
                    }
                }
                if (atResponsive_) {
                    // We timed out
                    LOG(ERROR, "5 minute timeout waiting for INSTALL to start");
                    ncpFwUpdateStatus_ = FW_UPDATE_FAILED_STATUS;
                    ncpFwUpdateState_ = FW_UPDATE_FINISHED_FACTORY_RESET_STATE;
                } else {
                    cellular_start_ncp_firmware_update(nullptr);
                    ncpFwUpdateState_ = FW_UPDATE_INSTALL_STATE_WAITING;
                }
            } else {
                LOG(ERROR, "AT+UFWINSTALL failed to respond");
                ncpFwUpdateStatus_ = FW_UPDATE_FAILED_STATUS;
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_FACTORY_RESET_STATE;
            }
            cooldown_(10000);
#else
            // DEBUG
            ncpFwUpdateState_ = FW_UPDATE_INSTALL_STATE_WAITING;  // DEBUG
#endif
        }
        break;

        case FW_UPDATE_INSTALL_STATE_WAITING:
        {
            // Wait for the install to take place, 25 - 32 or more minutes!
            startingFirmwareVersion_ = firmwareVersion_;
            startInstallTimer_ = HAL_Timer_Get_Milli_Seconds();
            atOkCheckTimer_ = HAL_Timer_Get_Milli_Seconds();
            g_respCode = -1;
            lastRespCode_ = -1;
            while (HAL_Timer_Get_Milli_Seconds() - startInstallTimer_ < 40 * 60000 && g_respCode != 128) {
                if (HAL_Timer_Get_Milli_Seconds() - atOkCheckTimer_ >= 10000) {
                    atOkCheckTimer_ = HAL_Timer_Get_Milli_Seconds();
                    atResp_ = cellular_command((_CALLBACKPTR_MDM)cbUUFWINSTALL_, (void*)&g_respCode, 1000, "AT\r\n");
                }
                cellular_urcs_get(nullptr);
                if (g_respCode != lastRespCode_) {
                    LOG(INFO, "INSTALL: %d%%", g_respCode);
                    lastRespCode_ = g_respCode;
                    if (g_respCode == 128) {
                        break;
                    }
                }
                if (atResp_ == RESP_OK) {
                    // We appear to be responsive again, if we missed the firmware UUFWINSTALL: 128 URC
                    // See if we can break out of here with verifying an updated or same as before FW revision
                    firmwareVersion_ = getAppFirmwareVersion_();
                    LOG(INFO, "App firmware: %d", firmwareVersion_);
                    if (firmwareVersion_ == updateVersion_) {
                        g_respCode = 128; // force a FW update success status
                    } else if (firmwareVersion_ == startingFirmwareVersion_) {
                        break; // fail early
                    } else {
                        HAL_Delay_Milliseconds(20000); // slow down the crazy log output
                    }
                }
            }

            if (g_respCode == 128) {
                ncpFwUpdateStatus_ = FW_UPDATE_SUCCESS_STATUS;
            }

            if (ncpFwUpdateStatus_ == FW_UPDATE_SUCCESS_STATUS) {
                LOG(INFO, "Firmware update success.");
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_FACTORY_RESET_STATE;
            } else {
                LOG(ERROR, "Firmware update failed!!");
                ncpFwUpdateStatus_ = FW_UPDATE_FAILED_STATUS;
                // Let's factory reset anyway... who know's what odd state we are in.
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_FACTORY_RESET_STATE;
                // TODO: exponential backoff timer and retry??
                // static int install_retries = 0;
                // if (++install_retries < 3) {
                //     cooldown_(30000);
                //     ncpFwUpdateState_ = FW_UPDATE_SETUP_CLOUD_CONNECT_STATE;
                //     // ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                // } else {
                //     ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
                // }
            }
            cellular_at_response_handler_set(nullptr, nullptr, nullptr); // cancel callback
        }
        break;

        case FW_UPDATE_FINISHED_FACTORY_RESET_STATE:
        {
            LOG(INFO, "Factory resetting modem...");
            if (RESP_OK == cellular_command(nullptr, nullptr, 10000, "AT+UFACTORY=2,2\r\n")) {
                LOG(INFO, "Power cycling modem and reconnecting to Cloud...");
                // cellular_off(nullptr);
                network_off(NETWORK_INTERFACE_CELLULAR, 0, 0, nullptr); // Cellular.off()
                startInstallTimer_ = HAL_Timer_Get_Milli_Seconds();
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_POWERING_OFF_STATE;
            }
        }
        break;

        case FW_UPDATE_FINISHED_POWERING_OFF_STATE:
        {
            if (HAL_Timer_Get_Milli_Seconds() - startInstallTimer_ > 60000) {
                LOG(ERROR, "Powering modem off timed out!");
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
            }
            if (network_is_off(NETWORK_INTERFACE_CELLULAR, nullptr)) {
                LOG(INFO, "Powering modem off success");
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_CLOUD_CONNECTING_STATE;
            }
        }
        break;

        case FW_UPDATE_FINISHED_CLOUD_CONNECTING_STATE:
        {
            if (spark_cloud_flag_connected()) { // Particle.connected()
                ncpFwUpdateState_ = FW_UPDATE_FINISHED_CLOUD_CONNECTED_STATE;
            } else {
                spark_cloud_flag_connect(); // Particle.connect()
                cooldown_(10000);
            }
        }
        break;

        case FW_UPDATE_FINISHED_CLOUD_CONNECTED_STATE:
        {
            if (spark_cloud_flag_connected()) { // Particle.connected()
                if (publishEvent(/*spark/device/*/"ncp/update", (ncpFwUpdateStatus_ == FW_UPDATE_SUCCESS_STATUS) ? "success" : "failed")) {
                    ncpFwUpdateState_ = FW_UPDATE_FINISHED_STATE;
                } else {
                    cooldown_(1000);
                }
            } else {
                ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
            }
        }
        break;

        case FW_UPDATE_FINISHED_STATE:
        {
            LOG(INFO, "Firmware update complete.");
            ncpFwUpdateState_ = FW_UPDATE_IDLE_STATE;
        }
        break;

        case FW_UPDATE_IDLE_STATE:
        default:
        {
            if (system_mode() == System_Mode_TypeDef::SAFE_MODE) {
                LOG(INFO, "Resetting out of Safe Mode!");
                delay(200);
                // Reset with graceful disconnect from Cloud
                system_reset(SYSTEM_RESET_MODE_NORMAL, 0, 0, 0, nullptr);
                // FIXME: We want to prevent getting stuck in Safe Mode,
                // but we don't want to get into a constant SMH reset loop as well
            }
        }
        break;
    } // switch end
    } // Not in cooldown

    // XXX: Despite this having a LOCK(), it appears to lockup/block the system thread
    // cellular_urcs_get(nullptr);

    return ncpFwUpdateStatus_;
}

// void NcpFwUpdate::dumpAtCmd_(const char* buf, int len)
// {
//     LOG_PRINTF_C(TRACE, "system.ncp.update", "%3d \"", len);
//     while (len --) {
//         if (*buf == 0) break;
//         char ch = *buf++;
//         if ((ch > 0x1F) && (ch < 0x7F)) { // is printable
//             if      (ch == '%')  LOG_PRINTF_C(TRACE, "system.ncp.update", "%%");
//             else if (ch == '"')  LOG_PRINTF_C(TRACE, "system.ncp.update", "\\\"");
//             else if (ch == '\\') LOG_PRINTF_C(TRACE, "system.ncp.update", "\\\\");
//             else LOG_PRINTF_C(TRACE, "system.ncp.update", "%c", ch);
//         } else {
//             if      (ch == '\a') LOG_PRINTF_C(TRACE, "system.ncp.update", "\\a"); // BEL (0x07)
//             else if (ch == '\b') LOG_PRINTF_C(TRACE, "system.ncp.update", "\\b"); // Backspace (0x08)
//             else if (ch == '\t') LOG_PRINTF_C(TRACE, "system.ncp.update", "\\t"); // Horizontal Tab (0x09)
//             else if (ch == '\n') LOG_PRINTF_C(TRACE, "system.ncp.update", "\\n"); // Linefeed (0x0A)
//             else if (ch == '\v') LOG_PRINTF_C(TRACE, "system.ncp.update", "\\v"); // Vertical Tab (0x0B)
//             else if (ch == '\f') LOG_PRINTF_C(TRACE, "system.ncp.update", "\\f"); // Formfeed (0x0C)
//             else if (ch == '\r') LOG_PRINTF_C(TRACE, "system.ncp.update", "\\r"); // Carriage Return (0x0D)
//             else                 LOG_PRINTF_C(TRACE, "system.ncp.update", "\\x%02x", (unsigned char)ch);
//         }
//     }
//     LOG_PRINTF_C(TRACE, "system.ncp.update", "\"\r\n");
// }

// static
void NcpFwUpdate::ATResponseOut_(void* data, const char* msg) {
    // Serial.print(msg); // AT command response sent back over USB serial
    int a, b, c;
    char s[40];
    // +UUHTTPCR: 0,100,1,200,"ccfdc48c0a45198d6e168b30d0740959"
    if (sscanf(msg, "%*[\r\n]+UUHTTPCR: %*d,%d,%d,%d,\"%32s\"", &a, &b, &c, s) >= 4) {
        g_httpsResp.command = a;
        g_httpsResp.result = b;
        g_httpsResp.status_code = c;
        memcpy(g_httpsResp.md5_sum, &s, sizeof(s));
        // LOG(INFO, "UUHTTPCR matched");
    }

    if (sscanf(msg, "+UUFWINSTALL: %d", &a) >= 1) {
        g_respCode = a;
        LOG(INFO, "UUFWINSTALL matched: %d", a);
    } else if (sscanf(msg, "%*[\r\n]+UUFWINSTALL: %d", &a) >= 1) {
        g_respCode = a;
        LOG(INFO, "UUFWINSTALL matched: %d", a);
    }
}

// static
int NcpFwUpdate::cbUUHTTPCR_(int type, const char* buf, int len, HTTPSresponse* data)
{
    int command = 0;
    int result = 0;
    int status_code = 0;
    char md5_sum[40] = {0};
    // LOG(INFO, "type:%d", type);
    // dumpAtCmd_(buf, 128);
    // const char bf[128] = "\r\n+UUHTTPCR: 0,100,1,200,\"ccfdc48c0a45198d6e168b30d0740959\"";
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        // +UUHTTPCR: 0,100,1,200,"ccfdc48c0a45198d6e168b30d0740959"
        if (sscanf(buf, "%*[\r\n]+UUHTTPCR: %*d,%d,%d,%d,\"%32s\"", &command, &result, &status_code, md5_sum) >= 4) {
            data->command = command;
            data->result = result;
            data->status_code = status_code;
            memcpy(data->md5_sum, &md5_sum, sizeof(md5_sum));
            // LOG(INFO, "UUHTTPCR matched");
        }
    }
    return WAIT;
}

// static
int NcpFwUpdate::cbUHTTPER_(int type, const char* buf, int len, HTTPSresponse* data)
{
    int err_class = 0;
    int err_code = 0;
    // LOG(INFO, "type:%d", type);
    // dumpAtCmd_(buf, 128);
    // const char bf[128] = "\r\n+UHTTPER: 0,10,22";
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        // +UHTTPER: 0,10,22
        if (sscanf(buf, "%*[\r\n]+UHTTPER: %*d,%d,%d", &err_class, &err_code) >= 2) {
            data->err_class = err_class;
            data->err_code = err_code;
            // LOG(INFO, "UHTTPER matched");
        }
    }
    return WAIT;
}

// static
int NcpFwUpdate::cbUUFWINSTALL_(int type, const char* buf, int len, int* data)
{
    int respCode = 0;
    if (data && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        // +UUFWINSTALL: 128
        if (sscanf(buf, "%*[\r\n]+UUFWINSTALL: %d", &respCode) >= 1) {
            *data = respCode;
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
    // const char bf[128] = "\r\n99.01,A00.01\r\n";
    if (val && (type == TYPE_PLUS || type == TYPE_UNKNOWN)) {
        // \r\n99.01,A00.01\r\n
        if (sscanf(buf, "%*[\r\nL0.0.00.00.]%d.%d%*[,A.]%d.%d", &major1, &minor1, &major2, &minor2) == 4) {
            *val = major1 * 1000000 + minor1 * 10000 + major2 * 100 + minor2;
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

int NcpFwUpdate::getAppFirmwareVersion_() {
    // ATI9 (get version and app version)
    // example output
    // "02.05,A00.01" R510 (older)               - v2050001
    // "02.06,A00.01" R510 (newer)               - v2060001
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
#if HAL_PLATFORM_NCP
    cellular_command(nullptr, nullptr, 10000, "AT+CGPADDR=0\r\n");
    // Activate data connection
    int actVal = 0;
    cellular_command((_CALLBACKPTR_MDM)cbUPSND_, (void*)&actVal, 10000, "AT+UPSND=0,8\r\n");
    if (actVal != 1) {
        cellular_command(nullptr, nullptr, 10000, "AT+UPSD=0,100,1\r\n");
        cellular_command(nullptr, nullptr, 10000, "AT+UPSD=0,0,0\r\n");
        cellular_command(nullptr, nullptr, 10000, "AT+UPSDA=0,3\r\n");
    }
#endif

    // Setup security settings
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,0,3\r\n")) return -1; // Highest level (3) root cert checks
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,1,3\r\n")) return -2; // Minimum TLS v1.2
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,2,99,\"C0\",\"2F\"\r\n")) return -3; // Cipher suite
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,3,\"ubx_digicert_global_root_ca\"\r\n")) return -4; // Cert name
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,4,\"fw-ftp.staging.particle.io\"\r\n")) return -5; // Expected server hostname
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+USECPRF=2,10,\"fw-ftp.staging.particle.io\"\r\n")) return -6; // SNI (Server Name Indication)
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,1,\"fw-ftp.staging.particle.io\"\r\n")) return -7;
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,5,443\r\n")) return -8;
    if (RESP_OK != cellular_command(nullptr, nullptr, 10000, "AT+UHTTP=0,6,1,2\r\n")) return -9;
    return 1;
}

#ifdef HAL_PLATFORM_NCP
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
    CHECK_TRUE(r >= 4, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

    g_httpsResp.command = a;
    g_httpsResp.result = b;
    g_httpsResp.status_code = c;
    memcpy(g_httpsResp.md5_sum, &s, sizeof(s));
    LOG_DEBUG(INFO, "UUHTTPCR matched");

    return SYSTEM_ERROR_NONE;
}
#endif // HAL_PLATFORM_NCP

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

} // namespace system

} // namespace particle

#endif // #if HAL_PLATFORM_CELLULAR
