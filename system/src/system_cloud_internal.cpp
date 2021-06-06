/**
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "logging.h"

#include "spark_wiring_string.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_ticks.h"
#include "spark_wiring_ipaddress.h"
#include "spark_wiring_led.h"
#include "system_cloud_internal.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_threading.h"
#include "system_user.h"
#include "spark_wiring_string.h"
#include "spark_protocol_functions.h"
#include "append_list.h"
#include "core_hal.h"
#include "deviceid_hal.h"
#include "ota_flash_hal.h"
#include "product_store_hal.h"
#include "rtc_hal.h"
#include "socket_hal.h"
#include "rgbled.h"
#include "spark_macros.h"   // for S2M
#include "string_convert.h"
#include "core_hal.h"
#include "hal_platform.h"
#include "bootloader.h"
#include "system_string_interpolate.h"
#include "dtls_session_persist.h"
#include "bytes2hexbuf.h"
#include "system_event.h"
#include "system_cloud_connection.h"
#include "system_network_internal.h"
#include "str_util.h"
#include "scope_guard.h"
#if HAL_PLATFORM_MUXER_MAY_NEED_DELAY_IN_TX
#include "network/ncp/cellular/ncp.h"
#include "network/ncp/cellular/cellular_ncp_client.h"
#endif // HAL_PLATFORM_MUXER_MAY_NEED_DELAY_IN_TX
#include "system_version.h"

#include <stdio.h>
#include <stdint.h>

using namespace particle;
using namespace particle::system;
using particle::protocol::ProtocolError;

namespace particle {

namespace {

constexpr const char CLAIM_EVENTS[] = "spark/device/claim/";
constexpr const char RESET_EVENT[] = "spark/device/reset";
constexpr const char KEY_RESTORE_EVENT[] = "spark/device/key/restore";
constexpr const char DEVICE_UPDATES_EVENT[] = "particle/device/updates/";
constexpr const char FORCED_EVENT[] = "forced";
constexpr const char UPDATES_PENDING_EVENT[] = "pending";

inline bool isSuffix(const char* eventName, const char* prefix, const char* suffix) {
    // todo - sanity check parameters?
    return !strncmp(eventName+strlen(prefix), suffix, strlen(eventName)-strlen(prefix));
}

inline uint8_t dataToFlag(const char* data) {
    return !strncmp(data, "true", strlen(data));
}

/**
 * Handler for system cloud events.
 */
void systemEventHandler(const char* name, const char* data)
{
    if (startsWith(name, DEVICE_UPDATES_EVENT)) {
        const uint8_t flagValue = dataToFlag(data);
        if (isSuffix(name, DEVICE_UPDATES_EVENT, FORCED_EVENT)) {
            system_set_flag(SYSTEM_FLAG_OTA_UPDATE_FORCED, flagValue, nullptr);
        }
        else if (isSuffix(name, DEVICE_UPDATES_EVENT, UPDATES_PENDING_EVENT)) {
            system_set_flag(SYSTEM_FLAG_OTA_UPDATE_PENDING, flagValue, nullptr);
        }
    }
    else if (!strncmp(name, CLAIM_EVENTS, strlen(CLAIM_EVENTS))) {
        LOG(TRACE, "Claim code received by the cloud and cleared locally.");
        HAL_Set_Claim_Code(NULL);
    }
    else if (!strcmp(name, RESET_EVENT)) {
        if (data && *data) {
            if (!strcmp("safe mode", data)) {
                System.enterSafeMode();
            } else if (!strcmp("dfu", data)) {
                System.dfu(false);
            } else if (!strcmp("reboot", data)) {
                System.reset();
            } else if (!strcmp("network", data)) {
                LOG(WARN, "Received a command to reset the network");
                // Reset all active network interfaces asynchronously
                const auto task = new(std::nothrow) ISRTaskQueue::Task();
                if (task) {
                    task->func = [](ISRTaskQueue::Task* task) {
                        delete task;
                        resetNetworkInterfaces();
                    };
                    SystemISRTaskQueue.enqueue(task);
                }
            }
        }
    }
    else if (!strncmp(name, KEY_RESTORE_EVENT, strlen(KEY_RESTORE_EVENT))) {
        // Restore PSK to DCT/DCD/FLASH
        LOG(INFO,"Restoring Public Server Key and Server Address to flash");
#if HAL_PLATFORM_CLOUD_UDP
        bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#else
        bool udp = false;
#endif
        unsigned char psk_buf[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];   // 320 (udp) vs 294 (tcp), allocate 320.
        unsigned char server_addr_buf[EXTERNAL_FLASH_SERVER_ADDRESS_LENGTH];
        memset(&psk_buf, 0xff, sizeof(psk_buf));
        memset(&server_addr_buf, 0xff, sizeof(server_addr_buf));
        if (udp) {
#if HAL_PLATFORM_CLOUD_UDP
            memcpy(&psk_buf, backup_udp_public_server_key, backup_udp_public_server_key_size);
            memcpy(&server_addr_buf, backup_udp_public_server_address, backup_udp_public_server_address_size);
#endif // HAL_PLATFORM_CLOUD_UDP
        } else {
#if HAL_PLATFORM_CLOUD_TCP
            memcpy(&psk_buf, backup_tcp_public_server_key, sizeof(backup_tcp_public_server_key));
            memcpy(&server_addr_buf, backup_tcp_public_server_address, sizeof(backup_tcp_public_server_address));
#endif // HAL_PLATFORM_CLOUD_TCP
        }
        HAL_FLASH_Write_ServerPublicKey(psk_buf, udp);
        HAL_FLASH_Write_ServerAddress(server_addr_buf, udp);
    }
}

} // namespace

