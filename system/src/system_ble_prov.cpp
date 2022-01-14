#include "ble_listening_mode_handler.h"
#include "system_ble_prov.h"
#include "core_hal.h"
#include "system_error.h"
#include "logging.h"
#include "system_control_internal.h"
#include "system_listening_mode.h"
#include "ble_hal.h"

#if HAL_PLATFORM_BLE

int system_ble_prov_mode(bool enabled, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
            LOG(ERROR, "Listening mode is not disabled. Cannot use prov mode");
            return SYSTEM_ERROR_NOT_ALLOWED;
    }
    if (enabled) {
        if (particle::system::BleListeningModeHandler::instance()->getProvModeStatus()) {
            LOG(ERROR, "Provisioning mode already enabled");
            return SYSTEM_ERROR_INVALID_STATE;
        }
        // If still in listening mode, exit from listening mode before entering prov mode
        if (particle::system::ListeningModeHandler::instance()->isActive() && HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
            LOG(TRACE, "Listening mode still active. Exiting listening mode before entering prov mode");
            particle::system::ListeningModeHandler::instance()->exit();
            // FIXME: Is delay needed here to check that the module actually exited listening mode
        }
        LOG(TRACE, "Enable BLE prov mode");
        // IMPORTANT: Set setProvModeStatus(true) before entering provisioning mode,
        // as certain operations in BleListeningModeHandler depend on the provMode_ flag
        particle::system::BleListeningModeHandler::instance()->setProvModeStatus(true);
        particle::system::BleControlRequestChannel::instance(particle::system::SystemControl::instance())->init();
        auto r = particle::system::BleListeningModeHandler::instance()->enter();
        if (r) {
            LOG(TRACE, "Unable to enter BLE prov mode");
            particle::system::BleListeningModeHandler::instance()->setProvModeStatus(false);
            return r;
        }
    } else {
        if (!particle::system::BleListeningModeHandler::instance()->getProvModeStatus()) {
            LOG(ERROR, "Provisioning mode already disabled");
            return SYSTEM_ERROR_INVALID_STATE;
        }
        LOG(TRACE, "Disable BLE prov mode");
        // IMPORTANT: Set setProvModeStatus(false) before exiting provsioning mode,
        // as certain operations in BleListeningModeHandler depend on the provMode_ flag
        particle::system::BleListeningModeHandler::instance()->setProvModeStatus(false);
        auto r = particle::system::BleListeningModeHandler::instance()->exit();
        if (r) {
            return r;
        }
    }
    return SYSTEM_ERROR_NONE;
}

bool system_get_ble_prov_status(void* reserved) {
    return particle::system::BleListeningModeHandler::instance()->getProvModeStatus();
}

int system_set_custom_prov_svc_uuid(hal_ble_uuid_t svcUuid, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    particle::system::BleControlRequestChannel::instance(particle::system::SystemControl::instance())->setProvSvcUuid(svcUuid);
    return SYSTEM_ERROR_NONE;
}

int system_set_custom_prov_tx_uuid(hal_ble_uuid_t txUuid, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    particle::system::BleControlRequestChannel::instance(particle::system::SystemControl::instance())->setProvTxUuid(txUuid);
    return SYSTEM_ERROR_NONE;
}

int system_set_custom_prov_rx_uuid(hal_ble_uuid_t rxUuid, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    particle::system::BleControlRequestChannel::instance(particle::system::SystemControl::instance())->setProvRxUuid(rxUuid);
    return SYSTEM_ERROR_NONE;
}

int system_set_custom_prov_ver_uuid(hal_ble_uuid_t verUuid, void* reserved) {
    if (!HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        LOG(TRACE, "Listening mode is not disabled. Cannot use prov mode APIs");
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    particle::system::BleControlRequestChannel::instance(particle::system::SystemControl::instance())->setProvVerUuid(verUuid);
    return SYSTEM_ERROR_NONE;
}


#endif // HAL_PLATFORM_BLE
