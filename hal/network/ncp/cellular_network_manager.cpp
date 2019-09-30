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

#include "cellular_network_manager.h"

#include "cellular_ncp_client.h"

#include "file_util.h"
#include "scope_guard.h"
#include "check.h"

// FIXME: Move nanopb utilities to a common header file
#include "../../../../system/src/control/common.h"

// FIXME: Build internal protocol files in a separate directory
#include "../../../../system/src/control/proto/internal.pb.h"

#define PB(_name) particle_firmware_##_name
#define PB_CELLULAR(_name) particle_ctrl_cellular_##_name

namespace particle {

namespace {

using namespace control::common;

const auto CONFIG_FILE = "/sys/cellular_config.bin";

struct CellularConfig {
    CellularNetworkConfig intSimConf;
    CellularNetworkConfig extSimConf;
    SimType activeSim;

    CellularConfig() :
            activeSim(SimType::INTERNAL) {
    }
};

int loadConfig(CellularConfig* conf) {
    // Get filesystem instance
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    fs::FsLock lock(fs);
    CHECK(filesystem_mount(fs));
    // Open configuration file
    lfs_file_t file = {};
    CHECK(openFile(&file, CONFIG_FILE, LFS_O_RDONLY));
    NAMED_SCOPE_GUARD(fileGuard, {
        lfs_file_close(&fs->instance, &file);
    });
    const auto fileSize = lfs_file_size(&fs->instance, &file);
    CHECK_TRUE(fileSize >= 0, SYSTEM_ERROR_FILE);
    if (fileSize == 0) {
        *conf = CellularConfig(); // Using default settings
        return 0;
    }
    // Parse configuration
    PB(CellularConfig) pbConf = {};
    DecodedCString dIntApn(&pbConf.internal_sim.apn);
    DecodedCString dIntUser(&pbConf.internal_sim.user);
    DecodedCString dIntPwd(&pbConf.internal_sim.password);
    DecodedCString dExtApn(&pbConf.external_sim.apn);
    DecodedCString dExtUser(&pbConf.external_sim.user);
    DecodedCString dExtPwd(&pbConf.external_sim.password);
    const int r = decodeMessageFromFile(&file, PB(CellularConfig_fields), &pbConf);
    if (r < 0) {
        LOG(ERROR, "Unable to parse network settings");
        *conf = CellularConfig(); // Using default settings
        LOG(WARN, "Removing file: %s", CONFIG_FILE);
        lfs_file_close(&fs->instance, &file);
        fileGuard.dismiss();
        lfs_remove(&fs->instance, CONFIG_FILE);
    } else {
        if (!pbConf.internal_sim.use_defaults) {
            conf->intSimConf.apn(dIntApn.data ? dIntApn.data : "").user(dIntUser.data ? dIntUser.data : "")
                    .password(dIntPwd.data ? dIntPwd.data : "");
        } else {
            conf->intSimConf = CellularNetworkConfig();
        }
        if (!pbConf.external_sim.use_defaults) {
            conf->extSimConf.apn(dExtApn.data ? dExtApn.data : "").user(dExtUser.data ? dExtUser.data : "")
                    .password(dExtPwd.data ? dExtPwd.data : "");
        } else {
            conf->extSimConf = CellularNetworkConfig();
        }
        conf->activeSim = SimType::INTERNAL;
        if (pbConf.active_sim == PB_CELLULAR(SimType_EXTERNAL)) {
            conf->activeSim = SimType::EXTERNAL;
        }
    }
    return 0;
}

int saveConfig(const CellularConfig& conf) {
    // Get filesystem instance
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    fs::FsLock lock(fs);
    CHECK(filesystem_mount(fs));
    // Open configuration file
    lfs_file_t file = {};
    CHECK(openFile(&file, CONFIG_FILE, LFS_O_WRONLY));
    SCOPE_GUARD({
        lfs_file_close(&fs->instance, &file);
    });
    int r = lfs_file_truncate(&fs->instance, &file, 0);
    CHECK_TRUE(r == LFS_ERR_OK, SYSTEM_ERROR_FILE);
    // Serialize configuration
    PB(CellularConfig) pbConf = {};
    EncodedString eIntApn(&pbConf.internal_sim.apn);
    EncodedString eIntUser(&pbConf.internal_sim.user);
    EncodedString eIntPwd(&pbConf.internal_sim.password);
    const auto& intSimConf = conf.intSimConf;
    if (!intSimConf.apn() && !intSimConf.user() && !intSimConf.password()) {
        pbConf.internal_sim.use_defaults = true;
    } else {
        if (intSimConf.apn()) {
            eIntApn.data = intSimConf.apn();
            eIntApn.size = strlen(intSimConf.apn());
        }
        if (intSimConf.user()) {
            eIntUser.data = intSimConf.user();
            eIntUser.size = strlen(intSimConf.user());
        }
        if (intSimConf.password()) {
            eIntPwd.data = intSimConf.password();
            eIntPwd.size = strlen(intSimConf.password());
        }
    }
    EncodedString eExtApn(&pbConf.external_sim.apn);
    EncodedString eExtUser(&pbConf.external_sim.user);
    EncodedString eExtPwd(&pbConf.external_sim.password);
    const auto& extSimConf = conf.extSimConf;
    if (!extSimConf.apn() && !extSimConf.user() && !extSimConf.password()) {
        pbConf.external_sim.use_defaults = true;
    } else {
        if (extSimConf.apn()) {
            eExtApn.data = extSimConf.apn();
            eExtApn.size = strlen(extSimConf.apn());
        }
        if (extSimConf.user()) {
            eExtUser.data = extSimConf.user();
            eExtUser.size = strlen(extSimConf.user());
        }
        if (extSimConf.password()) {
            eExtPwd.data = extSimConf.password();
            eExtPwd.size = strlen(extSimConf.password());
        }
    }
    pbConf.active_sim = (PB_CELLULAR(SimType))conf.activeSim;
    CHECK(encodeMessageToFile(&file, PB(CellularConfig_fields), &pbConf));
    LOG(TRACE, "Updated file: %s", CONFIG_FILE);
    return 0;
}

} // unnamed

CellularNetworkManager::CellularNetworkManager(CellularNcpClient* client) :
        client_(client) {
}

CellularNetworkManager::~CellularNetworkManager() {
}

int CellularNetworkManager::connect() {
    CellularConfig c;
    CHECK(loadConfig(&c));
    const CellularNetworkConfig* conf = &c.intSimConf;
    if (c.activeSim == SimType::EXTERNAL) {
        conf = &c.extSimConf;
    }
    CHECK(client_->connect(*conf));
    return 0;
}

int CellularNetworkManager::setNetworkConfig(SimType sim, CellularNetworkConfig conf) {
    CellularConfig c;
    CHECK(loadConfig(&c));
    switch (sim) {
    case SimType::INTERNAL:
        c.intSimConf = std::move(conf);
        break;
    case SimType::EXTERNAL:
        c.extSimConf = std::move(conf);
        break;
    default:
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK(saveConfig(c));
    return 0;
}

int CellularNetworkManager::getNetworkConfig(SimType sim, CellularNetworkConfig* conf) {
    CellularConfig c;
    CHECK(loadConfig(&c));
    switch (sim) {
    case SimType::INTERNAL:
        *conf = std::move(c.intSimConf);
        break;
    case SimType::EXTERNAL:
        *conf = std::move(c.extSimConf);
        break;
    default:
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return 0;
}

int CellularNetworkManager::clearNetworkConfig(SimType sim) {
    CellularConfig c;
    CHECK(loadConfig(&c));
    switch (sim) {
    case SimType::INTERNAL:
        c.intSimConf = CellularNetworkConfig();
        break;
    case SimType::EXTERNAL:
        c.extSimConf = CellularNetworkConfig();
        break;
    default:
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK(saveConfig(c));
    return 0;
}

int CellularNetworkManager::clearNetworkConfig() {
    CellularConfig c;
    CHECK(loadConfig(&c));
    c.intSimConf = CellularNetworkConfig();
    c.extSimConf = CellularNetworkConfig();
    CHECK(saveConfig(c));
    return 0;
}

int CellularNetworkManager::setActiveSim(SimType sim) {
    CellularConfig c;
    CHECK(loadConfig(&c));
    c.activeSim = sim;
    CHECK(saveConfig(c));
    return 0;
}

int CellularNetworkManager::getActiveSim(SimType* sim) {
    CellularConfig c;
    CHECK(loadConfig(&c));
    *sim = c.activeSim;
    return 0;
}

} // particle
