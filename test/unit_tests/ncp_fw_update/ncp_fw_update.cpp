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

#include <inttypes.h>
#include "ncp_fw_update.h"

#include <catch2/catch.hpp>
#include <hippomocks.h>

#include <string>
#include "system_mode.h"
#ifdef INFO
#undef INFO
#endif
#ifdef WARN
#undef WARN
#endif
#include "system_network.h"
#include "cellular_enums_hal.h"

using namespace particle::services;
using namespace particle;

namespace {
    int system_get_flag(system_flag_t flag, uint8_t* value, void*) {
        return 0;
    }

    bool spark_cloud_flag_connected(void) {
        return false;
    }

    void spark_cloud_flag_connect(void) {
    }

    void spark_cloud_flag_disconnect(void) {
    }

    bool publishEvent(const char* event, const char* data, unsigned flags) {
        return false;
    }

    const SaraNcpFwUpdateConfig saraNcpFwUpdateConfigUpgrade = {
        .size = sizeof(SaraNcpFwUpdateConfig),
        .start_version = 31400010,
        .end_version = 31400011,
        /*filename*/ "SARA-R510S-01B-00-ES-0314A0001_SARA-R510S-01B-00-XX-0314ENG0099A0001.upd",
        /*md5sum*/ "09c1a98d03c761bcbea50355f9b2a50f"
    };

    const SaraNcpFwUpdateConfig saraNcpFwUpdateConfigDowngrade = {
        .size = sizeof(SaraNcpFwUpdateConfig),
        .start_version = 31400011,
        .end_version = 31400010,
        /*filename*/ "SARA-R510S-01B-00-XX-0314ENG0099A0001_SARA-R510S-01B-00-ES-0314A0001.upd",
        /*md5sum*/ "09c1a98d03c761bcbea50355f9b2a50f"
    };
}

class SaraNcpFwUpdateMocks {
public:

    SaraNcpFwUpdateMocks(MockRepository* mocks) :
            mocks_(mocks),
            system_mode_(AUTOMATIC),
            mock_modem_version_(0),
            mock_network_is_on_(false),
            mock_network_is_off_(false),
            mock_platform_ncp_id_(PLATFORM_NCP_SARA_R510),
            mock_spark_cloud_flag_connected_(false),
            mock_spark_cloud_flag_connect_(false) {
        mocks_->OnCallFunc(system_mode).Do([&]() -> System_Mode_TypeDef {
            return system_mode_;
        });
        mocks_->OnCallFunc(cellular_get_ncp_firmware_version).Do([&](uint32_t* version, void* reserved) -> int {
            *version = mock_modem_version_;
            return SYSTEM_ERROR_NONE;
        });
        mocks_->OnCallFunc(network_is_on).Do([&](network_handle_t network, void* reserved) -> bool {
            return mock_network_is_on_;
        });
        mocks_->OnCallFunc(network_is_off).Do([&](network_handle_t network, void* reserved) -> bool {
            return mock_network_is_off_;
        });
        mocks_->OnCallFunc(platform_primary_ncp_identifier).Do([&]() -> PlatformNCPIdentifier {
            return mock_platform_ncp_id_;
        });
        mocks_->OnCallFunc(spark_cloud_flag_connected).Do([&]() -> bool {
            return mock_spark_cloud_flag_connected_;
        });
        mocks_->OnCallFunc(spark_cloud_flag_connect).Do([&]() {
            mock_spark_cloud_flag_connect_ = true;
        });
        mocks_->OnCallFunc(spark_cloud_flag_disconnect).Do([&]() {
            mock_spark_cloud_flag_connect_ = false;
        });
    };

    void setSystemMode(System_Mode_TypeDef mode) {
        system_mode_ = mode;
    }
    void setModemVersion(uint32_t v) {
        mock_modem_version_ = v;
    }
    void setNetworkIsOn(bool state) {
        mock_network_is_on_ = state;
    }
    void setNetworkIsOff(bool state) {
        mock_network_is_off_ = state;
    }
    void setNcpId(PlatformNCPIdentifier ncpid) {
        mock_platform_ncp_id_ = ncpid;
    }
    void setSparkCloudFlagConnected(bool connected) {
        mock_spark_cloud_flag_connected_ = connected;
    }
    bool getSparkCloudFlagConnect() {
        return mock_spark_cloud_flag_connect_;
    }

private:
    MockRepository* mocks_;
    System_Mode_TypeDef system_mode_;
    uint32_t mock_modem_version_;
    bool mock_network_is_on_;
    bool mock_network_is_off_;
    PlatformNCPIdentifier mock_platform_ncp_id_;
    bool mock_spark_cloud_flag_connected_;
    bool mock_spark_cloud_flag_connect_;
};

class SaraNcpFwUpdateTest : public SaraNcpFwUpdate {
public:

    SaraNcpFwUpdateTest() :
            SaraNcpFwUpdate() {
    };

    SaraNcpFwUpdateState getSaraNcpFwUpdateState() const {
        return saraNcpFwUpdateState_;
    }
    void setSaraNcpFwUpdateState(SaraNcpFwUpdateState state) {
        saraNcpFwUpdateState_ = state;
    }
    SaraNcpFwUpdateState getSaraNcpFwUpdateLastState() const {
        return saraNcpFwUpdateLastState_;
    }
    void setSaraNcpFwUpdateLastState(SaraNcpFwUpdateState state) {
        saraNcpFwUpdateLastState_ = state;
    }
    SaraNcpFwUpdateStatus getSaraNcpFwUpdateStatus() const {
        return saraNcpFwUpdateStatus_;
    }
    void setSaraNcpFwUpdateStatus(SaraNcpFwUpdateStatus status) {
        saraNcpFwUpdateStatus_ = status;
    }
    system_error_t getSaraNcpFwUpdateError() const {
        return saraNcpFwUpdateError_;
    }
    void setSaraNcpFwUpdateError(system_error_t error) {
        saraNcpFwUpdateError_ = error;
    }
    SaraNcpFwUpdateStatus getSaraNcpFwUpdateStatusDiagnostics() const {
        return saraNcpFwUpdateStatusDiagnostics_;
    }
    system_error_t getSaraNcpFwUpdateErrorDiagnostics() const {
        return saraNcpFwUpdateErrorDiagnostics_;
    }
    uint32_t getStartingFirmwareVersion() const {
        return startingFirmwareVersion_;
    }
    uint32_t getFirmwareVersion() const {
        return firmwareVersion_;
    }
    uint32_t getUpdateVersion() const {
        return updateVersion_;
    }
    int getUpdateAvailable() const {
        return updateAvailable_;
    }
    void setUpdateAvailable(uint32_t ua) {
        updateAvailable_ = ua;
    }
    int getDownloadRetries() const {
        return downloadRetries_;
    }
    int getFinishedCloudConnectingRetries() const {
        return finishedCloudConnectingRetries_;
    }
    volatile int getCgevDeactProfile() const {
        return cgevDeactProfile_;
    }
    void setCgevDeactProfile(int profile) {
        cgevDeactProfile_ = profile;
    }
    system_tick_t getStartTimer() const {
        return startTimer_;
    }
    system_tick_t getAtOkCheckTimer() const {
        return atOkCheckTimer_;
    }
    system_tick_t getCooldownTimer() const {
        return cooldownTimer_;
    }
    system_tick_t getCooldownTimeout() const {
        return cooldownTimeout_;
    }
    bool callInCooldown() {
        return inCooldown();
    }
    void callUpdateCooldown() {
        return updateCooldown();
    }
    void callCooldown(system_tick_t delay_ms) {
        return cooldown(delay_ms);
    }
    bool getIsUserConfig() const {
        return isUserConfig_;
    }
    bool getInitialized() const {
        return initialized_;
    }

