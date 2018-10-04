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

#include "wifi_manager.h"

#include "filesystem.h"

#include "nanopb_misc.h"
#include "scope_guard.h"
#include "check.h"

#include "spark_wiring_vector.h"

// FIXME: Move nanopb utilities to a common header file
#include "../../../system/src/control/common.h"

// FIXME: Build internal protocol files in a separate directory
#include "../../../system/src/control/proto/internal.pb.h"

#define PB(_name) particle_firmware_##_name
#define PB_WIFI(_name) particle_ctrl_wifi_##_name

namespace particle {

namespace {

using namespace control::common;
using spark::Vector;

int openConfigFile(lfs_t* lfs, lfs_file_t* file) {
    int ret = lfs_mkdir(lfs, "/sys");
    if (ret < 0 && ret != LFS_ERR_EXIST) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    ret = lfs_file_open(lfs, file, "/sys/wifi_config.bin", LFS_O_RDWR | LFS_O_CREAT);
    if (ret < 0) {
        LOG(ERROR, "Unable to open WiFi configuration file");
        return SYSTEM_ERROR_UNKNOWN;
    }
    return 0;
}

// TODO: Implement a couple functions to conveniently save/load a protobuf message to/from a file
int loadConfig(Vector<WifiNetworkConfig>* networks) {
    // Get filesystem instance
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_UNKNOWN);
    fs::FsLock lock(fs);
    CHECK(filesystem_mount(fs));
    // Open configuration file
    lfs_file_t file = {};
    CHECK(openConfigFile(&fs->instance, &file));
    SCOPE_GUARD({
        lfs_file_close(&fs->instance, &file);
    });
    // Initialize nanopb stream
    const auto strm = pb_istream_init(nullptr);
    CHECK_TRUE(strm, SYSTEM_ERROR_NO_MEMORY);
    SCOPE_GUARD({
        pb_istream_free(strm, nullptr);
    });
    CHECK_TRUE(pb_istream_from_file(strm, &file, nullptr), SYSTEM_ERROR_UNKNOWN);
    // Load configuration
    PB(WifiConfig) pbConf = {};
    pbConf.networks.arg = networks;
    pbConf.networks.funcs.decode = [](pb_istream_t* strm, const pb_field_t* field, void** arg) {
        const auto networks = (Vector<WifiNetworkConfig>*)*arg;
        PB(WifiConfig_Network) pbConf = {};
        DecodedCString dSsid(&pbConf.ssid);
        DecodedCString dPwd(&pbConf.credentials.password);
        if (!pb_decode_noinit(strm, PB(WifiConfig_Network_fields), &pbConf)) {
            return false;
        }
        WifiCredentials cred;
        cred.type((WifiCredentials::Type)pbConf.credentials.type);
        if (pbConf.credentials.type == PB_WIFI(CredentialsType_PASSWORD)) {
            cred.password(dPwd.data);
        }
        WifiNetworkConfig conf;
        conf.ssid(dSsid.data);
        conf.security((WifiSecurity)pbConf.security);
        conf.credentials(std::move(cred));
        if (!networks->append(std::move(conf))) {
            return false;
        }
        return true;
    };
    if (!pb_decode(strm, PB(WifiConfig_fields), &pbConf)) {
        LOG(ERROR, "Unable to parse WiFi configuration");
        lfs_file_truncate(&fs->instance, &file, 0);
        networks->clear();
    }
    return 0;
}

int saveConfig(const Vector<WifiNetworkConfig>& networks) {
    // Get filesystem instance
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_UNKNOWN);
    fs::FsLock lock(fs);
    CHECK(filesystem_mount(fs));
    // Open configuration file
    lfs_file_t file = {};
    CHECK(openConfigFile(&fs->instance, &file));
    SCOPE_GUARD({
        lfs_file_close(&fs->instance, &file);
    });
    // Initialize nanopb stream
    const auto strm = pb_ostream_init(nullptr);
    CHECK_TRUE(strm, SYSTEM_ERROR_NO_MEMORY);
    SCOPE_GUARD({
        pb_ostream_free(strm, nullptr);
    });
    CHECK_TRUE(pb_ostream_from_file(strm, &file, nullptr), SYSTEM_ERROR_UNKNOWN);
    // Save configuration
    PB(WifiConfig) pbConf = {};
    pbConf.networks.arg = const_cast<Vector<WifiNetworkConfig>*>(&networks);
    pbConf.networks.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        const auto networks = (const Vector<WifiNetworkConfig>*)*arg;
        for (const WifiNetworkConfig& conf: *networks) {
            PB(WifiConfig_Network) pbConf = {};
            EncodedString eSsid(&pbConf.ssid, conf.ssid(), strlen(conf.ssid()));
            EncodedString ePwd(&pbConf.credentials.password);
            pbConf.security = (PB_WIFI(Security))conf.security();
            pbConf.credentials.type = (PB_WIFI(CredentialsType))conf.credentials().type();
            if (conf.credentials().type() == WifiCredentials::PASSWORD) {
                const auto s = conf.credentials().password();
                ePwd.data = s;
                ePwd.size = strlen(s);
            }
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, PB(WifiConfig_Network_fields), &pbConf)) {
                return false;
            }
        }
        return true;
    };
    if (!pb_encode(strm, PB(WifiConfig_fields), &pbConf)) {
        LOG(ERROR, "Unable to serialize WiFi configuration");
        lfs_file_truncate(&fs->instance, &file, 0);
    }
    return 0;
}

