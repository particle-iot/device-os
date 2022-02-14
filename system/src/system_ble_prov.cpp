#include "ble_provisioning_mode_handler.h"
#include "system_ble_prov.h"
#include "core_hal.h"
#include "system_error.h"
#include "logging.h"
#include "system_control_internal.h"
#include "system_listening_mode.h"
#include "ble_hal.h"

#if HAL_PLATFORM_BLE

using namespace particle::system;

int system_ble_prov_mode(bool enabled, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
            LOG(ERROR, "Listening mode is not disabled. Cannot use prov mode");
            return SYSTEM_ERROR_NOT_ALLOWED;
    }
    if (enabled) {
        if (BleProvisioningModeHandler::instance()->getProvModeStatus()) {
            LOG(ERROR, "Provisioning mode already enabled");
            return SYSTEM_ERROR_INVALID_STATE;
        }
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
        if (!BleProvisioningModeHandler::instance()->getProvModeStatus()) {
            LOG(ERROR, "Provisioning mode already disabled");
            return SYSTEM_ERROR_INVALID_STATE;
        }
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
    return BleProvisioningModeHandler::instance()->getProvModeStatus();
}

int system_ble_prov_set_custom_svc_uuid(hal_ble_uuid_t* svcUuid, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    SystemControl::instance()->getBleCtrlRequestChannel()->setProvSvcUuid(svcUuid);
    return SYSTEM_ERROR_NONE;
}

int system_ble_prov_set_custom_tx_uuid(hal_ble_uuid_t* txUuid, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    SystemControl::instance()->getBleCtrlRequestChannel()->setProvTxUuid(txUuid);
    return SYSTEM_ERROR_NONE;
}

int system_ble_prov_set_custom_rx_uuid(hal_ble_uuid_t* rxUuid, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    SystemControl::instance()->getBleCtrlRequestChannel()->setProvRxUuid(rxUuid);
    return SYSTEM_ERROR_NONE;
}

int system_ble_prov_set_custom_ver_uuid(hal_ble_uuid_t* verUuid, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    SystemControl::instance()->getBleCtrlRequestChannel()->setProvVerUuid(verUuid);
    return SYSTEM_ERROR_NONE;
}


#endif // HAL_PLATFORM_BLE