    void setSaraNcpFwUpdateData(SaraNcpFwUpdateData data) {
        memcpy(&saraNcpFwUpdateData_, &data, sizeof(saraNcpFwUpdateData_));
    }
    SaraNcpFwUpdateData getSaraNcpFwUpdateData() {
        return saraNcpFwUpdateData_;
    }
    SaraNcpFwUpdateCallbacks getSaraNcpFwUpdateCallbacks() {
        return saraNcpFwUpdateCallbacks_;
    }
    HTTPSresponse getHttpsResp() {
        return httpsResp_;
    }
    void setHttpsResp(HTTPSresponse data) {
        memcpy(&httpsResp_, &data, sizeof(httpsResp_));
    }

    void callValidateSaraNcpFwUpdateData() {
        validateSaraNcpFwUpdateData();
    }
    void logSaraNcpFwUpdateData(const SaraNcpFwUpdateData& data) {
        printf("saraNcpFwUpdateData size:%u state:%d status:%d error:%d fv:%" PRIu32 " sfv:%" PRIu32 " uv:%" PRIu32 "\r\n",
                data.size,
                data.state,
                data.status,
                data.error,
                data.firmwareVersion,
                data.startingFirmwareVersion,
                data.updateVersion);
        printf("ua:%d iuc:%d sv:%" PRIu32 " ev:%" PRIu32 " file:%s md5:%s\r\n",
                data.updateAvailable,
                data.isUserConfig,
                data.userConfigData.start_version,
                data.userConfigData.end_version,
                data.userConfigData.filename,
                data.userConfigData.md5sum);
    }
    int callSaveSaraNcpFwUpdateData() {
        return saveSaraNcpFwUpdateData();
    }
    int callRecallSaraNcpFwUpdateData() {
        return recallSaraNcpFwUpdateData();
    }
    int callDeleteSaraNcpFwUpdateData() {
        return deleteSaraNcpFwUpdateData();
    }
    int callGetConfigData(SaraNcpFwUpdateConfig& configData) {
        return getConfigData(configData);
    }
    void reset() {
        saraNcpFwUpdateState_ = FW_UPDATE_STATE_IDLE;
        saraNcpFwUpdateLastState_ = saraNcpFwUpdateState_;
        saraNcpFwUpdateStatus_ = FW_UPDATE_STATUS_IDLE;
        saraNcpFwUpdateError_ = SYSTEM_ERROR_NONE;
        saraNcpFwUpdateStatusDiagnostics_ = FW_UPDATE_STATUS_NONE;
        saraNcpFwUpdateErrorDiagnostics_ = SYSTEM_ERROR_NONE;
        startingFirmwareVersion_ = 1;
        firmwareVersion_ = 0;
        updateVersion_ = 2;
        updateAvailable_ = SYSTEM_NCP_FW_UPDATE_UNKNOWN;
        downloadRetries_ = 0;
        finishedCloudConnectingRetries_ = 0;
        cgevDeactProfile_ = 0;
        startTimer_ = 0;
        atOkCheckTimer_ = 0;
        cooldownTimer_ = 0;
        cooldownTimeout_ = 0;
        isUserConfig_ = false;
        initialized_ = false;
    }
    int setupHTTPSProperties() {
        return SYSTEM_ERROR_NONE;
    }


private:
};