int sendApplicationDescription() {
    LOG(INFO, "Sending application DESCRIBE");
    int r = spark_protocol_post_description(sp, protocol::DescriptionType::DESCRIBE_APPLICATION, nullptr);
    if (r != 0) {
        return spark_protocol_to_system_error(r);
    }
    LOG(INFO, "Sending subscriptions");
    spark_protocol_send_subscriptions(sp);
    return 0;
}

void registerSystemSubscriptions() {
    bool ok = Particle.subscribe("particle", systemEventHandler, MY_DEVICES);
    ok = Particle.subscribe("spark", systemEventHandler, MY_DEVICES) && ok;
    if (!ok) {
        LOG(ERROR, "Particle.subscribe() failed");
    }
}

void clearSessionData() {
#if HAL_PLATFORM_CLOUD_UDP
    LOG(INFO, "Clearing session data");
    SessionPersistDataOpaque d = {};
    const int r = Spark_Save(&d, sizeof(d), SparkCallbacks::PERSIST_SESSION, nullptr);
    if (r < 0) {
        LOG(ERROR, "Spark_Save() failed: %d", r);
    }
#endif
}

} // namespace particle

extern uint8_t feature_cloud_udp;
extern volatile bool cloud_socket_aborted;

static volatile uint32_t lastCloudEvent = 0;
uint32_t particle_key_errors = NO_ERROR;

const int CLAIM_CODE_SIZE = 63;

int userVarType(const char *varKey);
int userFuncSchedule(const char *funcKey, const char *paramString, SparkDescriptor::FunctionResultCallback callback, void* reserved);

static int finish_ota_firmware_update(FileTransfer::Descriptor& file, uint32_t flags, void* module);
static void formatResetReasonEventData(int reason, uint32_t data, char *buf, size_t size);

ProtocolFacade* sp;

void (*random_seed_from_cloud_handler)(unsigned int) = nullptr;

/**
 * This is necessary since spark_protocol_instance() was defined in both system_cloud
 * and communication dynalibs. (Not sure why - just an oversight.)
 * Renaming this method, but keeping in the dynalib for backwards compatibility with wiring code
 * version 1. Wiring code compiled against version 2 will not use this function, since the
 * code will be linked to spark_protocol_instance() in comms lib.
 */
ProtocolFacade* system_cloud_protocol_instance(void)
{
	if (!sp)
		sp = spark_protocol_instance();
    return sp;
}

static append_list<User_Var_Lookup_Table_t> vars(5);
static append_list<User_Func_Lookup_Table_t> funcs(5);

User_Var_Lookup_Table_t* find_var_by_key(const char* varKey)
{
    for (int i = vars.size(); i-->0; )
    {
        if (0 == strncmp(vars[i].userVarKey, varKey, USER_VAR_KEY_LENGTH))
        {
            return &vars[i];
        }
    }
    return NULL;
}

template<typename T> T* add_if_sufficient_describe(append_list<T>& list, const char* name, const char* itemType, const T& value) {
	T* result = list.add(value);
	if (result) {
		spark_protocol_describe_data data;
		data.size = sizeof(data);
		data.flags = protocol::DESCRIBE_APPLICATION;
		if (!spark_protocol_get_describe_data(spark_protocol_instance(), &data, nullptr)) {
			if (data.maximum_size<data.current_size) {
				list.removeAt(list.size()-1);
				result = nullptr;
			}
		}
		else {
			INFO("get describe data unsupported");
		}
	}
	if (!result) {
		ERROR("Cannot add %s named %d: insufficient storage", itemType, name);
	}
	return result;
}

User_Var_Lookup_Table_t* find_var_by_key_or_add(const char* varKey, const void* userVar, Spark_Data_TypeDef userVarType, spark_variable_t* extra)
{
	User_Var_Lookup_Table_t item = {};
	item.userVar = userVar;
	item.userVarType = userVarType;
	if (extra) {
		item.update = extra->update;
		if (offsetof(spark_variable_t, copy) + sizeof(spark_variable_t::copy) <= extra->size) {
			item.copy = extra->copy;
		}
	}
	memcpy(item.userVarKey, varKey, USER_VAR_KEY_LENGTH);

    User_Var_Lookup_Table_t* result = find_var_by_key(varKey);

    if (!result) {
    	result = add_if_sufficient_describe(vars, varKey, "variable", item);
    }
    else {
    	*result = item;
    }
    return result;
}

User_Func_Lookup_Table_t* find_func_by_key(const char* funcKey)
{
    for (int i = funcs.size(); i-->0; )
    {
        if (0 == strncmp(funcs[i].userFuncKey, funcKey, USER_FUNC_KEY_LENGTH))
        {
            return &funcs[i];
        }
    }
    return NULL;
}

User_Func_Lookup_Table_t* find_func_by_key_or_add(const char* funcKey, const cloud_function_descriptor* desc)
{
	User_Func_Lookup_Table_t item = {0};
	item.pUserFunc = desc->fn;
	item.pUserFuncData = desc->data;
    memcpy(item.userFuncKey, desc->funcKey, USER_FUNC_KEY_LENGTH);

    User_Func_Lookup_Table_t* result = find_func_by_key(funcKey);
    if (result) {
    	*result = item;
    }
    else {
    	result = add_if_sufficient_describe(funcs, funcKey, "function", item);
    }
    return result;
}

int call_raw_user_function(void* data, const char* param, void* reserved)
{
    user_function_int_str_t* fn = (user_function_int_str_t*)(data);
    String p(param);
    return (*fn)(p);
}