int networkIndexForSsid(const char* ssid, const Vector<WifiNetworkConfig>& networks) {
    for (int i = 0; i < networks.size(); ++i) {
        if (strcmp(ssid, networks.at(i).ssid()) == 0) {
            return i;
        }
    }
    return -1;
}

} // unnamed

WifiManager::WifiManager(WifiNcpClient* ncpClient) :
        ncpClient_(ncpClient) {
}

WifiManager::~WifiManager() {
}

int WifiManager::on() {
    return 0;
}

void WifiManager::off() {
}

WifiManager::AdapterState WifiManager::adapterState() {
    return AdapterState::OFF;
}

int WifiManager::connect(const char* ssid) {
    return 0;
}

int WifiManager::connect() {
    return 0;
}

void WifiManager::disconnect() {
}

WifiManager::ConnectionState WifiManager::connectionState() {
    return ConnectionState::DISCONNECTED;
}

int WifiManager::getNetworkInfo(WifiNetworkInfo* info) {
    return 0;
}

int WifiManager::setNetworkConfig(WifiNetworkConfig conf) {
    CHECK_TRUE(conf.ssid(), SYSTEM_ERROR_INVALID_ARGUMENT);
    Vector<WifiNetworkConfig> networks;
    CHECK(loadConfig(&networks));
    int index = networkIndexForSsid(conf.ssid(), networks);
    if (index < 0) {
        // Add a new network or replace the last network in the list
        if (networks.size() < (int)MAX_CONFIGURED_WIFI_NETWORK_COUNT) {
            CHECK_TRUE(networks.append(WifiNetworkConfig()), SYSTEM_ERROR_NO_MEMORY);
        }
        index = networks.size() - 1;
    }
    networks[index] = std::move(conf);
    CHECK(saveConfig(networks));
    return 0;
}

int WifiManager::getNetworkConfig(const char* ssid, WifiNetworkConfig* conf) {
    // TODO: Cache the list of networks
    CHECK_TRUE(ssid, SYSTEM_ERROR_INVALID_ARGUMENT);
    Vector<WifiNetworkConfig> networks;
    CHECK(loadConfig(&networks));
    const int index = networkIndexForSsid(ssid, networks);
    if (index < 0) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    *conf = std::move(networks[index]);
    return 0;
}

int WifiManager::getConfiguredNetworks(GetConfiguredNetworksCallback callback, void* data) {
    Vector<WifiNetworkConfig> networks;
    CHECK(loadConfig(&networks));
    for (int i = 0; i < networks.size(); ++i) {
        const int ret = callback(std::move(networks[i]), data);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

void WifiManager::removeConfiguredNetwork(const char* ssid) {
    Vector<WifiNetworkConfig> networks;
    if (loadConfig(&networks) < 0) {
        return;
    }
    const int index = networkIndexForSsid(ssid, networks);
    if (index < 0) {
        return;
    }
    networks.removeAt(index);
    saveConfig(networks);
}

void WifiManager::clearConfiguredNetworks() {
    Vector<WifiNetworkConfig> networks;
    saveConfig(networks);
}

int WifiManager::scan(WifiScanCallback callback, void* data) {
    return 0;
}

} // particle
