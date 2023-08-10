/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"

SerialLogHandler dbg(LOG_LEVEL_ALL);

namespace {
bool hookExecuted = false;
spark::Vector<ApplicationAsset> hookAssets;
} // anonymous

SYSTEM_THREAD(ENABLED);

PRODUCT_VERSION(2);

void handleAvailableAssets(spark::Vector<ApplicationAsset> assets) {
    // Called before setup
    hookExecuted = true;
    hookAssets = assets;
}

STARTUP(System.onAssetOta(handleAvailableAssets));

void logAssetInfo(ApplicationAsset& asset) {
    LOG(INFO, "Asset name=%s hash=%s size=%u valid=%d readable=%d",
            asset.name().c_str(), asset.hash().toString().c_str(), asset.size(),
            asset.isValid(), asset.isReadable());
}

void dumpAsset(ApplicationAsset& asset) {
    while (true) {
        int avail = asset.available();
        if (avail <= 0) {
            break;
        }
        char tmp[128] = {};
        int r = asset.read(tmp, sizeof(tmp));
        LOG_DUMP(INFO, tmp, r);
        LOG_PRINTF(INFO, "\r\n");
    }
}

void setup() {

}

void loop() {
    if (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'i') {
            LOG(INFO, "Querying required and available assets");
            auto required = System.assetsRequired();
            auto available = System.assetsAvailable();
            LOG(INFO, "required=%u available=%u", required.size(), available.size());
            if (required.size()) {
                LOG(INFO, "Required assets:");
                for (auto& asset: required) {
                    logAssetInfo(asset);
                }
            }
            if (available.size()) {
                LOG(INFO, "Available assets:");
                for (auto& asset: available) {
                    logAssetInfo(asset);
                }
            }
        } else if (c == 'h') {
            LOG(INFO, "Hook executed=%d", hookExecuted);
            if (hookExecuted) {
                LOG(INFO, "%u assets captured in hook:", hookAssets.size());
                for (auto& asset: hookAssets) {
                    logAssetInfo(asset);
                }
            }
        } else if (c == 'd') {
            LOG(INFO, "Dumping available assets");
            auto available = System.assetsAvailable();
            if (available.size()) {
                for (auto& asset: available) {
                    logAssetInfo(asset);
                    dumpAsset(asset);
                }
            }
        } else if (c == 's') {
            LOG(INFO, "Disabling hook execution until new assets OTAd (all available assets handled)");
            System.assetsHandled(true);
        } else if (c == 'S') {
            LOG(INFO, "Enabling hook execution");
            System.assetsHandled(false);
        }
    }
}