inline uint32_t crc(const void* data, size_t len)
{
	return HAL_Core_Compute_CRC32((const uint8_t*)data, len);
}

template <typename T>
uint32_t crc(const T& t)
{
	return crc(&t, sizeof(t));
}

uint32_t string_crc(const char* s)
{
	return crc(s, strlen(s));
}

/**
 * Computes the checksum of the registered functions.
 * The function name is used to compute the checksum.
 */
uint32_t compute_functions_checksum()
{
	uint32_t checksum = 0;
	for (int i = funcs.size(); i-->0; )
    {
		checksum += string_crc(funcs[i].userFuncKey);
    }
	return checksum;
}

/**
 * Computes the checksum of the registered variables.
 * The checksum is derived from the variable name and type.
 */
uint32_t compute_variables_checksum()
{
	uint32_t checksum = 0;
	for (int i = vars.size(); i-->0; )
	{
		checksum += string_crc(vars[i].userVarKey);
		checksum += crc(vars[i].userVarType);
	}
	return checksum;
}

/**
 * Computes the checksum of all functions and variables.
 */
uint32_t compute_describe_app_checksum()
{
	uint32_t chk[2];
	chk[0] = compute_variables_checksum();
	chk[1] = compute_functions_checksum();
	return crc(chk, sizeof(chk));
}

uint32_t compute_describe_system_checksum()
{
    hal_system_info_t info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    HAL_System_Info(&info, true, NULL);
	uint32_t checksum = info.platform_id;
	for (int i=0; i<info.module_count; i++)
	{
		checksum += crc(info.modules[i].suffix.sha);
	}
	HAL_System_Info(&info, false, NULL);
    return checksum;
}


/**
 * Register a function.
 * @param desc
 * @param reserved
 * @return
 */
bool spark_function_internal(const cloud_function_descriptor* desc, void* reserved)
{
    User_Func_Lookup_Table_t* item = NULL;
    if (NULL != desc->fn && NULL != desc->funcKey && strlen(desc->funcKey)<=USER_FUNC_KEY_LENGTH)
    {
        item=find_func_by_key_or_add(desc->funcKey, desc);
    }
    return item!=NULL;
}


void invokeEventHandlerInternal(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo,
                const char* event_name, const char* data, void* reserved)
{
    if(handlerInfo->handler_data)
    {
        EventHandlerWithData handler = (EventHandlerWithData) handlerInfo->handler;
        handler(handlerInfo->handler_data, event_name, data);
    }
    else
    {
        handlerInfo->handler(event_name, data);
    }
}

void invokeEventHandlerString(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo,
                const String& name, const String& data, void* reserved)
{
    invokeEventHandlerInternal(handlerInfoSize, handlerInfo, name.c_str(), data.c_str(), reserved);
}

bool is_system_handler(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo) {
	// for now we hack this to recognize our own system handler
    return handlerInfo->handler == systemEventHandler;
}

void invokeEventHandler(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo,
                const char* event_name, const char* event_data, void* reserved)
{
    if (is_system_handler(handlerInfoSize, handlerInfo) || system_thread_get_state(NULL)==spark::feature::DISABLED)
    {
        invokeEventHandlerInternal(handlerInfoSize, handlerInfo, event_name, event_data, reserved);
    }
    else
    {
        // copy the buffers to dynamically allocated storage.
        String name(event_name);
        String data(event_data);
        APPLICATION_THREAD_CONTEXT_ASYNC(invokeEventHandlerString(handlerInfoSize, handlerInfo, name, data, reserved));
    }
}

int numUserFunctions(void)
{
    return funcs.size();
}

const char* getUserFunctionKey(int function_index)
{
    return funcs[function_index].userFuncKey;
}

int numUserVariables(void)
{
    return vars.size();
}

const char* getUserVariableKey(int variable_index)
{
    return vars[variable_index].userVarKey;
}

SparkReturnType::Enum wrapVarTypeInEnum(const char *varKey)
{
    switch (userVarType(varKey))
    {
        case 1:
            return SparkReturnType::BOOLEAN;
        case 4:
            return SparkReturnType::STRING;
        case 9:
            return SparkReturnType::DOUBLE;
        case 2:
        default:
            return SparkReturnType::INT;
    }
}

#if HAL_PLATFORM_CLOUD_UDP

using particle::protocol::SessionPersistOpaque;
using particle::protocol::SessionPersistData;
using particle::protocol::AppStateDescriptor;

int Spark_Save(const void* buffer, size_t length, uint8_t type, void* reserved)
{
	if (type==SparkCallbacks::PERSIST_SESSION)
	{
		static_assert(sizeof(SessionPersistOpaque::connection)>=sizeof(g_system_cloud_session_data),"connection space in session is not large enough");

		// save the current connection to the persisted session
		SessionPersistOpaque* persist = (SessionPersistOpaque*)buffer;
		if (persist->is_valid())
		{
			memcpy(persist->connection_data(), &g_system_cloud_session_data, sizeof(g_system_cloud_session_data));
		}
		return HAL_System_Backup_Save(0, buffer, length, nullptr);
	}
	return -1;	// eek. define a constant for this error - Unknown Type.
}

int Spark_Restore(void* buffer, size_t max_length, uint8_t type, void* reserved)
{
	size_t length = 0;
	int error = HAL_System_Backup_Restore(0, buffer, max_length, &length, nullptr);
	if (error)
		length = 0;
	return length;
}

