#include "ble_listening_mode_handler.h"
#include "system_ble_prov.h"
#include "core_hal.h"
#include "system_error.h"
#include "logging.h"
#include "system_control_internal.h"
#include "system_listening_mode.h"

using namespace particle::system;

#if HAL_PLATFORM_BLE

int system_ble_prov_mode(bool enabled, void* reserved) {
    if (enabled) {
        if (BleListeningModeHandler::instance()->getProvModeStatus()) {
            LOG(ERROR, "Provisioning mode already enabled");
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
            LOG(ERROR, "Listening mode is not disabled. Cannot use prov mode");
            return SYSTEM_ERROR_NOT_ALLOWED;
        }
        // If still in listening mode, exit from listening mode before entering prov mode
        if (ListeningModeHandler::instance()->isActive() && HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
            LOG(TRACE, "Listening mode still active. Exiting listening mode before entering prov mode");
            ListeningModeHandler::instance()->exit();
        }
        LOG(TRACE, "Enable BLE prov mode");
        // IMPORTANT: Set setProvModeStatus(true) before entering provisioning mode,
        // as certain operations in BleListeningModeHandler depend on the provMode_ flag
        BleListeningModeHandler::instance()->setProvModeStatus(true);
        if (!BleControlRequestChannel::instance(SystemControl::instance())->getProfInitStatus()) {
            BleControlRequestChannel::instance(SystemControl::instance())->init();
        }
        auto r = BleListeningModeHandler::instance()->enter();
        if (r) {
            return r;
        }
    } else {
        if (!BleListeningModeHandler::instance()->getProvModeStatus()) {
            LOG(ERROR, "Provisioning mode already disabled");
            return SYSTEM_ERROR_INVALID_STATE;
        }
        LOG(TRACE, "Disable BLE prov mode");
        // IMPORTANT: Set setProvModeStatus(false) before exiting provsioning mode,
        // as certain operations in BleListeningModeHandler depend on the provMode_ flag
        BleListeningModeHandler::instance()->setProvModeStatus(false);
        auto r = BleListeningModeHandler::instance()->exit();
        if (r) {
            return r;
        }
    }
    return SYSTEM_ERROR_NONE;
}

bool system_get_ble_prov_status(void* reserved) {
    return BleListeningModeHandler::instance()->getProvModeStatus();
}

int system_set_prov_svc_uuid(const uint8_t* svcUuid, const uint8_t* txUuid, const uint8_t* rxUuid, size_t len, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    if (BleControlRequestChannel::instance(SystemControl::instance())->getProfInitStatus()) {
        LOG(TRACE, "Ble control req channel prov UUIDs already initialized");      // XXX: What about other UUIDs set thru characteristic APIs?
        return SYSTEM_ERROR_INVALID_STATE;
    }
    BleControlRequestChannel::instance(SystemControl::instance())->setProvUuids(svcUuid, txUuid, rxUuid, len);
    return SYSTEM_ERROR_NONE;
}

#endif // HAL_PLATFORM_BLE