TEST_CASE("SaraNcpFwUpdate") {

    SECTION("put fail early REQUIRE's in this section!") {

    MockRepository mocks;
    SaraNcpFwUpdateMocks ncpMocks(&mocks);
    SaraNcpFwUpdateTest ncpTest;
    REQUIRE(ncpTest.callDeleteSaraNcpFwUpdateData() == SYSTEM_ERROR_NONE); // start clean

    SaraNcpFwUpdateCallbacks saraNcpFwUpdateCallbacks = {};
    saraNcpFwUpdateCallbacks.size = sizeof(saraNcpFwUpdateCallbacks);
    saraNcpFwUpdateCallbacks.system_get_flag = system_get_flag;
    saraNcpFwUpdateCallbacks.spark_cloud_flag_connected = spark_cloud_flag_connected;
    saraNcpFwUpdateCallbacks.spark_cloud_flag_connect = spark_cloud_flag_connect;
    saraNcpFwUpdateCallbacks.spark_cloud_flag_disconnect = spark_cloud_flag_disconnect;
    saraNcpFwUpdateCallbacks.publishEvent = publishEvent;

    // Production debugging setttings - with verbose errors
    const char* SARA_NCP_FW_UPDATE_ENABLE_DOWNLOAD_SETTING = SARA_NCP_FW_UPDATE_ENABLE_DOWNLOAD ? "SARA_NCP_FW_UPDATE_ENABLE_DOWNLOAD = 1" : "SARA_NCP_FW_UPDATE_ENABLE_DOWNLOAD = 0";
    const char* SARA_NCP_FW_UPDATE_ENABLE_INSTALL_SETTING = SARA_NCP_FW_UPDATE_ENABLE_INSTALL ? "SARA_NCP_FW_UPDATE_ENABLE_INSTALL = 1" : "SARA_NCP_FW_UPDATE_ENABLE_INSTALL = 0";
    const char* SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING_SETTING = SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING ? "SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING = 1" : "SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING = 0";
    REQUIRE(SARA_NCP_FW_UPDATE_ENABLE_DOWNLOAD_SETTING == "SARA_NCP_FW_UPDATE_ENABLE_DOWNLOAD = 1");
    REQUIRE(SARA_NCP_FW_UPDATE_ENABLE_INSTALL_SETTING == "SARA_NCP_FW_UPDATE_ENABLE_INSTALL = 1");
    REQUIRE(SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING_SETTING == "SARA_NCP_FW_UPDATE_ENABLE_DEBUG_LOGGING = 0");

    SECTION("init") {
        REQUIRE(system_mode() == AUTOMATIC);

        SECTION("init member variables") {
            CHECK(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateLastState() == FW_UPDATE_STATE_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_NONE);
            CHECK(ncpTest.getSaraNcpFwUpdateStatusDiagnostics() == FW_UPDATE_STATUS_NONE);
            CHECK(ncpTest.getSaraNcpFwUpdateErrorDiagnostics() == SYSTEM_ERROR_NONE);
            CHECK(ncpTest.getStartingFirmwareVersion() == 1);
            CHECK(ncpTest.getFirmwareVersion() == 0);
            CHECK(ncpTest.getUpdateVersion() == 2);
            CHECK(ncpTest.getUpdateAvailable() == SYSTEM_NCP_FW_UPDATE_UNKNOWN);
            CHECK(ncpTest.getDownloadRetries() == 0);
            CHECK(ncpTest.getFinishedCloudConnectingRetries() == 0);
            CHECK(ncpTest.getCgevDeactProfile() == 0);
            CHECK(ncpTest.getStartTimer() == 0);
            CHECK(ncpTest.getAtOkCheckTimer() == 0);
            CHECK(ncpTest.getCooldownTimer() == 0);
            CHECK(ncpTest.getCooldownTimeout() == 0);
            CHECK(ncpTest.getIsUserConfig() == false);
            CHECK(ncpTest.getInitialized() == false);

            // initialized_ == false;
            CHECK(ncpTest.enableUpdates() == SYSTEM_ERROR_INVALID_STATE);
            CHECK(ncpTest.updateStatus() == SYSTEM_ERROR_INVALID_STATE);
        }

        SECTION("initialize callbacks") {
            REQUIRE(ncpTest.getInitialized() == false);
            ncpTest.init(saraNcpFwUpdateCallbacks);
            SaraNcpFwUpdateCallbacks testCb = ncpTest.getSaraNcpFwUpdateCallbacks();
            CHECK(testCb.size == sizeof(saraNcpFwUpdateCallbacks));
            CHECK(testCb.system_get_flag == system_get_flag);
            CHECK(testCb.spark_cloud_flag_connected == spark_cloud_flag_connected);
            CHECK(testCb.spark_cloud_flag_connect == spark_cloud_flag_connect);
            CHECK(testCb.spark_cloud_flag_disconnect == spark_cloud_flag_disconnect);
            CHECK(testCb.publishEvent == publishEvent);
            CHECK(ncpTest.getInitialized() == true);
        }

        SECTION("init() system_mode AUTOMATIC - state FW_UPDATE_STATE_IDLE - first boot") {
            // fresh first boot
            REQUIRE(ncpTest.callDeleteSaraNcpFwUpdateData() == SYSTEM_ERROR_NONE);

            REQUIRE(ncpTest.getInitialized() == false);
            ncpTest.init(saraNcpFwUpdateCallbacks);
            CHECK(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_NONE);
            CHECK(ncpTest.getUpdateAvailable() == SYSTEM_NCP_FW_UPDATE_UNKNOWN);
            CHECK(ncpTest.getInitialized() == true);
        }

        SECTION("init() system_mode AUTOMATIC - state FW_UPDATE_STATE_INSTALL_WAITING - resuming update") {
            // Store in cache
            const SaraNcpFwUpdateData saraNcpFwUpdateData = {
                .size = sizeof(SaraNcpFwUpdateData),
                .state = FW_UPDATE_STATE_INSTALL_WAITING,
                .status = FW_UPDATE_STATUS_UPDATING,
                .error = SYSTEM_ERROR_NONE,
                .firmwareVersion = 2,
                .startingFirmwareVersion = 3,
                .updateVersion = 4,
                .updateAvailable = 1,
                .isUserConfig = 0,
                .userConfigData = {}
            };
            ncpTest.setSaraNcpFwUpdateData(saraNcpFwUpdateData);
            REQUIRE(ncpTest.callSaveSaraNcpFwUpdateData() == SYSTEM_ERROR_NONE);

            mocks.ExpectCallFunc(system_reset).With(SYSTEM_RESET_MODE_SAFE, 0, 0, SYSTEM_RESET_FLAG_NO_WAIT, _).Return(0);
            REQUIRE(ncpTest.getInitialized() == false);
            ncpTest.init(saraNcpFwUpdateCallbacks);
            CHECK(ncpTest.getInitialized() == true);
        }

        SECTION("init() system_mode AUTOMATIC - state FW_UPDATE_STATE_FINISHED_IDLE - finished update") {
            // Store in cache
            const SaraNcpFwUpdateData saraNcpFwUpdateData = {
                .size = sizeof(SaraNcpFwUpdateData),
                .state = FW_UPDATE_STATE_FINISHED_IDLE,
                .status = FW_UPDATE_STATUS_SUCCESS,
                .error = SYSTEM_ERROR_NONE,
                .firmwareVersion = 2,
                .startingFirmwareVersion = 3,
                .updateVersion = 4,
                .updateAvailable = 1,
                .isUserConfig = 0,
                .userConfigData = {}
            };
            ncpTest.setSaraNcpFwUpdateData(saraNcpFwUpdateData);
            REQUIRE(ncpTest.callSaveSaraNcpFwUpdateData() == SYSTEM_ERROR_NONE);

            // mocks.ExpectCallFunc(system_reset).With(SYSTEM_RESET_MODE_SAFE, 0, 0, SYSTEM_RESET_FLAG_NO_WAIT, _).Return(0);
            REQUIRE(ncpTest.getInitialized() == false);
            ncpTest.init(saraNcpFwUpdateCallbacks);
            CHECK(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            CHECK(ncpTest.getSaraNcpFwUpdateData().updateAvailable == SYSTEM_NCP_FW_UPDATE_UNKNOWN);
            CHECK(ncpTest.getUpdateAvailable() == SYSTEM_NCP_FW_UPDATE_UNKNOWN);
            CHECK(ncpTest.getSaraNcpFwUpdateStatusDiagnostics() == saraNcpFwUpdateData.status);
            CHECK(ncpTest.getSaraNcpFwUpdateErrorDiagnostics() == saraNcpFwUpdateData.error);
            CHECK(ncpTest.getInitialized() == true);
        }

        SECTION("init() system_mode SAFE_MODE - state FW_UPDATE_STATE_SETUP_CLOUD_CONNECT") {
            ncpMocks.setSystemMode(SAFE_MODE);
            REQUIRE(system_mode() == SAFE_MODE);

            // Store in cache
            const SaraNcpFwUpdateData saraNcpFwUpdateData = {
                .size = sizeof(SaraNcpFwUpdateData),
                .state = FW_UPDATE_STATE_SETUP_CLOUD_CONNECT,
                .status = FW_UPDATE_STATUS_DOWNLOADING, // give it some status other than IDLE
                .error = SYSTEM_ERROR_NONE,
                .firmwareVersion = 2,
                .startingFirmwareVersion = 3,
                .updateVersion = 4,
                .updateAvailable = 1,
                .isUserConfig = 1,
                .userConfigData = {}
            };
            ncpTest.setSaraNcpFwUpdateData(saraNcpFwUpdateData);
            REQUIRE(ncpTest.callSaveSaraNcpFwUpdateData() == SYSTEM_ERROR_NONE);

            REQUIRE(ncpTest.getInitialized() == false);
            ncpTest.init(saraNcpFwUpdateCallbacks);
            CHECK(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_SETUP_CLOUD_CONNECT);
            CHECK(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);
            CHECK(ncpTest.getSaraNcpFwUpdateData().status == saraNcpFwUpdateData.status);
            CHECK(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            CHECK(ncpTest.getSaraNcpFwUpdateData().firmwareVersion == saraNcpFwUpdateData.firmwareVersion);
            CHECK(ncpTest.getSaraNcpFwUpdateData().updateVersion == saraNcpFwUpdateData.updateVersion);
            CHECK(ncpTest.getSaraNcpFwUpdateData().isUserConfig == saraNcpFwUpdateData.isUserConfig);
            CHECK(ncpTest.getSaraNcpFwUpdateData().updateAvailable == saraNcpFwUpdateData.updateAvailable);
            CHECK(ncpTest.getInitialized() == true);
        }

        SECTION("init() system_mode SAFE_MODE - state FW_UPDATE_STATE_FINISHED_IDLE") {
            ncpMocks.setSystemMode(SAFE_MODE);
            REQUIRE(system_mode() == SAFE_MODE);

            // Store in cache
            const SaraNcpFwUpdateData saraNcpFwUpdateData = {
                .size = sizeof(SaraNcpFwUpdateData),
                .state = FW_UPDATE_STATE_FINISHED_IDLE,
                .status = FW_UPDATE_STATUS_IDLE,
                .error = SYSTEM_ERROR_NONE,
                .firmwareVersion = 2,
                .startingFirmwareVersion = 3,
                .updateVersion = 4,
                .updateAvailable = 1,
                .isUserConfig = 1,
                .userConfigData = {}
            };
            ncpTest.setSaraNcpFwUpdateData(saraNcpFwUpdateData);
            REQUIRE(ncpTest.callSaveSaraNcpFwUpdateData() == SYSTEM_ERROR_NONE);

            REQUIRE(ncpTest.getInitialized() == false);
            ncpTest.init(saraNcpFwUpdateCallbacks);
            CHECK(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_IDLE);
            CHECK(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            CHECK(ncpTest.getSaraNcpFwUpdateData().firmwareVersion == saraNcpFwUpdateData.firmwareVersion);
            CHECK(ncpTest.getSaraNcpFwUpdateData().updateVersion == saraNcpFwUpdateData.updateVersion);
            CHECK(ncpTest.getSaraNcpFwUpdateData().isUserConfig == saraNcpFwUpdateData.isUserConfig);
            CHECK(ncpTest.getUpdateAvailable() == SYSTEM_NCP_FW_UPDATE_UNKNOWN);
            CHECK(ncpTest.getSaraNcpFwUpdateData().updateAvailable == SYSTEM_NCP_FW_UPDATE_UNKNOWN);
            CHECK(ncpTest.getInitialized() == true);
        }

        SECTION("setConfig / checkUpdate / enableUpdates") {
            ncpTest.init(saraNcpFwUpdateCallbacks);

            // ncpTest.logSaraNcpFwUpdateData(ncpTest.getSaraNcpFwUpdateData());

            // setConfig - invalid state at checkUpdate due to modem error
            ncpMocks.setModemVersion(0);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_UNKNOWN);
            REQUIRE(ncpTest.setConfig(nullptr) == SYSTEM_ERROR_INVALID_STATE);
            REQUIRE(ncpTest.getIsUserConfig() == false);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().isUserConfig == false);

            // setConfig - disable isUserConfig by sending nullptr - update not available
            ncpMocks.setModemVersion(99999999);
            REQUIRE(ncpTest.setConfig(nullptr) == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_NOT_AVAILABLE);
            REQUIRE(ncpTest.getIsUserConfig() == false);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().isUserConfig == false);
            REQUIRE(ncpTest.getUpdateAvailable() == SYSTEM_NCP_FW_UPDATE_NOT_AVAILABLE);

            // setConfig - enable isUserConfig by sending proper config - update not available
            ncpMocks.setModemVersion(99999999);
            REQUIRE(ncpTest.setConfig(&saraNcpFwUpdateConfigUpgrade) == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_NOT_AVAILABLE);
            REQUIRE(ncpTest.getIsUserConfig() == true);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().isUserConfig == true);
            REQUIRE(ncpTest.getUpdateAvailable() == SYSTEM_NCP_FW_UPDATE_NOT_AVAILABLE);

            // setConfig - enable isUserConfig by sending proper config - update available
            ncpMocks.setModemVersion(31400010);
            REQUIRE(ncpTest.setConfig(&saraNcpFwUpdateConfigUpgrade) == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_PENDING);
            REQUIRE(ncpTest.getIsUserConfig() == true);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().firmwareVersion == ncpTest.getFirmwareVersion());
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().isUserConfig == true);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().updateAvailable == SYSTEM_NCP_FW_UPDATE_PENDING);

            // checkUpdate / enableUpdates / updateStatus
            ncpMocks.setNetworkIsOn(false);
            ncpMocks.setNetworkIsOff(false);
            REQUIRE(ncpTest.enableUpdates() == SYSTEM_ERROR_INVALID_STATE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_PENDING);
            ncpMocks.setNetworkIsOn(true);
            ncpMocks.setNetworkIsOff(true);
            REQUIRE(ncpTest.enableUpdates() == SYSTEM_ERROR_INVALID_STATE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_PENDING);
            ncpMocks.setNetworkIsOn(false);
            ncpMocks.setNetworkIsOff(true);
            REQUIRE(ncpTest.enableUpdates() == SYSTEM_ERROR_INVALID_STATE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_PENDING);
            ncpMocks.setNetworkIsOn(true);
            ncpMocks.setNetworkIsOff(false);
            REQUIRE(ncpTest.enableUpdates() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_IN_PROGRESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_QUALIFY_FLAGS);
            // updateAvailable == SYSTEM_NCP_FW_UPDATE_IN_PROGRESS blocks checkUpdate()
            REQUIRE(ncpTest.checkUpdate() == SYSTEM_ERROR_INVALID_STATE);

            // delete cache - recall cache - not found
            REQUIRE(ncpTest.callDeleteSaraNcpFwUpdateData() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.callRecallSaraNcpFwUpdateData() == SYSTEM_ERROR_NOT_FOUND);

            // Check some reset variables
            REQUIRE(ncpTest.getCooldownTimer() == 0);
            REQUIRE(ncpTest.getCooldownTimeout() == 0);
            REQUIRE(ncpTest.getDownloadRetries() == 0);
            REQUIRE(ncpTest.getFinishedCloudConnectingRetries() == 0);
        }

        SECTION("NCP_ID != PLATFORM_NCP_SARA_R510") {
            ncpMocks.setNcpId(PLATFORM_NCP_SARA_R410);
            CHECK(ncpTest.setConfig(nullptr) == SYSTEM_ERROR_NOT_SUPPORTED);
            CHECK(ncpTest.checkUpdate() == SYSTEM_ERROR_NOT_SUPPORTED);
            CHECK(ncpTest.enableUpdates() == SYSTEM_ERROR_NOT_SUPPORTED);
            CHECK(ncpTest.updateStatus() == SYSTEM_ERROR_NOT_SUPPORTED);
            CHECK(ncpTest.process() == SYSTEM_ERROR_NOT_SUPPORTED);
        }
    } // SECTION INIT

    SECTION("process") {
        REQUIRE(system_mode() == AUTOMATIC);

        SECTION("process() uninitialized / initialized") {
            CHECK(ncpTest.getInitialized() == false);
            CHECK(ncpTest.process() == SYSTEM_ERROR_INVALID_STATE);

            ncpTest.init(saraNcpFwUpdateCallbacks);
            CHECK(ncpTest.getInitialized() == true);
            CHECK(ncpTest.process() == SYSTEM_ERROR_NONE);
        }

        SECTION("cooldown") {
            ncpTest.init(saraNcpFwUpdateCallbacks);
            CHECK(ncpTest.getCooldownTimer() == 0);
            ncpTest.callCooldown(2);
            CHECK(ncpTest.getCooldownTimeout() == 2);
            CHECK(ncpTest.callInCooldown() == true);
            ncpTest.callUpdateCooldown(); // cooldown--;
            CHECK(ncpTest.callInCooldown() == true);
            CHECK(ncpTest.process() == SYSTEM_ERROR_NONE); // cooldown--;
            CHECK(ncpTest.getCooldownTimeout() == 0);
            CHECK(ncpTest.getCooldownTimer() == 0);
            CHECK(ncpTest.callInCooldown() == false);
        }

        SECTION("setConfig upgrade to 31400011") {
            ncpTest.init(saraNcpFwUpdateCallbacks);

            // setConfig - enable isUserConfig by sending proper config - update available
            ncpMocks.setModemVersion(31400010);
            REQUIRE(ncpTest.setConfig(&saraNcpFwUpdateConfigUpgrade) == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_PENDING);
            REQUIRE(ncpTest.getIsUserConfig() == true);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().firmwareVersion == ncpTest.getFirmwareVersion());
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().isUserConfig == true);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().updateAvailable == SYSTEM_NCP_FW_UPDATE_PENDING);

            // checkUpdate / enableUpdates
            ncpMocks.setNetworkIsOn(true);
            ncpMocks.setNetworkIsOff(false);
            REQUIRE(ncpTest.enableUpdates() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.updateStatus() == SYSTEM_NCP_FW_UPDATE_IN_PROGRESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_QUALIFY_FLAGS);

            //============================================
            // FW_UPDATE_STATE_QUALIFY_FLAGS
            //============================================

            // Force a bad updateAvailable_ for FW_UPDATE_STATE_QUALIFY_FLAGS
            ncpTest.setUpdateAvailable(SYSTEM_NCP_FW_UPDATE_PENDING);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_QUALIFY_FLAGS);
            // Restore proper state
            ncpTest.setUpdateAvailable(SYSTEM_NCP_FW_UPDATE_IN_PROGRESS);
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_QUALIFY_FLAGS);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_IDLE);

            mocks.ExpectCallFunc(system_reset).With(SYSTEM_RESET_MODE_SAFE, 0, 0, SYSTEM_RESET_FLAG_NO_WAIT, _).Return(0);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_SETUP_CLOUD_CONNECT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_SETUP_CLOUD_CONNECT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().firmwareVersion == ncpTest.getFirmwareVersion());
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().updateAvailable == SYSTEM_NCP_FW_UPDATE_IN_PROGRESS);
            ncpTest.reset(); // simulate reset of device

            //============================================
            // FW_UPDATE_STATE_SETUP_CLOUD_CONNECT
            //============================================

            ncpMocks.setSystemMode(SAFE_MODE);
            REQUIRE(system_mode() == SAFE_MODE);

            REQUIRE(ncpTest.getInitialized() == false);
            ncpTest.init(saraNcpFwUpdateCallbacks);
            REQUIRE(ncpTest.getInitialized() == true);

            // Run state already connected
            system_tick_t tempTimer = HAL_Timer_Get_Milli_Seconds();
            ncpMocks.setSparkCloudFlagConnected(true);
            mocks.ExpectCallFunc(spark_cloud_flag_connected).Return(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getStartTimer() == tempTimer + 1);
            // Restore proper state
            ncpMocks.setSparkCloudFlagConnected(false);
            mocks.ExpectCallFunc(spark_cloud_flag_connected).Return(false);
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_SETUP_CLOUD_CONNECT);

            REQUIRE(ncpMocks.getSparkCloudFlagConnect() == false);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_SETUP_CLOUD_CONNECTING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().firmwareVersion == ncpTest.getFirmwareVersion());
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().isUserConfig == true);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatusDiagnostics() == FW_UPDATE_STATUS_DOWNLOADING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateErrorDiagnostics() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpMocks.getSparkCloudFlagConnect() == true);
            REQUIRE(ncpTest.getStartTimer() == tempTimer + 2);

            //============================================
            // FW_UPDATE_STATE_SETUP_CLOUD_CONNECTING
            //============================================

            // Connect timeout case
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_SETUP_CLOUD_CONNECTING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + NCP_FW_MODEM_CLOUD_CONNECT_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_CLOUD_CONNECT_ON_ENTRY_TIMEOUT);

            // Restore proper state
            tempTimer = HAL_Timer_Get_Milli_Seconds();
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_SETUP_CLOUD_CONNECT); // back to set startTimer again
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_IDLE);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            // Connect successful case
            ncpMocks.setSparkCloudFlagConnected(true);
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_SETUP_CLOUD_CONNECTING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_CLOUD_CONNECT_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_SETUP_CLOUD_CONNECTED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);

            //============================================
            // FW_UPDATE_STATE_SETUP_CLOUD_CONNECTED
            //============================================

            // Failed to publish case
            mocks.ExpectCallFunc(publishEvent).Return(false);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_PUBLISH_START);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_SETUP_CLOUD_CONNECTED); // back to set startTimer again
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            // Connected false case
            ncpMocks.setSparkCloudFlagConnected(false);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_PUBLISH_START);

            // Restore proper state
            ncpMocks.setSparkCloudFlagConnected(true);
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_SETUP_CLOUD_CONNECTED); // back to set startTimer again
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            // Publish successful case
            mocks.ExpectCallFunc(publishEvent).Return(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_CLOUD_DISCONNECT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);

            //============================================
            // FW_UPDATE_STATE_DOWNLOAD_CLOUD_DISCONNECT
            //============================================

            // Connected successful case
            REQUIRE(ncpMocks.getSparkCloudFlagConnect() == true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpMocks.getSparkCloudFlagConnect() == false);
            ncpMocks.setSparkCloudFlagConnected(false);

            // Network not ready case
            mocks.ExpectCallFunc(network_ready).Return(false);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_CLOUD_DISCONNECT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);

            // Network ready case
            mocks.ExpectCallFunc(network_ready).Return(true);
            tempTimer = HAL_Timer_Get_Milli_Seconds();
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_CELL_DISCONNECTING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);
            REQUIRE(ncpTest.getStartTimer() == tempTimer + 1);
            REQUIRE(ncpTest.getCgevDeactProfile() == 0);
            // TODO: Add test for cellular_add_urc_handler?

            //============================================
            // FW_UPDATE_STATE_DOWNLOAD_CELL_DISCONNECTING
            //============================================

            // Disonnect timeout case
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_CELL_DISCONNECTING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + NCP_FW_MODEM_CLOUD_DISCONNECT_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING);

            // Restore proper state - back one state set startTimer again
            mocks.ExpectCallFunc(network_ready).Return(true);
            tempTimer = HAL_Timer_Get_Milli_Seconds();
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_DOWNLOAD_CLOUD_DISCONNECT);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            // Connect successful case
            ncpTest.setCgevDeactProfile(NCP_FW_UBLOX_DEFAULT_CID);
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_CELL_DISCONNECTING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_CLOUD_DISCONNECT_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);

            //============================================
            // FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING
            //============================================

            // Cellular remains connected
            mocks.ExpectCallFunc(network_ready).Return(true);
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + 1000); // cooldown from previous state
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_SETUP_CELLULAR_STILL_CONNECTED);

            // Restore proper state - back one state set startTimer again
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_DOWNLOAD_CELL_CONNECTING);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            // Cellular disconnected successful case
            mocks.ExpectCallFunc(network_ready).Return(false);
            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).Return(SYSTEM_ERROR_NONE);
            mocks.ExpectCallFunc(network_connect);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_DOWNLOAD_HTTPS_SETUP);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);

            //============================================
            // FW_UPDATE_STATE_DOWNLOAD_HTTPS_SETUP
            //============================================

            // HTTPS Setup fails invalid state
            mocks.ExpectCallFunc(setupHTTPSProperties_impl).Return(SYSTEM_ERROR_INVALID_STATE);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_SETUP_CELLULAR_CONNECT_TIMEOUT);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_DOWNLOAD_HTTPS_SETUP);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            // HTTPS Setup fails https setup error
            mocks.ExpectCallFunc(setupHTTPSProperties_impl).Return(SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_1); // TODO: Convert to specific NCP_FW_ERROR code from system_error.h
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_HTTPS_SETUP_1);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_DOWNLOAD_HTTPS_SETUP);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            // HTTPS Setup succeeds
            mocks.ExpectCallFunc(setupHTTPSProperties_impl).Return(SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_DOWNLOAD_READY);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);

            //============================================
            // FW_UPDATE_STATE_DOWNLOAD_READY
            //============================================

            // UHTTPC RESP_ERROR, download timerout & download retries
            Call &command1 = mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                if (!strcmp(format, "AT+ULSTFILE=0,\"FOAT\"\r\n")) {
                    // printf("ULSTFILE\r\n");
                    *((int*)param) = 1; // file present
                }
                return RESP_OK;
            });
            Call &command2 = mocks.ExpectCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                if (!strcmp(format, "AT+UDELFILE=\"updatePackage.bin\",\"FOAT\"\r\n")) {
                    // printf("UDELFILE\r\n");
                }
                return RESP_OK;
            }).After(command1);
            tempTimer = HAL_Timer_Get_Milli_Seconds();
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getDownloadRetries() == 1);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getDownloadRetries() == 2);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getDownloadRetries() == 3);
            REQUIRE(ncpTest.getUpdateVersion() == saraNcpFwUpdateConfigUpgrade.end_version);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_DOWNLOAD_RETRY_MAX);
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + NCP_FW_MODEM_DOWNLOAD_TIMEOUT * 3);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_DOWNLOAD_READY);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            // UHTTPC error
            mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                if (!strcmp(format, "AT+UHTTPC=0,100,\"/%s\"\r\n")) {
                    // printf("UHTTPC\r\n");
                    HTTPSresponse h = {};
                    h.valid = true;
                    h.command = 100;
                    h.result = 0;
                    h.status_code = 0;
                    memcpy(&h.md5_sum, &saraNcpFwUpdateConfigUpgrade.md5sum, sizeof(saraNcpFwUpdateConfigUpgrade.md5sum));
                    ncpTest.setHttpsResp(h);
                }
                return RESP_OK;
            });
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getDownloadRetries() >= 3);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_DOWNLOAD_RETRY_MAX);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_DOWNLOAD_READY);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            // UHTTPC success
            mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                if (!strcmp(format, "AT+UHTTPC=0,100,\"/%s\"\r\n")) {
                    // printf("UHTTPC\r\n");
                    HTTPSresponse h = {};
                    h.valid = true;
                    h.command = 100;
                    h.result = 1;
                    h.status_code = 200;
                    memcpy(&h.md5_sum, &saraNcpFwUpdateConfigUpgrade.md5sum, sizeof(saraNcpFwUpdateConfigUpgrade.md5sum));
                    ncpTest.setHttpsResp(h);
                }
                return RESP_OK;
            });
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_INSTALL_CELL_DISCONNECTING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);
            REQUIRE(ncpTest.getCgevDeactProfile() == 0);
            tempTimer = HAL_Timer_Get_Milli_Seconds();

            //============================================
            // FW_UPDATE_STATE_INSTALL_CELL_DISCONNECTING
            //============================================

            // Disonnect timeout case
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_INSTALL_CELL_DISCONNECTING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + NCP_FW_MODEM_CLOUD_DISCONNECT_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_INSTALL_STARTING);

            // Restore proper state - back one state set startTimer again
            tempTimer = HAL_Timer_Get_Milli_Seconds();
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_DOWNLOAD_READY);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            // Disconnect successful case
            ncpTest.setCgevDeactProfile(NCP_FW_UBLOX_DEFAULT_CID);
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_INSTALL_CELL_DISCONNECTING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_CLOUD_DISCONNECT_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_INSTALL_STARTING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING);

            //============================================
            // FW_UPDATE_STATE_INSTALL_STARTING
            //============================================

            // UFWINSTALL error
            mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                int ret = RESP_OK;
                if (!strcmp(format, "AT+UFWINSTALL=1,115200\r\n")) {
                    // printf("UFWINSTALL 1\r\n");
                    ret = RESP_ERROR;
                }
                return ret;
            });
            // Wait for cooldown from previous state
            while (ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + 1000); // cooldown from previous state
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWER_OFF);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_INSTALL_AT_ERROR);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_INSTALL_STARTING);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            // UFWINSTALL failure - 5 minute timeout waiting for INSTALL to start
            mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                int ret = RESP_OK;
                if (!strcmp(format, "AT+UFWINSTALL=1,115200\r\n")) {
                    // printf("UFWINSTALL 2\r\n");
                }
                return ret;
            });
            // Wait for 10s cooldown
            while (ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING) {
                tempTimer = HAL_Timer_Get_Milli_Seconds();
                ncpTest.process();
            }
            // REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + NCP_FW_MODEM_INSTALL_START_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWER_OFF);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_START_INSTALL_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_UPDATING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_INSTALL_STARTING);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_DOWNLOADING);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            // UFWINSTALL success
            int responsiveATcount = 10;
            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).With(true, _).Return(SYSTEM_ERROR_NONE);
            mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                int ret = RESP_OK;
                if (!strcmp(format, "AT+UFWINSTALL=1,115200\r\n")) {
                    // printf("UFWINSTALL 3\r\n");
                }
                if (!strcmp(format, "AT\r\n")) {
                    if (responsiveATcount-- > 0) {
                        // printf("AT OK 1\r\n");
                        ret = RESP_OK;
                    } else {
                        // printf("AT TIMEOUT 1\r\n");
                        ret = WAIT; // timeout
                    }
                }
                return ret;
            });
            // Wait for 10s cooldown
            while (ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_DOWNLOADING) {
                tempTimer = HAL_Timer_Get_Milli_Seconds();
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_INSTALL_START_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_INSTALL_WAITING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_UPDATING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_UPDATING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);

            //============================================
            // FW_UPDATE_STATE_INSTALL_WAITING
            //============================================

            // UFWINSTALL timeout & AT polling interval
            system_tick_t atTimer;
            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).With(true, _).Return(SYSTEM_ERROR_NONE);
            mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                int ret = RESP_OK;
                if (!strcmp(format, "AT\r\n")) {
                    // printf("AT\r\n");
                    if (atTimer < tempTimer + (10 * NCP_FW_MODEM_INSTALL_ATOK_INTERVAL)) {
                        // CHECK 10 times
                        REQUIRE(HAL_Timer_Get_Milli_Seconds() >= atTimer + NCP_FW_MODEM_INSTALL_ATOK_INTERVAL);
                    }
                    atTimer = HAL_Timer_Get_Milli_Seconds();
                    ret = WAIT; // timeout
                }
                return ret;
            });
            // wait for previous state 10s cooldown
            while (ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_UPDATING) {
                tempTimer = HAL_Timer_Get_Milli_Seconds(); // start of startTimer_
                atTimer = HAL_Timer_Get_Milli_Seconds(); // start of atOkCheckTimer_
                ncpTest.process();
            }
            REQUIRE(ncpTest.getStartingFirmwareVersion() == saraNcpFwUpdateConfigUpgrade.start_version);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().startingFirmwareVersion == saraNcpFwUpdateConfigUpgrade.start_version);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_INSTALL_WAITING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_UPDATING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWER_OFF);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_INSTALL_TIMEOUT);
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + NCP_FW_MODEM_INSTALL_FINISH_TIMEOUT);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_INSTALL_WAITING);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_UPDATING);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            // UFWINSTALL unresponsive, then responsive after some time with the same firmware version as started with
            const int unresponsiveATcountMax = 120; // represents ~20 minutes
            int unresponsiveATcount = unresponsiveATcountMax;
            tempTimer = HAL_Timer_Get_Milli_Seconds(); // start of startTimer_
            atTimer = HAL_Timer_Get_Milli_Seconds(); // start of atOkCheckTimer_
            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).With(true, _).Return(SYSTEM_ERROR_NONE);
            mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                int ret = RESP_OK;
                if (!strcmp(format, "AT\r\n")) {
                    if (unresponsiveATcount-- > 0) {
                        // printf("AT TIMEOUT 2\r\n");
                        ret = WAIT; // timeout
                    } else {
                        // printf("AT OK 2\r\n");
                        // DO NOT UPGRADE TO LATEST VERSION
                        // ncpMocks.setModemVersion(saraNcpFwUpdateConfigUpgrade.end_version);
                        ret = RESP_OK;
                    }
                    if (unresponsiveATcount > (unresponsiveATcountMax - 10)) {
                        // CHECK 10 times
                        REQUIRE(HAL_Timer_Get_Milli_Seconds() >= atTimer + NCP_FW_MODEM_INSTALL_ATOK_INTERVAL);
                    }
                    atTimer = HAL_Timer_Get_Milli_Seconds();
                }
                return ret;
            });
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getStartingFirmwareVersion() == saraNcpFwUpdateConfigUpgrade.start_version);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().startingFirmwareVersion == saraNcpFwUpdateConfigUpgrade.start_version);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_INSTALL_WAITING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_UPDATING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWER_OFF);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_SAME_VERSION);
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_INSTALL_FINISH_TIMEOUT);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_INSTALL_WAITING);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_UPDATING);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            // UFWINSTALL unresponsive, then responsive after some time with desired updated firmware version
            unresponsiveATcount = unresponsiveATcountMax;
            tempTimer = HAL_Timer_Get_Milli_Seconds(); // start of startTimer_
            atTimer = HAL_Timer_Get_Milli_Seconds(); // start of atOkCheckTimer_
            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).With(true, _).Return(SYSTEM_ERROR_NONE);
            mocks.OnCallFunc(sendCommandWithArgs)
                    .Do([&](_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) -> int {
                int ret = RESP_OK;
                if (!strcmp(format, "AT\r\n")) {
                    if (unresponsiveATcount-- > 0) {
                        // printf("AT TIMEOUT 2\r\n");
                        ret = WAIT; // timeout
                    } else {
                        // printf("AT OK 2\r\n");
                        ncpMocks.setModemVersion(saraNcpFwUpdateConfigUpgrade.end_version);
                        ret = RESP_OK;
                    }
                    if (unresponsiveATcount > (unresponsiveATcountMax - 10)) {
                        // CHECK 10 times
                        REQUIRE(HAL_Timer_Get_Milli_Seconds() >= atTimer + NCP_FW_MODEM_INSTALL_ATOK_INTERVAL);
                    }
                    atTimer = HAL_Timer_Get_Milli_Seconds();
                }
                return ret;
            });
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getStartingFirmwareVersion() == saraNcpFwUpdateConfigUpgrade.start_version);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().startingFirmwareVersion == saraNcpFwUpdateConfigUpgrade.start_version);
            REQUIRE(ncpTest.getFirmwareVersion() == saraNcpFwUpdateConfigUpgrade.end_version);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_INSTALL_WAITING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_UPDATING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWER_OFF);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_INSTALL_FINISH_TIMEOUT);

            //============================================
            // FW_UPDATE_STATE_FINISHED_POWER_OFF
            //============================================

            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).With(false, _).Return(SYSTEM_ERROR_NONE);
            mocks.ExpectCallFunc(network_off).With(NETWORK_INTERFACE_CELLULAR, 0, 0, _);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_FINISHED_POWER_OFF);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWERING_OFF);
            tempTimer = HAL_Timer_Get_Milli_Seconds();

            //============================================
            // FW_UPDATE_STATE_FINISHED_POWERING_OFF
            //============================================

            // Power off timeout case
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWERING_OFF) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() >= tempTimer + NCP_FW_MODEM_POWER_OFF_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_POWER_OFF_TIMEOUT);

            // Restore proper state - back one state set startTimer again
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_FINISHED_POWER_OFF);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_SUCCESS);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).With(false, _).Return(SYSTEM_ERROR_NONE);
            mocks.ExpectCallFunc(network_off).With(NETWORK_INTERFACE_CELLULAR, 0, 0, _);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_FINISHED_POWER_OFF);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWERING_OFF);
            tempTimer = HAL_Timer_Get_Milli_Seconds();
            // Power off success case
            ncpMocks.setNetworkIsOff(true);
            mocks.ExpectCallFunc(network_is_off).With(NETWORK_INTERFACE_CELLULAR, _).Return(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING);
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_POWER_OFF_TIMEOUT);
            tempTimer = HAL_Timer_Get_Milli_Seconds();

            //============================================
            // FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING
            //============================================

            // Particle connect case
            ncpMocks.setSparkCloudFlagConnected(false);
            mocks.ExpectCallFunc(spark_cloud_flag_connect);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatusDiagnostics() == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateErrorDiagnostics() == SYSTEM_ERROR_NONE);

            // Cloud connected succesfully case
            ncpMocks.setSparkCloudFlagConnected(true);
            // wait for previous case 1sec cooldown
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING) {
                ncpTest.process();
            }
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED);

            // Restore proper state - back one states to set startTimer again
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_FINISHED_POWERING_OFF);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_SUCCESS);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            ncpMocks.setNetworkIsOff(true);
            mocks.ExpectCallFunc(network_is_off).With(NETWORK_INTERFACE_CELLULAR, _).Return(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING);
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_POWER_OFF_TIMEOUT);
            tempTimer = HAL_Timer_Get_Milli_Seconds();
            // Try to connect - but timeout
            ncpMocks.setSparkCloudFlagConnected(false);
            mocks.ExpectCallFunc(spark_cloud_flag_connect);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatusDiagnostics() == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateErrorDiagnostics() == SYSTEM_ERROR_NONE);
            // Process multiple cooldowns waiting for timeout
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING) {
                ncpTest.process();
            }
            REQUIRE(HAL_Timer_Get_Milli_Seconds() > tempTimer + NCP_FW_MODEM_CLOUD_CONNECT_TIMEOUT);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_POWER_OFF);
            REQUIRE(ncpTest.getFinishedCloudConnectingRetries() == 1);
            // POWER_OFF -> POWERING_OFF
            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).With(false, _).Return(SYSTEM_ERROR_NONE);
            mocks.ExpectCallFunc(network_off).With(NETWORK_INTERFACE_CELLULAR, 0, 0, _);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            // POWERING_OFF -> CLOUD_CONNECTING
            mocks.ExpectCallFunc(network_is_off).With(NETWORK_INTERFACE_CELLULAR, _).Return(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            // Process multiple cooldowns waiting for timeout
            while (ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING) {
                ncpTest.process();
            }
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_CLOUD_CONNECT_ON_EXIT_TIMEOUT);
            REQUIRE(ncpTest.getFinishedCloudConnectingRetries() == 2);

            // Restore proper state - back two states to set startTimer again
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_FINISHED_POWER_OFF);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_SUCCESS);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            tempTimer = HAL_Timer_Get_Milli_Seconds();
            // POWER_OFF -> POWERING_OFF
            mocks.ExpectCallFunc(cellular_start_ncp_firmware_update).With(false, _).Return(SYSTEM_ERROR_NONE);
            mocks.ExpectCallFunc(network_off).With(NETWORK_INTERFACE_CELLULAR, 0, 0, _);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            // POWERING_OFF -> CLOUD_CONNECTING
            ncpMocks.setNetworkIsOff(true);
            mocks.ExpectCallFunc(network_is_off).With(NETWORK_INTERFACE_CELLULAR, _).Return(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTING);
            // Cloud connected succesfully case
            ncpMocks.setSparkCloudFlagConnected(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED);
            REQUIRE(HAL_Timer_Get_Milli_Seconds() < tempTimer + NCP_FW_MODEM_CLOUD_CONNECT_TIMEOUT);

            //============================================
            // FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED
            //============================================

            // Connected false case
            ncpMocks.setSparkCloudFlagConnected(false);
            mocks.ExpectCallFunc(spark_cloud_flag_connected).Return(false);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_PUBLISH_RESULT);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_SUCCESS);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            ncpMocks.setSparkCloudFlagConnected(true);
            // Failed to publish case
            mocks.ExpectCallFunc(publishEvent).Return(false);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_PUBLISH_RESULT);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_FAILED); // simulated failure
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_SARA_NCP_FW_UPDATE_POWER_OFF_TIMEOUT); // simulated error
            // Publish a failed status case
            mocks.ExpectCallFunc(publishEvent).Return(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_FAILED);
            REQUIRE(ncpTest.getSaraNcpFwUpdateError() == SYSTEM_ERROR_SARA_NCP_FW_UPDATE_POWER_OFF_TIMEOUT);

            // Restore proper state
            ncpTest.setSaraNcpFwUpdateState(FW_UPDATE_STATE_FINISHED_CLOUD_CONNECTED);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_SUCCESS);
            ncpTest.setSaraNcpFwUpdateError(SYSTEM_ERROR_NONE);
            // Publish a failed status case
            mocks.ExpectCallFunc(publishEvent).Return(true);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_SUCCESS);

            //============================================
            // FW_UPDATE_STATE_FINISHED_IDLE
            //============================================

            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateStatus() == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().state == FW_UPDATE_STATE_FINISHED_IDLE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().status == FW_UPDATE_STATUS_SUCCESS);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().error == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().updateAvailable == SYSTEM_NCP_FW_UPDATE_UNKNOWN);
            REQUIRE(ncpTest.getSaraNcpFwUpdateData().isUserConfig == false);

            //============================================
            // FW_UPDATE_STATE_IDLE
            //============================================

            ncpMocks.setSystemMode(SAFE_MODE);
            mocks.ExpectCallFunc(system_reset).With(SYSTEM_RESET_MODE_NORMAL, 0, 0, 0, _).Return(0);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_IDLE);

            ncpMocks.setSystemMode(AUTOMATIC);
            mocks.NeverCallFunc(system_reset).With(SYSTEM_RESET_MODE_NORMAL, 0, 0, 0, _).Return(0);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_IDLE);

            ncpMocks.setSystemMode(SAFE_MODE);
            ncpTest.setSaraNcpFwUpdateStatus(FW_UPDATE_STATUS_IDLE);
            mocks.NeverCallFunc(system_reset).With(SYSTEM_RESET_MODE_NORMAL, 0, 0, 0, _).Return(0);
            REQUIRE(ncpTest.process() == SYSTEM_ERROR_NONE);
            REQUIRE(ncpTest.getSaraNcpFwUpdateState() == FW_UPDATE_STATE_IDLE);
        }
    } // SECTION PROCESS
    } // SECTION("put fail early tests in here!") {
}