void update_persisted_state(std::function<void(SessionPersistOpaque&)> fn)
{
	SessionPersistOpaque persist;
	if (Spark_Restore(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr)==sizeof(persist) && persist.is_valid())
	{
		fn(persist);
		Spark_Save(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr);
	}
}

uint32_t compute_cloud_state_checksum(SparkAppStateSelector::Enum stateSelector, SparkAppStateUpdate::Enum operation, uint32_t value, void* reserved)
{
	if (operation == SparkAppStateUpdate::COMPUTE_AND_PERSIST) {
		switch (stateSelector)
		{
		case SparkAppStateSelector::DESCRIBE_APP:
			update_persisted_state([](SessionPersistData& data){
				data.describe_app_crc = compute_describe_app_checksum();
				data.app_state_flags |= AppStateDescriptor::APP_DESCRIBE_CRC;
			});
			break;
		case SparkAppStateSelector::DESCRIBE_SYSTEM:
			update_persisted_state([](SessionPersistData& data){
				data.describe_system_crc = compute_describe_system_checksum();
				data.app_state_flags |= AppStateDescriptor::SYSTEM_DESCRIBE_CRC;
			});
			break;
		}
	}
	else if (operation == SparkAppStateUpdate::PERSIST)
	{
		switch (stateSelector) {
		case SparkAppStateSelector::SUBSCRIPTIONS: {
			update_persisted_state([value](SessionPersistData& data){
				data.subscriptions_crc = value;
				data.app_state_flags |= AppStateDescriptor::SUBSCRIPTIONS_CRC;
			});
			break;
		}
		case SparkAppStateSelector::PROTOCOL_FLAGS: {
			update_persisted_state([value](SessionPersistData& data){
				data.protocol_flags = value;
				data.app_state_flags |= AppStateDescriptor::PROTOCOL_FLAGS;
			});
			break;
		}
		default:
			break;
		}
	}
	else if (operation == SparkAppStateUpdate::COMPUTE)
	{
		switch (stateSelector)
		{
		case SparkAppStateSelector::DESCRIBE_APP:
			return compute_describe_app_checksum();

		case SparkAppStateSelector::DESCRIBE_SYSTEM:
			return compute_describe_system_checksum();
		}
	}
	else if (operation == SparkAppStateUpdate::RESET && stateSelector == SparkAppStateSelector::ALL)
	{
		update_persisted_state([](SessionPersistData& data) {
			data.describe_system_crc = 0;
			data.describe_app_crc = 0;
			data.subscriptions_crc = 0;
			data.protocol_flags = 0;
			data.app_state_flags = 0;
		});
	}
	return 0;
}
#endif /* HAL_PLATFORM_CLOUD_UDP */

void system_set_time(uint32_t time, unsigned param, void*)
{
    struct timeval tv = {
        .tv_sec = (time_t)time,
        .tv_usec = 0
    };
    hal_rtc_set_time(&tv, nullptr);
    system_notify_event(time_changed, time_changed_sync);
}

namespace {

// LED status for the test signal that can be triggered from the cloud
class LEDCloudSignalStatus: public LEDStatus {
public:
    explicit LEDCloudSignalStatus(LEDPriority priority) :
            LEDStatus(LED_PATTERN_CUSTOM, priority),
            ticks_(0),
            index_(0) {
        updateColor();
    }

protected:
    virtual void update(system_tick_t t) override {
        if (t >= ticks_) {
            // Change LED color
            if (++index_ == COLOR_COUNT) {
                index_ = 0;
            }
            updateColor();
        } else {
            ticks_ -= t; // Update timing
        }
    }

private:
    uint16_t ticks_;
    uint8_t index_;

    void updateColor() {
        setColor(COLORS[index_]);
        ticks_ = 100;
    }

    static const uint32_t COLORS[];
    static const size_t COLOR_COUNT;
};

const uint32_t LEDCloudSignalStatus::COLORS[] = { 0xEE82EE, 0x4B0082, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFFA500, 0xFF0000 }; // VIBGYOR
const size_t LEDCloudSignalStatus::COLOR_COUNT = sizeof(LEDCloudSignalStatus::COLORS) / sizeof(LEDCloudSignalStatus::COLORS[0]);

void clientMessagesProcessed(void* reserved) {
    if (SPARK_CLOUD_HANDSHAKE_PENDING) {
        SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE = 1;
        SPARK_CLOUD_HANDSHAKE_PENDING = 0;
        LOG(INFO, "All handshake messages have been processed");
    }
}

bool publishSafeModeEventIfNeeded() {
    if (system_mode() == SAFE_MODE) {
        LOG(INFO, "Sending safe mode event");
        return publishEvent("spark/device/safemode", "");
    }
    return true; // ok
}

#if HAL_PLATFORM_COMPRESSED_OTA
// Minimum bootloader version required to support compressed/combined OTA updates
const uint16_t COMPRESSED_OTA_MIN_BOOTLOADER_VERSION = 1000; // 2.0.0
#endif // HAL_PLATFORM_COMPRESSED_OTA

} // namespace

void Spark_Signal(bool on, unsigned, void*)
{
    static LEDCloudSignalStatus ledCloudSignal(LED_PRIORITY_IMPORTANT);
    ledCloudSignal.setActive(on);
}

size_t system_interpolate_cloud_server_hostname(const char* var, size_t var_len, char* buf, size_t buf_len)
{
    if (var_len==2 && memcmp("id", var, 2)==0)
    {
        String deviceID = spark_deviceID();
        size_t id_len = deviceID.length();

        SystemVersionInfo sys_ver;
        system_version_info(&sys_ver, nullptr);
        uint8_t mv = BYTE_N(sys_ver.versionNumber, 3);
        String majorVer = String::format(".v%d", mv);
        size_t mv_len = majorVer.length();

        if (buf_len > (id_len + mv_len)) {
            memcpy(buf, deviceID.c_str(), id_len);
            memcpy(buf + id_len, majorVer.c_str(), mv_len);
            return id_len + mv_len;
        }
    }
    return 0;
}

