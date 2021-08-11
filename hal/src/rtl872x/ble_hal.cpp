/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"
LOG_SOURCE_CATEGORY("hal.ble");

#include "ble_hal.h"

#if HAL_PLATFORM_BLE

#ifdef __cplusplus
extern "C" {
#endif
#include "rtl8721d.h"
void bt_coex_init(void);
#ifdef __cplusplus
} // extern "C"
#endif

#include <platform_opts_bt.h>
#include <os_sched.h>
#include <string.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <profile_server.h>
#include <gap_msg.h>
#include <simple_ble_service.h>
#include <bas.h>
#include <bte.h>
#include <gap_config.h>
#include <bt_flags.h>
#include <stdio.h>
#include "wifi_constants.h"
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "ftl_int.h"
#include "rtl8721d.h"

//FIXME
#include "app_msg.h"
#include "os_msg.h"
#include "os_task.h"

#include "delay_hal.h"
#include "concurrent_hal.h"
#include "static_recursive_mutex.h"
#include <mutex>
#include "spark_wiring_vector.h"
#include <string.h>
#include <memory>
#include "check.h"
#include "scope_guard.h"

#include "mbedtls/ecdh.h"
#include "mbedtls_util.h"

// FIXME
#undef OFF
#undef ON
#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"

using spark::Vector;
using namespace particle;
using namespace particle::ble;

namespace {

#define DEFAULT_ADVERTISING_INTERVAL_MIN            352 //220ms
#define DEFAULT_ADVERTISING_INTERVAL_MAX            384 //240ms

#define APP_TASK_PRIORITY             9//OS_THREAD_PRIORITY_NETWORK         //!< Task priorities
#define APP_TASK_STACK_SIZE           256 * 4   //!<  Task stack size
#define MAX_NUMBER_OF_GAP_MESSAGE     0x20      //!<  GAP message queue size
#define MAX_NUMBER_OF_IO_MESSAGE      0x20      //!<  IO message queue size
#define MAX_NUMBER_OF_EVENT_MESSAGE   (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE)    //!< Event message queue size

#define FTL_PHY_PAGE_START_ADDR       0x0005D000

StaticRecursiveMutex s_bleMutex;

T_SERVER_ID simp_srv_id; /**< Simple ble service id*/
T_SERVER_ID bas_srv_id;  /**< Battery service id */

void *app_task_handle = NULL;   //!< APP Task handle
void *evt_queue_handle = NULL;  //!< Event queue handle
void *io_queue_handle = NULL;   //!< IO queue handle

const uint8_t ftl_phy_page_num = 3;	/* The number of physical map pages, default is 3: BT_FTL_BKUP_ADDR, BT_FTL_PHY_ADDR1, BT_FTL_PHY_ADDR0 */
const uint32_t ftl_phy_page_start_addr = FTL_PHY_PAGE_START_ADDR;

const uint8_t adv_data[] = {
    /* Flags */
    0x02,             /* length */
    GAP_ADTYPE_FLAGS, /* type="Flags" */
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
    /* Service */
    0x03,             /* length */
    GAP_ADTYPE_16BIT_COMPLETE,
    LO_WORD(GATT_UUID_SIMPLE_PROFILE),
    HI_WORD(GATT_UUID_SIMPLE_PROFILE),
    /* Local name */
    0x0F,             /* length */
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B', 'L', 'E', '_', 'P', 'E', 'R', 'I', 'P', 'H', 'E', 'R', 'A', 'L',
};

const uint8_t scan_rsp_data[] = {
    0x03,                             /* length */
    GAP_ADTYPE_APPEARANCE,            /* type="Appearance" */
    LO_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
    HI_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
};

T_APP_RESULT app_gap_callback(uint8_t cb_type, void *p_cb_data) {
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;

    switch (cb_type)
    {
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        LOG_DEBUG(TRACE, "GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, max_tx_time 0x%x",
                        p_data->p_le_data_len_change_info->conn_id,
                        p_data->p_le_data_len_change_info->max_tx_octets,
                        p_data->p_le_data_len_change_info->max_tx_time);
        break;
#endif
    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        LOG_DEBUG(TRACE, "GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);
        break;

    default:
        LOG_DEBUG(TRACE, "app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}

void app_le_gap_init(void) {
    /* Device name and device appearance */
    uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BLE_PERIPHERAL";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
    uint8_t  slave_init_mtu_req = false;


    /* Advertising parameters */
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint16_t adv_int_min = DEFAULT_ADVERTISING_INTERVAL_MIN;
    uint16_t adv_int_max = DEFAULT_ADVERTISING_INTERVAL_MAX;

    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#if F_BT_LE_SMP_OOB_SUPPORT
    uint8_t  auth_oob = false;
#endif
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    /* Set device name and device appearance */
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
    le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_mtu_req),
                     &slave_init_mtu_req);

    /* Set advertising parameters */
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
    le_adv_set_param(GAP_PARAM_ADV_DATA, sizeof(adv_data), (void *)adv_data);
    le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data), (void *)scan_rsp_data);

    /* Setup the GAP Bond Manager */
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
#endif
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);

