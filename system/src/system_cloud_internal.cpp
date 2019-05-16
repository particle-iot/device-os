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
#include "system_network.h"
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
#include "system_string_interpolate.h"
#include "dtls_session_persist.h"
#include "bytes2hexbuf.h"
#include "system_event.h"
#include "system_cloud_connection.h"
#include "str_util.h"
#include <stdio.h>
#include <stdint.h>

using particle::CloudDiagnostics;

#ifndef SPARK_NO_CLOUD

extern uint8_t feature_cloud_udp;
extern volatile bool cloud_socket_aborted;

static volatile uint32_t lastCloudEvent = 0;
uint32_t particle_key_errors = NO_ERROR;

const int CLAIM_CODE_SIZE = 63;

using particle::LEDStatus;

int userVarType(const char *varKey);
const void *getUserVar(const char *varKey);
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
		data.flags = particle::protocol::DESCRIBE_APPLICATION;
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
	User_Var_Lookup_Table_t item = { .userVar = userVar, .userVarType = userVarType, 0, 0};
	if (extra) {
		item.update = extra->update;
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
		checksum += crc(info.modules[i].suffix->sha);
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

void SystemEvents(const char* name, const char* data);

bool is_system_handler(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo) {
	// for now we hack this to recognize our own system handler
    return handlerInfo->handler==SystemEvents;
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

constexpr const char CLAIM_EVENTS[] = "spark/device/claim/";
constexpr const char RESET_EVENT[] = "spark/device/reset";
constexpr const char KEY_RESTORE_EVENT[] = "spark/device/key/restore";
constexpr const char DEVICE_UPDATES_EVENT[] = "particle/device/updates/";
constexpr const char FORCED_EVENT[] = "forced";
constexpr const char UPDATES_PENDING_EVENT[] = "pending";

inline bool is_suffix(const char* eventName, const char* prefix, const char* suffix) {
	// todo - sanity check parameters?
	return !strncmp(eventName+strlen(prefix), suffix, strlen(eventName)-strlen(prefix));
}

uint8_t data_to_flag(const char* data) {
	return !strncmp(data, "true", strlen(data));
}

/**
 * Handler for system events.
 */
void SystemEvents(const char* name, const char* data)
{
    if (particle::startsWith(name, DEVICE_UPDATES_EVENT)) {
        const uint8_t flagValue = data_to_flag(data);
        if (is_suffix(name, DEVICE_UPDATES_EVENT, FORCED_EVENT)) {
            system_set_flag(SYSTEM_FLAG_OTA_UPDATE_FORCED, flagValue, nullptr);
        }
        else if (is_suffix(name, DEVICE_UPDATES_EVENT, UPDATES_PENDING_EVENT)) {
            system_set_flag(SYSTEM_FLAG_OTA_UPDATE_PENDING, flagValue, nullptr);
        }
    }
    else if (!strncmp(name, CLAIM_EVENTS, strlen(CLAIM_EVENTS))) {
        LOG(TRACE, "Claim code received by the cloud and cleared locally.");
        HAL_Set_Claim_Code(NULL);
    }
    else if (!strcmp(name, RESET_EVENT)) {
        if (data && *data) {
            if (!strcmp("safe mode", data))
                System.enterSafeMode();
            else if (!strcmp("dfu", data))
                System.dfu(false);
            else if (!strcmp("reboot", data))
                System.reset();
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

#if HAL_PLATFORM_CLOUD_UDP
using particle::protocol::SessionPersistOpaque;
using particle::protocol::SessionPersistData;

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
	if (operation==SparkAppStateUpdate::COMPUTE_AND_PERSIST ) {
		switch (stateSelector)
		{
		case SparkAppStateSelector::DESCRIBE_APP:
			update_persisted_state([](SessionPersistData& data){
				data.describe_app_crc = compute_describe_app_checksum();
			});
            break;
		case SparkAppStateSelector::DESCRIBE_SYSTEM:
			update_persisted_state([](SessionPersistData& data){
				data.describe_system_crc = compute_describe_system_checksum();
			});
            break;
		}
	}
	else if (operation==SparkAppStateUpdate::PERSIST && stateSelector==SparkAppStateSelector::SUBSCRIPTIONS)
	{
		update_persisted_state([value](SessionPersistData& data){
			data.subscriptions_crc = value;
		});
	}
	else if (operation==SparkAppStateUpdate::COMPUTE)
	{
		switch (stateSelector)
		{
		case SparkAppStateSelector::DESCRIBE_APP:
			return compute_describe_app_checksum();

		case SparkAppStateSelector::DESCRIBE_SYSTEM:
			return compute_describe_system_checksum();
		}
	}
	return 0;
}
#endif /* HAL_PLATFORM_CLOUD_UDP */

void system_set_time(time_t time, unsigned param, void*)
{
    HAL_RTC_Set_UnixTime(time);
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
		if (buf_len>deviceID.length()) {
			memcpy(buf, deviceID.c_str(), deviceID.length());
			return deviceID.length();
		}
	}
	return 0;
}

int userVarType(const char *varKey)
{
    User_Var_Lookup_Table_t* item = find_var_by_key(varKey);
    return item ? item->userVarType : -1;
}

const void *getUserVar(const char *varKey)
{
    User_Var_Lookup_Table_t* item = find_var_by_key(varKey);
    const void* result = nullptr;
    if (item) {
    	if (item->update)
            result = item->update(item->userVarKey, item->userVarType, item->userVar, nullptr);
    	else
            result = item->userVar;
    }
    return result;
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

int formatOtaUpdateStatusEventData(uint32_t flags, int result, hal_module_t* module, uint8_t *buf, size_t size)
{
    int res = 1;
    memset(buf, 0, size);

    BufferAppender appender(buf, size);
    appender.append("{");
    appender.append("\"r\":");
    appender.append(result ? "\"error\"" : "\"ok\"");

    if (flags & 1) {
        appender.append(",");
        res = ota_update_info(append_instance, &appender, module, false, NULL);
    }

    appender.append("}");

    return res;
}

int finish_ota_firmware_update(FileTransfer::Descriptor& file, uint32_t flags, void* buf)
{
    using namespace particle::protocol;
    hal_module_t module;

    int result = Spark_Finish_Firmware_Update(file, flags, &module);

    if (buf && (flags & (UpdateFlag::SUCCESS | UpdateFlag::VALIDATE_ONLY)) == (UpdateFlag::SUCCESS | UpdateFlag::VALIDATE_ONLY)) {
        formatOtaUpdateStatusEventData(flags, result, &module, (uint8_t*)buf, 255);
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
#ifndef SPARK_NO_CLOUD
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
            cloud_disconnect(false); // TODO: Do we need to specify a reason of the disconnection here?
            return false;
        }
    }
#endif
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

        SparkDescriptor descriptor;
        memset(&descriptor, 0, sizeof(descriptor));
        descriptor.size = sizeof(descriptor);
        descriptor.num_functions = numUserFunctions;
        descriptor.get_function_key = getUserFunctionKey;
        descriptor.call_function = userFuncSchedule;
        descriptor.num_variables = numUserVariables;
        descriptor.get_variable_key = getUserVariableKey;
        descriptor.variable_type = wrapVarTypeInEnum;
        descriptor.get_variable = getUserVar;
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

        Particle.subscribe("spark", SystemEvents, MY_DEVICES);
        Particle.subscribe("particle", SystemEvents, MY_DEVICES);

        CommunicationsHandlers handlers;
        handlers.size = sizeof(handlers);
        handlers.random_seed_from_cloud = random_seed_from_cloud_handler;
        spark_protocol_communications_handlers(sp, &handlers);
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
    int err = spark_protocol_handshake(sp);
    if (!err)
    {
        char buf[CLAIM_CODE_SIZE + 1];
        if (!HAL_Get_Claim_Code(buf, sizeof (buf)) && buf[0] != 0 && (uint8_t)buf[0] != 0xff)
        {
            LOG(INFO,"Send spark/device/claim/code event for code %s", buf);
            Particle.publish("spark/device/claim/code", buf, 60, PRIVATE);
        }

        // open up for possibility of retrieving multiple ID datums
        if (!HAL_Get_Device_Identifier(NULL, buf, sizeof(buf), 0, NULL) && *buf) {
            LOG(INFO,"Send spark/device/ident/0 event");
            Particle.publish("spark/device/ident/0", buf, 60, PRIVATE);
        }

        bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#if PLATFORM_ID!=PLATFORM_ELECTRON_PRODUCTION || !defined(MODULAR_FIRMWARE)
        ultoa(HAL_OTA_FlashLength(), buf, 10);
        LOG(INFO,"Send spark/hardware/max_binary event");
        Particle.publish("spark/hardware/max_binary", buf, 60, PRIVATE);
#endif

        uint32_t chunkSize = HAL_OTA_ChunkSize();
        if (chunkSize!=512 || !udp) {
            ultoa(chunkSize, buf, 10);
            LOG(INFO,"spark/hardware/ota_chunk_size event");
            Particle.publish("spark/hardware/ota_chunk_size", buf, 60, PRIVATE);
        }
        if (system_mode()==SAFE_MODE) {
            LOG(INFO,"Send spark/device/safemode event");
            Particle.publish("spark/device/safemode","", 60, PRIVATE);
        }
#if defined(SPARK_SUBSYSTEM_EVENT_NAME)
        if (!HAL_core_subsystem_version(buf, sizeof (buf)) && *buf)
        {
            LOG(INFO,"Send spark/" SPARK_SUBSYSTEM_EVENT_NAME " event");
            Particle.publish("spark/" SPARK_SUBSYSTEM_EVENT_NAME, buf, 60, PRIVATE);
        }
#endif
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
                Particle.publish("spark/device/last_reset", buf, 60, PRIVATE);
            }
        }

        Send_Firmware_Update_Flags();

        if (presence_announce) {
            Multicast_Presence_Announcement();
        }
        LOG(INFO,"Send subscriptions");
        spark_protocol_send_subscriptions(sp);
        // important this comes at the end since it requires a response from the cloud.
        spark_protocol_send_time_request(sp);
        Spark_Process_Events();
    }
    if (err==particle::protocol::SESSION_RESUMED)
    {
        LOG(INFO,"cloud connected from existing session.");
        err = 0;
        if (!HAL_RTC_Time_Is_Valid(nullptr) && spark_sync_time_last(nullptr, nullptr) == 0) {
            spark_protocol_send_time_request(sp);
            Spark_Process_Events();
        }

        Send_Firmware_Update_Flags();
    }
    if (particle_key_errors != NO_ERROR) {
        char buf[sizeof(unsigned long)*8+1];
        ultoa((unsigned long)particle_key_errors, buf, 10);
        LOG(INFO,"Send event spark/device/key/error=%s", buf);
        Particle.publish("spark/device/key/error", buf, 60, PRIVATE);
    }
    return err;
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
        cloud_disconnect(false, false, CLOUD_DISCONNECT_REASON_ERROR);
    }
    else
    {
        lastCloudEvent = millis();
    }
}

#endif // SPARK_NO_CLOUD

namespace {

CloudDiagnostics g_cloudDiagnostics;

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

void Spark_Sleep(void)
{
#ifndef SPARK_NO_CLOUD
	spark_protocol_command(sp, ProtocolCommands::SLEEP);
#endif
}

void Spark_Wake(void)
{
#ifndef SPARK_NO_CLOUD
	spark_protocol_command(sp, ProtocolCommands::WAKE);
#endif
}

CloudDiagnostics* CloudDiagnostics::instance() {
    return &g_cloudDiagnostics;
}