int userVarType(const char *varKey)
{
    User_Var_Lookup_Table_t* item = find_var_by_key(varKey);
    return item ? item->userVarType : -1;
}

SparkReturnType::Enum protocolVariableType(Spark_Data_TypeDef type) {
    switch (type) {
    case CLOUD_VAR_BOOLEAN:
        return SparkReturnType::BOOLEAN;
    case CLOUD_VAR_DOUBLE:
        return SparkReturnType::DOUBLE;
    case CLOUD_VAR_STRING:
        return SparkReturnType::STRING;
    default:
        return SparkReturnType::INT;
    }
}

size_t variableDataSize(const void* data, Spark_Data_TypeDef type) {
    switch (type) {
    case CLOUD_VAR_BOOLEAN:
        return sizeof(bool);
    case CLOUD_VAR_DOUBLE:
        return sizeof(double);
    case CLOUD_VAR_STRING:
        return data ? strlen((const char*)data) : 0;
    default:
        return sizeof(uint32_t);
    }
}

void getUserVarResult(int error, int type, void* data, size_t size, SparkDescriptor::GetVariableCallback callback,
        void* context) {
    SYSTEM_THREAD_CONTEXT_ASYNC(getUserVarResult(error, type, data, size, callback, context));
    callback(error, type, data, size, context);
}

void getUserVarImpl(User_Var_Lookup_Table_t* item, SparkDescriptor::GetVariableCallback callback, void* context)
{
    APPLICATION_THREAD_CONTEXT_ASYNC(getUserVarImpl(item, callback, context));
    size_t size = 0;
    void* copy = nullptr;
    NAMED_SCOPE_GUARD(copyGuard, {
        free(copy);
    });
    if (item->copy) {
        const int result = item->copy(item->userVar, &copy, &size);
        if (result < 0) {
            getUserVarResult(ProtocolError::NO_MEMORY, 0 /* type */, nullptr /* data */, 0 /* size */, callback, context);
            return;
        }
    } else {
        const void* data = nullptr;
        if (item->update) {
            data = item->update(item->userVarKey, item->userVarType, item->userVar, nullptr);
        } else {
            data = item->userVar;
        }
        size = variableDataSize(data, item->userVarType);
        if (size > 0) {
            copy = malloc(size);
            if (!copy) {
                getUserVarResult(ProtocolError::NO_MEMORY, 0, nullptr, 0, callback, context);
                return;
            }
            memcpy(copy, data, size);
        }
    }
    const auto type = protocolVariableType(item->userVarType); // Spark_Data_TypeDef -> SparkReturnType::Enum
    getUserVarResult(ProtocolError::NO_ERROR, type, copy, size, callback, context);
    copyGuard.dismiss();
}

void getUserVar(const char* varKey, SparkDescriptor::GetVariableCallback callback, void* context)
{
    const auto item = find_var_by_key(varKey);
    if (item) {
        getUserVarImpl(item, callback, context);
    } else {
        callback(ProtocolError::NOT_FOUND, 0 /* type */, nullptr /* data */, 0 /* size */, context);
    }
}

void userFuncScheduleImpl(User_Func_Lookup_Table_t* item, const char* paramString, bool freeParamString, SparkDescriptor::FunctionResultCallback callback)
{
    int result = item->pUserFunc(item->pUserFuncData, paramString, NULL);
    if (freeParamString)
        delete paramString;
    // run the cloud return on the system thread again
    SYSTEM_THREAD_CONTEXT_ASYNC(callback((const void*)long(result), SparkReturnType::INT));
    callback((const void*)long(result), SparkReturnType::INT);
}

int userFuncSchedule(const char *funcKey, const char *paramString, SparkDescriptor::FunctionResultCallback callback, void* reserved)
{
    // for now, we invoke the function directly and return the result via the callback
    User_Func_Lookup_Table_t* item = find_func_by_key(funcKey);
    if (!item)
        return -1;

#if PLATFORM_THREADING
    paramString = strdup(paramString);      // ensure we have a copy since the oriignal isn't guaranteed to be available once this function returns.
    APPLICATION_THREAD_CONTEXT_ASYNC_RESULT(userFuncScheduleImpl(item, paramString, true, callback), 0);
    userFuncScheduleImpl(item, paramString, true, callback);
#else
    userFuncScheduleImpl(item, paramString, false, callback);
#endif
    return 0;
}

int formatOtaUpdateStatusEventData(uint32_t flags, int result, uint8_t *buf, size_t size)
{
    memset(buf, 0, size);

    BufferAppender appender(buf, size);
    appender.append("{\"r\":");

    char str[12] = {};
    snprintf(str, sizeof(str), "%d", result);

    appender.append(str);
    appender.append("}");

    return 0;
}

int finish_ota_firmware_update(FileTransfer::Descriptor& file, uint32_t flags, void* buf)
{
    using namespace particle::protocol;

    int result = Spark_Finish_Firmware_Update(file, flags, nullptr);

    if (buf && (flags & UpdateFlag::SUCCESS)) {
        formatOtaUpdateStatusEventData(flags, result, (uint8_t*)buf, 255 /* :( */);
    }

    return result;
}