    /* register gap message callback */
    le_register_app_cb(app_gap_callback);
}

T_APP_RESULT app_profile_callback(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    if (service_id == SERVICE_PROFILE_GENERAL_ID)
    {
        T_SERVER_APP_CB_DATA *p_param = (T_SERVER_APP_CB_DATA *)p_data;
        switch (p_param->eventId)
        {
        case PROFILE_EVT_SRV_REG_COMPLETE:// srv register result event.
            LOG(TRACE, "PROFILE_EVT_SRV_REG_COMPLETE: result %d",
                            p_param->event_data.service_reg_result);
            break;

        case PROFILE_EVT_SEND_DATA_COMPLETE:
            LOG(TRACE, "PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
                            p_param->event_data.send_data_result.conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);
            if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
            {
                LOG(TRACE, "PROFILE_EVT_SEND_DATA_COMPLETE success");
            }
            else
            {
                LOG(TRACE, "PROFILE_EVT_SEND_DATA_COMPLETE failed");
            }
            break;

        default:
            break;
        }
    }
    else  if (service_id == simp_srv_id)
    {
        TSIMP_CALLBACK_DATA *p_simp_cb_data = (TSIMP_CALLBACK_DATA *)p_data;
        switch (p_simp_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_simp_cb_data->msg_data.notification_indification_index)
                {
                case SIMP_NOTIFY_INDICATE_V3_ENABLE:
                    {
                        LOG(TRACE, "SIMP_NOTIFY_INDICATE_V3_ENABLE");
                    }
                    break;

                case SIMP_NOTIFY_INDICATE_V3_DISABLE:
                    {
                        LOG(TRACE, "SIMP_NOTIFY_INDICATE_V3_DISABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V4_ENABLE:
                    {
                        LOG(TRACE, "SIMP_NOTIFY_INDICATE_V4_ENABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V4_DISABLE:
                    {
                        LOG(TRACE, "SIMP_NOTIFY_INDICATE_V4_DISABLE");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_simp_cb_data->msg_data.read_value_index == SIMP_READ_V1)
                {
                    uint8_t value[2] = {0x01, 0x02};
                    LOG(TRACE, "SIMP_READ_V1");
                    simp_ble_service_set_parameter(SIMPLE_BLE_SERVICE_PARAM_V1_READ_CHAR_VAL, 2, &value);
                }
            }
            break;
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                switch (p_simp_cb_data->msg_data.write.opcode)
                {
                case SIMP_WRITE_V2:
                    {
                        LOG(TRACE, "SIMP_WRITE_V2: write type %d, len %d", p_simp_cb_data->msg_data.write.write_type,
                                        p_simp_cb_data->msg_data.write.len);
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        default:
            break;
        }
    }
    else if (service_id == bas_srv_id)
    {
        T_BAS_CALLBACK_DATA *p_bas_cb_data = (T_BAS_CALLBACK_DATA *)p_data;
        switch (p_bas_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_bas_cb_data->msg_data.notification_indification_index)
                {
                case BAS_NOTIFY_BATTERY_LEVEL_ENABLE:
                    {
                        LOG(TRACE, "BAS_NOTIFY_BATTERY_LEVEL_ENABLE");
                    }
                    break;

                case BAS_NOTIFY_BATTERY_LEVEL_DISABLE:
                    {
                        LOG(TRACE, "BAS_NOTIFY_BATTERY_LEVEL_DISABLE");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_bas_cb_data->msg_data.read_value_index == BAS_READ_BATTERY_LEVEL)
                {
                    uint8_t battery_level = 90;
                    LOG(TRACE, "BAS_READ_BATTERY_LEVEL: battery_level %d", battery_level);
                    bas_set_parameter(BAS_PARAM_BATTERY_LEVEL, 1, &battery_level);
                }
            }
            break;

        default:
            break;
        }
    }

    return app_result;
}

void app_le_profile_init(void) {
    server_init(2);

    simp_srv_id = simp_ble_service_add_service((void *)app_profile_callback);
    bas_srv_id  = bas_add_service((void *)app_profile_callback);
    server_register_app_cb(app_profile_callback);
}

void app_handle_io_msg(T_IO_MSG io_msg) {

}

void app_main_task(void *p_param) {
    (void)p_param;
    uint8_t event;
    os_msg_queue_create(&io_queue_handle, MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&evt_queue_handle, MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(uint8_t));

    gap_start_bt_stack(evt_queue_handle, io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

    LOG_DEBUG(TRACE, "gap_start_bt_stack() done.");

    while (true)
    {
        if (os_msg_recv(evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            LOG_DEBUG(TRACE, "os_msg_recv: %d", event);
            if (event == EVENT_IO_TO_APP)
            {
                T_IO_MSG io_msg;
                if (os_msg_recv(io_queue_handle, &io_msg, 0) == true)
                {
                    app_handle_io_msg(io_msg);
                }
            }
            else
            {
                gap_handle_msg(event);
                LOG_DEBUG(TRACE, "after gap_handle_msg().");
            }
        } else {
            LOG_DEBUG(TRACE, "os_msg_recv: none.");
        }
    }
}

void app_task_init(void) {
    os_task_create(&app_task_handle, "app", app_main_task, 0, APP_TASK_STACK_SIZE, APP_TASK_PRIORITY);
}

} //anonymous namespace

uint32_t ftl_load_from_storage(void *pdata_tmp, uint16_t offset, uint16_t size) {
    LOG_DEBUG(TRACE, "ftl_load_from_storage().");
    return 1;
}

uint32_t ftl_save_to_storage(void *pdata, uint16_t offset, uint16_t size) {
    LOG_DEBUG(TRACE, "ftl_save_to_storage().");
    return 1;
}

/**********************************************
 * Particle BLE APIs
 */
int hal_ble_lock(void* reserved) {
    return !s_bleMutex.lock();
}

int hal_ble_unlock(void* reserved) {
    return !s_bleMutex.unlock();
}

int hal_ble_enter_locked_mode(void* reserved) {
    return SYSTEM_ERROR_NONE;
}

int hal_ble_exit_locked_mode(void* reserved) {
    return SYSTEM_ERROR_NONE;
}

int hal_ble_stack_init(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_stack_init().");

	RCC_PeriphClockCmd(APBPeriph_VENDOR_REG, APBPeriph_VENDOR_REG_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_USI_REG, APBPeriph_USI_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_IRDA_REG, APBPeriph_IRDA_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_IPC, APBPeriph_IPC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_GTIMER, APBPeriph_GTIMER_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_UART1, APBPeriph_UART1_CLOCK, ENABLE);

	/* close BT temp for simulation */
	//RCC_PeriphClockCmd(APBPeriph_BT, APBPeriph_BT_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_WL, APBPeriph_WL_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_GDMA0, APBPeriph_GDMA0_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SECURITY_ENGINE, APBPeriph_SEC_ENG_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_LXBUS, APBPeriph_LXBUS_CLOCK, ENABLE);

    // ftl_init(ftl_phy_page_start_addr, ftl_phy_page_num);
    // LOG_DEBUG(TRACE, "ftl_init() done.");

    T_GAP_DEV_STATE new_state;
	
	const auto mgr = wifiNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
    client->on();
    LOG_DEBUG(TRACE, "wifi is on.");

	//judge BLE central is already on
	le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
		return SYSTEM_ERROR_NONE;
	}
	else {
        gap_config_max_le_link_num(BLE_MAX_LINK_COUNT);
        gap_config_max_le_paired_device(BLE_MAX_LINK_COUNT);
        if (!bte_init()) {
            LOG_DEBUG(ERROR, "bte_init() FAILED!");
        } else {
            LOG_DEBUG(TRACE, "bte_init() SUCCEED!");
        }
        le_gap_init(BLE_MAX_LINK_COUNT);
        app_le_gap_init();
        app_le_profile_init();
        app_task_init();
    }

	bt_coex_init();

    /*Wait BT init complete*/
    LOG_DEBUG(TRACE, "Waiting ready.");
	do {
		HAL_Delay_Milliseconds(100);
		le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	} while(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);
    LOG_DEBUG(TRACE, "BLE stack is ready.");

	/*Start BT WIFI coexistence*/
	wifi_btcoex_set_bt_on();

    LOG_DEBUG(TRACE, "hal_ble_stack_init() done.");

    return SYSTEM_ERROR_NONE;
}

int hal_ble_stack_deinit(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_stack_deinit().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_select_antenna(hal_ble_ant_type_t antenna, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_set_callback_on_adv_events(hal_ble_on_adv_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_set_callback_on_adv_events().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_cancel_callback_on_adv_events(hal_ble_on_adv_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_cancel_callback_on_adv_events().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_set_callback_on_periph_link_events(hal_ble_on_link_evt_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_set_callback_on_periph_link_events().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

/**********************************************
 * BLE GAP APIs
 */
int hal_ble_gap_set_device_address(const hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_device_address().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_device_address(hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_device_address().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_device_name(const char* device_name, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_device_name().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_device_name(char* device_name, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_device_name().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_appearance(ble_sig_appearance_t appearance, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_appearance().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_appearance(ble_sig_appearance_t* appearance, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_appearance().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_ppcp(const hal_ble_conn_params_t* ppcp, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_ppcp().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_ppcp(hal_ble_conn_params_t* ppcp, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_ppcp().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_add_whitelist(const hal_ble_addr_t* addr_list, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_add_whitelist().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_delete_whitelist(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_delete_whitelist().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_tx_power(int8_t tx_power, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_tx_power().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_tx_power(int8_t* tx_power, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_tx_power().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_advertising_parameters(const hal_ble_adv_params_t* adv_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_advertising_parameters().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_advertising_parameters(hal_ble_adv_params_t* adv_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_advertising_parameters().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_advertising_data(const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_advertising_data().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gap_get_advertising_data(uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_advertising_data().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_scan_response_data(const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_scan_response_data().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gap_get_scan_response_data(uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_scan_response_data().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_start_advertising(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_advertising().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_auto_advertise(hal_ble_auto_adv_cfg_t config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_auto_advertise().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_auto_advertise(hal_ble_auto_adv_cfg_t* cfg, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_auto_advertise().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_stop_advertising(void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_stop_advertising().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gap_is_advertising(void* reserved) {
    BleLock lk;
    return false;
}

int hal_ble_gap_set_scan_parameters(const hal_ble_scan_params_t* scan_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_scan_parameters().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_scan_parameters(hal_ble_scan_params_t* scan_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_scan_parameters().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_start_scan(hal_ble_on_scan_result_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_scan().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gap_is_scanning(void* reserved) {
    BleLock lk;
    return false;
}

int hal_ble_gap_stop_scan(void* reserved) {
    // Do not acquire the lock here, otherwise another thread cannot cancel the scanning.
    LOG_DEBUG(TRACE, "hal_ble_gap_stop_scan().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_connect(const hal_ble_conn_cfg_t* config, hal_ble_conn_handle_t* conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_connect().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gap_is_connecting(const hal_ble_addr_t* address, void* reserved) {
    BleLock lk;
    return false;
}

bool hal_ble_gap_is_connected(const hal_ble_addr_t* address, void* reserved) {
    return false;
}

int hal_ble_gap_connect_cancel(const hal_ble_addr_t* address, void* reserved) {
    LOG_DEBUG(TRACE, "hal_ble_gap_connect_cancel().");
    // Do not acquire the lock here, otherwise another thread cannot cancel the connection attempt.
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_disconnect(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_disconnect().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_update_connection_params(hal_ble_conn_handle_t conn_handle, const hal_ble_conn_params_t* conn_params, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_update_connection_params().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_connection_info(hal_ble_conn_handle_t conn_handle, hal_ble_conn_info_t* info, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_connection_info().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_rssi(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_rssi().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_pairing_config(const hal_ble_pairing_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_config().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_get_pairing_config(hal_ble_pairing_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_get_pairing_config().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_start_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_start_pairing().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_reject_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_reject_pairing().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_pairing_auth_data(hal_ble_conn_handle_t conn_handle, const hal_ble_pairing_auth_data_t* auth, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_auth_data().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gap_set_pairing_passkey_deprecated(hal_ble_conn_handle_t conn_handle, const uint8_t* passkey, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_set_pairing_passkey_deprecated().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gap_is_pairing(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_is_pairing().");
    return false;
}

bool hal_ble_gap_is_paired(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gap_is_paired().");
    return false;
}

/**********************************************
 * BLE GATT Server APIs
 */
int hal_ble_gatt_server_add_service(uint8_t type, const hal_ble_uuid_t* uuid, hal_ble_attr_handle_t* handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_service().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_server_add_characteristic(const hal_ble_char_init_t* char_init, hal_ble_char_handles_t* char_handles, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_characteristic().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_server_add_descriptor(const hal_ble_desc_init_t* desc_init, hal_ble_attr_handle_t* handle, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_add_descriptor().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_server_set_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_set_characteristic_value().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_server_notify_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_notify_characteristic_value().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_server_indicate_characteristic_value(hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_indicate_characteristic_value().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_server_get_characteristic_value(hal_ble_attr_handle_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_server_get_characteristic_value().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

/**********************************************
 * BLE GATT Client APIs
 */
int hal_ble_gatt_client_discover_all_services(hal_ble_conn_handle_t conn_handle, hal_ble_on_disc_service_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_all_services().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_discover_service_by_uuid(hal_ble_conn_handle_t conn_handle, const hal_ble_uuid_t* uuid, hal_ble_on_disc_service_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_service_by_uuid().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_discover_characteristics(hal_ble_conn_handle_t conn_handle, const hal_ble_svc_t* service, hal_ble_on_disc_char_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_discover_characteristics().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_discover_characteristics_by_uuid(hal_ble_conn_handle_t conn_handle, const hal_ble_svc_t* service, const hal_ble_uuid_t* uuid, hal_ble_on_disc_char_cb_t callback, void* context, void* reserved) {
    BleLock lk;
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool hal_ble_gatt_client_is_discovering(hal_ble_conn_handle_t conn_handle, void* reserved) {
    BleLock lk;
    return false;
}

int hal_ble_gatt_set_att_mtu(size_t att_mtu, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "ble_gatt_client_set_att_mtu().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_ble_gatt_client_configure_cccd(const hal_ble_cccd_config_t* config, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_configure_cccd().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_client_write_with_response(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_write_with_response().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_client_write_without_response(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, const uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_write_without_response().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

ssize_t hal_ble_gatt_client_read(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t value_handle, uint8_t* buf, size_t len, void* reserved) {
    BleLock lk;
    LOG_DEBUG(TRACE, "hal_ble_gatt_client_read().");
    return SYSTEM_ERROR_NOT_SUPPORTED;
}


#if HAL_PLATFORM_BLE_BETA_COMPAT

int hal_ble_set_callback_on_events_deprecated(hal_ble_on_generic_evt_cb_deprecated_t callback, void* context, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

int hal_ble_gap_connect_deprecated(const hal_ble_addr_t* address, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

int hal_ble_gatt_server_add_characteristic_deprecated(const hal_ble_char_init_deprecated_t* char_init, hal_ble_char_handles_t* char_handles, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

int hal_ble_gatt_client_configure_cccd_deprecated(hal_ble_conn_handle_t conn_handle, hal_ble_attr_handle_t cccd_handle, ble_sig_cccd_value_t cccd_value, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

int hal_ble_gap_get_connection_params_deprecated(hal_ble_conn_handle_t conn_handle, hal_ble_conn_params_t* conn_params, void* reserved) {
    return SYSTEM_ERROR_DEPRECATED;
}

#endif // #if HAL_PLATFORM_BLE_BETA_COMPAT

#endif // HAL_PLATFORM_BLE
