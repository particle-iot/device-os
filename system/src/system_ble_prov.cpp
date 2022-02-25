/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include "ble_provisioning_mode_handler.h"
#include "system_ble_prov.h"
#include "core_hal.h"
#include "system_error.h"
#include "logging.h"
#include "system_control_internal.h"
#include "system_listening_mode.h"
#include "ble_hal.h"
#include "check.h"
#include "system_threading.h"

#if HAL_PLATFORM_BLE

using namespace particle::system;

int system_ble_prov_mode(bool enabled, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_ble_prov_mode(enabled, reserved));
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
            LOG(ERROR, "Listening mode is not disabled. Cannot use prov mode");
            return SYSTEM_ERROR_NOT_ALLOWED;
    }
    if (enabled) {
        CHECK_FALSE(BleProvisioningModeHandler::instance()->getProvModeStatus(), SYSTEM_ERROR_NONE);
        // If still in listening mode, exit from listening mode before entering prov mode
        if (ListeningModeHandler::instance()->isActive() && HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
            LOG(TRACE, "Listening mode still active. Exiting listening mode before entering prov mode");
            ListeningModeHandler::instance()->exit();
            // FIXME: Is delay needed here to check that the module actually exited listening mode
        }
        LOG(TRACE, "Enable BLE prov mode");
        // IMPORTANT: Set setProvModeStatus(true) before entering provisioning mode,
        // as certain operations in BleProvisioningModeHandler depend on the provMode_ flag
        BleProvisioningModeHandler::instance()->setProvModeStatus(true);
        SystemControl::instance()->getBleCtrlRequestChannel()->init();
        auto r = BleProvisioningModeHandler::instance()->enter();
        if (r) {
            LOG(TRACE, "Unable to enter BLE prov mode");
            BleProvisioningModeHandler::instance()->setProvModeStatus(false);
            return r;
        }
    } else {
        CHECK_TRUE(BleProvisioningModeHandler::instance()->getProvModeStatus(), SYSTEM_ERROR_NONE);
        LOG(TRACE, "Disable BLE prov mode");
        // IMPORTANT: Set setProvModeStatus(false) before exiting provsioning mode,
        // as certain operations in BleProvisioningModeHandler depend on the provMode_ flag
        BleProvisioningModeHandler::instance()->setProvModeStatus(false);
        auto r = BleProvisioningModeHandler::instance()->exit();
        if (r) {
            return r;
        }
    }
    return SYSTEM_ERROR_NONE;
}

bool system_ble_prov_get_status(void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_ble_prov_get_status(reserved));
    return BleProvisioningModeHandler::instance()->getProvModeStatus();
}

int system_ble_prov_set_custom_svc_uuid(hal_ble_uuid_t* svcUuid, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_ble_prov_set_custom_svc_uuid(svcUuid, reserved));
    if (!svcUuid) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_TRUE(HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE), SYSTEM_ERROR_NOT_ALLOWED);
    return SystemControl::instance()->getBleCtrlRequestChannel()->setProvSvcUuid(svcUuid);
}

int system_ble_prov_set_custom_tx_uuid(hal_ble_uuid_t* txUuid, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_ble_prov_set_custom_tx_uuid(txUuid, reserved));
    if (!txUuid) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_TRUE(HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE), SYSTEM_ERROR_NOT_ALLOWED);
    return SystemControl::instance()->getBleCtrlRequestChannel()->setProvTxUuid(txUuid);
}

int system_ble_prov_set_custom_rx_uuid(hal_ble_uuid_t* rxUuid, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_ble_prov_set_custom_rx_uuid(rxUuid, reserved));
    if (!rxUuid) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_TRUE(HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE), SYSTEM_ERROR_NOT_ALLOWED);
    return SystemControl::instance()->getBleCtrlRequestChannel()->setProvRxUuid(rxUuid);
}

int system_ble_prov_set_custom_ver_uuid(hal_ble_uuid_t* verUuid, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_ble_prov_set_custom_ver_uuid(verUuid, reserved));
    if (!verUuid) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_TRUE(HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE), SYSTEM_ERROR_NOT_ALLOWED);
    return SystemControl::instance()->getBleCtrlRequestChannel()->setProvVerUuid(verUuid);
}

int system_ble_prov_set_company_id(uint16_t companyId, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_ble_prov_set_company_id(companyId, reserved));
    return BleProvisioningModeHandler::instance()->setCompanyId(companyId);
}

#endif // HAL_PLATFORM_BLE