static const char* resetReasonString(System_Reset_Reason reason)
{
    switch (reason) {
    case RESET_REASON_UNKNOWN:
        return "unknown";
    case RESET_REASON_PIN_RESET:
        return "pin_reset";
    case RESET_REASON_POWER_MANAGEMENT:
        return "power_management";
    case RESET_REASON_POWER_DOWN:
        return "power_down";
    case RESET_REASON_POWER_BROWNOUT:
        return "power_brownout";
    case RESET_REASON_WATCHDOG:
        return "watchdog";
    case RESET_REASON_UPDATE:
        return "update";
    case RESET_REASON_UPDATE_ERROR:
        return "update_error";
    case RESET_REASON_UPDATE_TIMEOUT:
        return "update_timeout";
    case RESET_REASON_FACTORY_RESET:
        return "factory_reset";
    case RESET_REASON_SAFE_MODE:
        return "safe_mode";
    case RESET_REASON_DFU_MODE:
        return "dfu_mode";
    case RESET_REASON_PANIC:
        return "panic";
    case RESET_REASON_USER:
        return "user";
    default:
        return nullptr;
    }
}

static const char* panicCodeString(ePanicCode code)
{
    switch (code) {
    case HardFault:
        return "hard_fault";
    case MemManage:
        return "memory_fault";
    case BusFault:
        return "bus_fault";
    case UsageFault:
        return "usage_fault";
    case OutOfHeap:
        return "out_of_heap";
    case AssertionFailure:
        return "assert_failed";
    case StackOverflow:
        return "stack_overflow";
    default:
        return nullptr;
    }
}

static void formatResetReasonEventData(int reason, uint32_t data, char *buf, size_t size)
{
    // TODO: Consider using numeric reason codes instead of string identifiers for uniformity
    // with the CoAP protocol

    // Reset reason
    int n = 0;
    const char* s = resetReasonString((System_Reset_Reason)reason);
    if (s) {
        n = snprintf(buf, size, "%s", s);
    } else {
        n = snprintf(buf, size, "%d", (int)reason); // Print as numeric code
    }
    if (n < 0 || n >= (int)size) {
        return;
    }
    buf += n;
    size -= n;
    // Additional data for selected reason codes
    if (reason == RESET_REASON_PANIC) {
        s = panicCodeString((ePanicCode)data);
        if (s) {
            n = snprintf(buf, size, ", %s", s);
        } else {
            n = snprintf(buf, size, ", %d", (int)data);
        }
    }
}

bool system_cloud_active()
{
    const int SYSTEM_CLOUD_TIMEOUT = 15*1000;
    if (!SPARK_CLOUD_SOCKETED)
        return false;

#if HAL_PLATFORM_CLOUD_UDP
    if (!feature_cloud_udp)
#endif
    {
        system_tick_t now = millis();
        if (SPARK_CLOUD_CONNECTED && ((now-lastCloudEvent))>SYSTEM_CLOUD_TIMEOUT)
        {
            WARN("Disconnecting cloud due to inactivity! %d, %d", now, lastCloudEvent);
            // TODO: Not sure if we want to specify a disconnection reason here
            cloud_disconnect(HAL_PLATFORM_MAY_LEAK_SOCKETS ? CLOUD_DISCONNECT_DONT_CLOSE : 0,
                    CLOUD_DISCONNECT_REASON_NONE);
            return false;
        }
    }
    return true;
}

void Spark_Protocol_Init(void)
{
    system_cloud_protocol_instance();

    if (!spark_protocol_is_initialized(sp))
    {
        product_details_t info;
        info.size = sizeof(info);
        spark_protocol_get_product_details(sp, &info);

        particle_key_errors = NO_ERROR;

        // User code was run, so persist the current values stored in the comms lib.
        // These will either have been left as default or overridden via PRODUCT_ID/PRODUCT_VERSION macros
        if (system_mode()!=SAFE_MODE) {
            HAL_SetProductStore(PRODUCT_STORE_ID, info.product_id);
            HAL_SetProductStore(PRODUCT_STORE_VERSION, info.product_version);
        }
        else {      // user code was not executed, use previously persisted values
            info.product_id = HAL_GetProductStore(PRODUCT_STORE_ID);
            info.product_version = HAL_GetProductStore(PRODUCT_STORE_VERSION);
            if (info.product_id!=0xFFFF)
                spark_protocol_set_product_id(sp, info.product_id);
            if (info.product_version!=0xFFFF)
                spark_protocol_set_product_firmware_version(sp, info.product_version);
        }

#if HAL_PLATFORM_CLOUD_UDP
        const bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#else
        const bool udp = false;
#endif // HAL_PLATFORM_CLOUD_UDP

        SparkCallbacks callbacks;
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.size = sizeof(callbacks);
        callbacks.protocolFactory = udp ? PROTOCOL_DTLS : PROTOCOL_LIGHTSSL;
#if HAL_PLATFORM_CLOUD_UDP
        if (udp)
        {
            callbacks.send = Spark_Send_UDP;
            callbacks.receive = Spark_Receive_UDP;
            callbacks.transport_context = &g_system_cloud_session_data;
            callbacks.save = Spark_Save;
            callbacks.restore = Spark_Restore;
        }
        else
#endif
        {
            callbacks.send = Spark_Send;
            callbacks.receive = Spark_Receive;
            callbacks.transport_context = nullptr;
        }
        callbacks.prepare_for_firmware_update = Spark_Prepare_For_Firmware_Update;
        //callbacks.finish_firmware_update = Spark_Finish_Firmware_Update;
        callbacks.finish_firmware_update = finish_ota_firmware_update;
        callbacks.calculate_crc = HAL_Core_Compute_CRC32;
        callbacks.save_firmware_chunk = Spark_Save_Firmware_Chunk;
        callbacks.signal = Spark_Signal;
        callbacks.millis = HAL_Timer_Get_Milli_Seconds;
        callbacks.set_time = system_set_time;
        callbacks.notify_client_messages_processed = clientMessagesProcessed;

        SparkDescriptor descriptor;
        memset(&descriptor, 0, sizeof(descriptor));
        descriptor.size = sizeof(descriptor);
        descriptor.num_functions = numUserFunctions;
        descriptor.get_function_key = getUserFunctionKey;
        descriptor.call_function = userFuncSchedule;
        descriptor.num_variables = numUserVariables;
        descriptor.get_variable_key = getUserVariableKey;
        descriptor.variable_type = wrapVarTypeInEnum;
        descriptor.get_variable_async = getUserVar;
        descriptor.was_ota_upgrade_successful = HAL_OTA_Flashed_GetStatus;
        descriptor.ota_upgrade_status_sent = HAL_OTA_Flashed_ResetStatus;
        descriptor.append_system_info = system_module_info;
        descriptor.append_metrics = system_metrics;
        descriptor.call_event_handler = invokeEventHandler;
#if HAL_PLATFORM_CLOUD_UDP
        descriptor.app_state_selector_info = compute_cloud_state_checksum;
#endif
        // todo - this pushes a lot of data on the stack! refactor to remove heavy stack usage
        unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];
        unsigned char private_key[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];
        memset(&pubkey, 0xff, sizeof(pubkey));
        memset(&private_key, 0xff, sizeof(private_key));

        SparkKeys keys;
        keys.size = sizeof(keys);
        keys.server_public = pubkey;
        keys.core_private = private_key;

        // ensure the private key is read first since the public key may be derived from it.
        private_key_generation_t genspec;
        genspec.size = sizeof(genspec);
        genspec.gen = PRIVATE_KEY_GENERATE_MISSING;
        HAL_FLASH_Read_CorePrivateKey(private_key, &genspec);
        HAL_FLASH_Read_ServerPublicKey(pubkey);

        // if public server key is erased, restore with a backup from system firmware
        if (pubkey[0] == 0xff) {
            LOG(WARN, "Public Server Key was blank, restoring.");
            if (udp) {
#if HAL_PLATFORM_CLOUD_UDP
                memcpy(&pubkey, backup_udp_public_server_key, backup_udp_public_server_key_size);
#endif // HAL_PLATFORM_CLOUD_UDP
            } else {
#if HAL_PLATFORM_CLOUD_TCP
                memcpy(&pubkey, backup_tcp_public_server_key, sizeof(backup_tcp_public_server_key));
#endif // HAL_PLATFORM_CLOUD_TCP
            }
            particle_key_errors |= PUBLIC_SERVER_KEY_BLANK;
        }

        uint8_t id_length = HAL_device_ID(NULL, 0);
        uint8_t id[id_length];
        HAL_device_ID(id, id_length);
        spark_protocol_init(sp, (const char*) id, keys, callbacks, descriptor);

        // Enable device-initiated describe messages
        spark_protocol_set_connection_property(sp, protocol::Connection::DEVICE_INITIATED_DESCRIBE, 0, nullptr, nullptr);

#if HAL_PLATFORM_COMPRESSED_OTA
        // Enable compressed/combined OTA updates
        if (bootloader_get_version() >= COMPRESSED_OTA_MIN_BOOTLOADER_VERSION) {
            spark_protocol_set_connection_property(sp, protocol::Connection::COMPRESSED_OTA, 0, nullptr, nullptr);
        }
#endif // HAL_PLATFORM_COMPRESSED_OTA

        CommunicationsHandlers handlers;
        handlers.size = sizeof(handlers);
        handlers.random_seed_from_cloud = random_seed_from_cloud_handler;
        spark_protocol_communications_handlers(sp, &handlers);

        registerSystemSubscriptions();
    }
}

int Send_Firmware_Update_Flags()
{
    // short-circuiting the current state and always sending these flags
    /*
      Problem
      The device previously only sends the value of these flags when they
      have changed from the default. However, they are reset when the device
      resets, yet the cloud is not informed of this change.

      Solution
      Send the state of these flags whenever connecting to the cloud, so the
      cloud has the latest state. This will increase data usage marginally.
      This feature will be reworked in a later release to allow
      synchronization with less data use.
     */
    if (true || !System.updatesEnabled()) {
    	// force the event to be resent. The cloud assumes updates
    	// are enabled by default.
    	system_refresh_flag(SYSTEM_FLAG_OTA_UPDATE_ENABLED);
    }

    if (true || System.updatesForced()) {
    	system_refresh_flag(SYSTEM_FLAG_OTA_UPDATE_FORCED);
    }

    return 0;
}

int Spark_Handshake(bool presence_announce)
{
    cloud_socket_aborted = false; // Clear cancellation flag for socket operations
    LOG(INFO,"Starting handshake: presense_announce=%d", presence_announce);
    bool session_resumed = false;
    int err = spark_protocol_handshake(sp);

#if HAL_PLATFORM_MUXER_MAY_NEED_DELAY_IN_TX
    // XXX: Adding a delay only for platforms Boron and BSoM, because older cell versions of
    // Boron R410M modems crash when handshake messages are sent without gap.
    // We have a workaround for this issue in the NCP client, so the modem should no longer crash
    // in any case, but we want to avoid dropping a set of publishes which are generated below,
    // hence a delay here as a workaround.
    auto timeout = cellularNetworkManager()->ncpClient()->getTxDelayInDataChannel();
    if (timeout > 0) {
        HAL_Delay_Milliseconds(timeout);
    }
#endif // HAL_PLATFORM_MUXER_MAY_NEED_DELAY_IN_TX

    if (err == protocol::SESSION_RESUMED) {
        session_resumed = true;
    } else if (err != 0) {
        return spark_protocol_to_system_error(err);
    }
    if (!session_resumed) {
        char buf[CLAIM_CODE_SIZE + 1];
        if (!HAL_Get_Claim_Code(buf, sizeof(buf)) && buf[0] != 0 && (uint8_t)buf[0] != 0xff) {
            LOG(INFO,"Send spark/device/claim/code event for code %s", buf);
            publishEvent("spark/device/claim/code", buf);
        }

        // open up for possibility of retrieving multiple ID datums
        if (!HAL_Get_Device_Identifier(NULL, buf, sizeof(buf), 0, NULL) && *buf) {
            LOG(INFO,"Send spark/device/ident/0 event");
            publishEvent("spark/device/ident/0", buf);
        }

        bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#if PLATFORM_ID!=PLATFORM_ELECTRON_PRODUCTION || !defined(MODULAR_FIRMWARE)
        ultoa(HAL_OTA_FlashLength(), buf, 10);
        LOG(INFO,"Send spark/hardware/max_binary event");
        publishEvent("spark/hardware/max_binary", buf);
#endif

        uint32_t chunkSize = HAL_OTA_ChunkSize();
        if (chunkSize!=512 || !udp) {
            ultoa(chunkSize, buf, 10);
            LOG(INFO,"spark/hardware/ota_chunk_size event");
            publishEvent("spark/hardware/ota_chunk_size", buf);
        }

        publishSafeModeEventIfNeeded();

        uint8_t flag = 0;
        if (system_get_flag(SYSTEM_FLAG_PUBLISH_RESET_INFO, &flag, nullptr) == 0 && flag)
        {
            system_set_flag(SYSTEM_FLAG_PUBLISH_RESET_INFO, 0, nullptr); // Publish the reset info only once
            int reason = RESET_REASON_NONE;
            uint32_t data = 0;
            if (HAL_Core_Get_Last_Reset_Info(&reason, &data, nullptr) == 0 && reason != RESET_REASON_NONE)
            {
                char buf[64];
                formatResetReasonEventData(reason, data, buf, sizeof(buf));
                LOG(INFO,"Send spark/device/last_reset event");
                publishEvent("spark/device/last_reset", buf);
            }
        }

        Send_Firmware_Update_Flags();

        if (presence_announce) {
            Multicast_Presence_Announcement();
        }
        spark_protocol_send_time_request(sp);
    } else {
        LOG(INFO,"cloud connected from existing session.");

        publishSafeModeEventIfNeeded();
        Send_Firmware_Update_Flags();

        if (!hal_rtc_time_is_valid(nullptr) && spark_sync_time_last(nullptr, nullptr) == 0) {
            spark_protocol_send_time_request(sp);
        }
    }
    if (system_mode() != AUTOMATIC || APPLICATION_SETUP_DONE) {
        err = sendApplicationDescription();
        if (err != 0) {
            return err;
        }
    }
    if (particle_key_errors != NO_ERROR) {
        char buf[sizeof(unsigned long)*8+1];
        ultoa((unsigned long)particle_key_errors, buf, 10);
        LOG(INFO,"Send event spark/device/key/error=%s", buf);
        publishEvent("spark/device/key/error", buf);
    }
    protocol_status status = {};
    status.size = sizeof(status);
    err = spark_protocol_get_status(sp, &status, nullptr);
    if (err != 0) {
        return spark_protocol_to_system_error(err);
    }
    if (status.flags & PROTOCOL_STATUS_HAS_PENDING_CLIENT_MESSAGES) {
        SPARK_CLOUD_HANDSHAKE_PENDING = 1;
        LOG(TRACE, "Waiting until all handshake messages are processed by the protocol layer");
    } else {
        SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE = 1;
    }
    return 0;
}

// Returns true if all's well or
//         false on error, meaning we're probably disconnected

bool Spark_Communication_Loop(void)
{
    return spark_protocol_event_loop(sp);
}

/**
 * This is the internal function called by the background loop to pump cloud events.
 */
void Spark_Process_Events()
{
    if (SPARK_CLOUD_SOCKETED && !Spark_Communication_Loop())
    {
        WARN("Communication loop error, closing cloud socket");
        cloud_disconnect(HAL_PLATFORM_MAY_LEAK_SOCKETS ? CLOUD_DISCONNECT_DONT_CLOSE : 0, CLOUD_DISCONNECT_REASON_ERROR);
    }
    else
    {
        lastCloudEvent = millis();
    }
}

namespace {

CloudDiagnostics g_cloudDiagnostics;
CloudConnectionSettings g_cloudConnectionSettings;

} // namespace

inline void concat_nibble(String& result, uint8_t nibble)
{
    char hex_digit = nibble + 48;
    if (57 < hex_digit)
        hex_digit += 39;
    result.concat(hex_digit);
}

String bytes2hex(const uint8_t* buf, unsigned len)
{
    String result;
    for (unsigned i = 0; i < len; ++i)
    {
        concat_nibble(result, (buf[i] >> 4));
        concat_nibble(result, (buf[i] & 0xF));
    }
    return result;
}

CloudDiagnostics* CloudDiagnostics::instance() {
    return &g_cloudDiagnostics;
}

CloudConnectionSettings* CloudConnectionSettings::instance() {
    return &g_cloudConnectionSettings;
}
