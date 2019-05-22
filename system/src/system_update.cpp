/**
 ******************************************************************************
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

#include <stddef.h>
#include "spark_wiring_cloud.h"
#include "spark_wiring_system.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_rgb.h"
#include "spark_wiring_led.h"
#include "spark_wiring_usbserial.h"
#include "spark_wiring_diagnostics.h"
#include "ota_flash_hal.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "system_event.h"
#include "system_update.h"
#include "system_cloud_internal.h"
#include "system_network.h"
#include "system_ymodem.h"
#include "system_task.h"
#include "module_info.h"
#include "spark_protocol_functions.h"
#include "string_convert.h"
#include "appender.h"
#include "system_version.h"
#include "spark_macros.h"
#include "system_network_internal.h"
#include "bytes2hexbuf.h"
#include "system_threading.h"
#if HAL_PLATFORM_DCT
#include "dct.h"
#endif // HAL_PLATFORM_DCT

#ifdef START_DFU_FLASHER_SERIAL_SPEED
static uint32_t start_dfu_flasher_serial_speed = START_DFU_FLASHER_SERIAL_SPEED;
#endif
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
static uint32_t start_ymodem_flasher_serial_speed = START_YMODEM_FLASHER_SERIAL_SPEED;
#endif

ymodem_serial_flash_update_handler Ymodem_Serial_Flash_Update_Handler = NULL;

volatile uint8_t SPARK_CLOUD_SOCKETED;
volatile uint8_t SPARK_CLOUD_CONNECTED;
volatile uint8_t SPARK_FLASH_UPDATE;
volatile uint32_t TimingFlashUpdateTimeout;

static_assert(SYSTEM_FLAG_OTA_UPDATE_PENDING==0, "system flag value");
static_assert(SYSTEM_FLAG_OTA_UPDATE_ENABLED==1, "system flag value");
static_assert(SYSTEM_FLAG_RESET_PENDING==2, "system flag value");
static_assert(SYSTEM_FLAG_RESET_ENABLED==3, "system flag value");
static_assert(SYSTEM_FLAG_STARTUP_LISTEN_MODE == 4, "system flag value");
static_assert(SYSTEM_FLAG_WIFITESTER_OVER_SERIAL1 == 5, "system flag value");
static_assert(SYSTEM_FLAG_PUBLISH_RESET_INFO == 6, "system flag value");
static_assert(SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS == 7, "system flag value");
static_assert(SYSTEM_FLAG_PM_DETECTION == 8, "system flag value");
static_assert(SYSTEM_FLAG_OTA_UPDATE_FORCED == 9, "system flag value");
static_assert(SYSTEM_FLAG_MAX == 10, "system flag max value");

volatile uint8_t systemFlags[SYSTEM_FLAG_MAX] = {
    0, 1, // OTA updates pending/enabled
    0, 1, // Reset pending/enabled
    0,    // SYSTEM_FLAG_STARTUP_LISTEN_MODE
    0,    // SYSTEM_FLAG_SETUP_OVER_SERIAL1
    1,    // SYSTEM_FLAG_PUBLISH_RESET_INFO
    1,    // SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS
    0,    // UNUSED (SYSTEM_FLAG_PM_DETECTION)
	0,	  // SYSTEM_FLAG_OTA_UPDATE_FORCED
};

const uint16_t SAFE_MODE_LISTEN = 0x5A1B;

const char* UPDATES_ENABLED_EVENT = "particle/device/updates/enabled";
const char* UPDATES_FORCED_EVENT = "particle/device/updates/forced";

const char* flag_to_string(uint8_t flag) {
    return flag ? "true" : "false";
}

void system_flag_changed(system_flag_t flag, uint8_t oldValue, uint8_t newValue)
{
    if (flag == SYSTEM_FLAG_STARTUP_LISTEN_MODE)
    {
        HAL_Core_Write_Backup_Register(BKP_DR_09, newValue ? SAFE_MODE_LISTEN : 0xFFFF);
    }
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    else if (flag == SYSTEM_FLAG_PM_DETECTION) {
        static_assert(DCT_PM_DETECT_SIZE == sizeof(newValue), "");
        system_get_flag(flag, &oldValue, nullptr);
        if (oldValue != newValue) {
            newValue = !newValue;
            dct_write_app_data(&newValue, DCT_PM_DETECT_OFFSET, sizeof(newValue));
        }
    }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    else if (flag == SYSTEM_FLAG_OTA_UPDATE_ENABLED)
    {
        // publish the firmware enabled event
        spark_send_event(UPDATES_ENABLED_EVENT, flag_to_string(newValue), 60, PUBLISH_EVENT_FLAG_ASYNC|PUBLISH_EVENT_FLAG_PRIVATE, nullptr);
    }
    else if (flag == SYSTEM_FLAG_OTA_UPDATE_FORCED)
    {
        // acknowledge to the cloud that system updates are forced. It helps avoid a race condition where we might try sending firmware before the event has been received.
        spark_send_event(UPDATES_FORCED_EVENT, flag_to_string(newValue), 60, PUBLISH_EVENT_FLAG_ASYNC|PUBLISH_EVENT_FLAG_PRIVATE, nullptr);
    }
    else if (flag == SYSTEM_FLAG_OTA_UPDATE_PENDING)
    {
        if (newValue) {
            system_notify_event(firmware_update_pending, 0, nullptr, nullptr, nullptr);
            // publish an internal system event for pending updates
    	}
	}
}

/**
 * Refreshes the flag by performing the update action.
 */
int system_refresh_flag(system_flag_t flag) {
    uint8_t value;
    int result = system_get_flag(flag, &value, nullptr);
    system_flag_changed(flag, value, value);
    return result;
}

int system_set_flag(system_flag_t flag, uint8_t value, void*)
{
    if (flag>=SYSTEM_FLAG_MAX)
        return -1;

    if (systemFlags[flag] != value || flag == SYSTEM_FLAG_STARTUP_LISTEN_MODE || flag == SYSTEM_FLAG_PM_DETECTION) {
        uint8_t oldValue = systemFlags[flag];
        systemFlags[flag] = value;
        system_flag_changed(flag, oldValue, value);
    }
    return 0;
}


int system_get_flag(system_flag_t flag, uint8_t* value, void*)
{
    if (flag>=SYSTEM_FLAG_MAX)
        return -1;
    if (value)
    {
        if (flag == SYSTEM_FLAG_STARTUP_LISTEN_MODE)
        {
            uint16_t reg = HAL_Core_Read_Backup_Register(BKP_DR_09);
            *value = (reg == SAFE_MODE_LISTEN);
            systemFlags[flag] = *value;
        }
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
        else if (flag == SYSTEM_FLAG_PM_DETECTION)
        {
            uint8_t v = 0;
            static_assert(DCT_PM_DETECT_SIZE == sizeof(v), "");
            auto r = dct_read_app_data_copy(DCT_PM_DETECT_OFFSET, &v, sizeof(v));
            *value = !r && !v;
            systemFlags[flag] = *value;
        }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
        else
        {
            *value = systemFlags[flag];
        }
    }
    return 0;
}


void set_ymodem_serial_flash_update_handler(ymodem_serial_flash_update_handler handler)
{
    Ymodem_Serial_Flash_Update_Handler = handler;
}

void set_start_dfu_flasher_serial_speed(uint32_t speed)
{
#ifdef START_DFU_FLASHER_SERIAL_SPEED
    start_dfu_flasher_serial_speed = speed;
#endif
}

void set_start_ymodem_flasher_serial_speed(uint32_t speed)
{
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
    start_ymodem_flasher_serial_speed = speed;
#endif
}

bool system_firmwareUpdate(Stream* stream, void* reserved)
{
#if PLATFORM_ID>2
    set_ymodem_serial_flash_update_handler(Ymodem_Serial_Flash_Update);
#endif
    system_file_transfer_t tx;
    tx.descriptor.store = FileTransfer::Store::FIRMWARE;
    tx.stream = stream;
    return system_fileTransfer(&tx);
}

bool system_fileTransfer(system_file_transfer_t* tx, void* reserved)
{
    bool status = false;
    Stream* serialObj = tx->stream;

    if (NULL != Ymodem_Serial_Flash_Update_Handler)
    {
        status = Ymodem_Serial_Flash_Update_Handler(serialObj, tx->descriptor, NULL);
        SPARK_FLASH_UPDATE = 0;
        TimingFlashUpdateTimeout = 0;

        if (status == true)
        {
            if (tx->descriptor.store==FileTransfer::Store::FIRMWARE) {
                serialObj->println("Restarting system to apply firmware update...");
                HAL_Delay_Milliseconds(100);
                HAL_Core_System_Reset_Ex(RESET_REASON_UPDATE, 0, nullptr);
            }
        }
    }
    else
    {
        serialObj->println("Firmware update using this terminal is not supported!");
        serialObj->println("Add #include \"Ymodem/Ymodem.h\" to your sketch and try again.");
    }
    return status;
}

void system_lineCodingBitRateHandler(uint32_t bitrate)
{
// todo - ideally the system should post a reset pending event before
// resetting. This does mean the application can block entering listening mode

#ifdef START_DFU_FLASHER_SERIAL_SPEED
    if (bitrate == start_dfu_flasher_serial_speed)
    {
        network_connect_cancel(0, 1, 0, 0);
        //Reset device and briefly enter DFU bootloader mode
        System.dfu(false);
    }
#endif
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
    if (!network_listening(0, 0, NULL) && bitrate == start_ymodem_flasher_serial_speed)
    {
        network_listen(0, 0, 0);
    }
#endif
}

uint32_t timeRemaining(uint32_t start, uint32_t duration)
{
    uint32_t elapsed = HAL_Timer_Milliseconds()-start;
    return (elapsed>=duration) ? 0 : duration-elapsed;
}

void set_flag(void* flag)
{
	volatile uint8_t* p = (volatile uint8_t*)flag;
	*p = true;
}

int Spark_Prepare_For_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* reserved)
{
    if (file.store==FileTransfer::Store::FIRMWARE)
    {
        // address is relative to the OTA region. Normally will be 0.
        file.file_address = HAL_OTA_FlashAddress() + file.chunk_address;

        // chunk_size 0 indicates defaults.
        if (file.chunk_size==0) {
            file.chunk_size = HAL_OTA_ChunkSize();
            file.file_length = HAL_OTA_FlashLength();
        }
    }
    int result = 0;
    if (System.updatesEnabled() || System.updatesForced()) {		// application event is handled asynchronously
        if (flags & 1) {
            // only check address
		}
		else {
            system_set_flag(SYSTEM_FLAG_OTA_UPDATE_PENDING, 0, nullptr);
            RGB.control(true);
            // Get base color used for the update process indication
            const LEDStatusData* status = led_signal_status(LED_SIGNAL_FIRMWARE_UPDATE, nullptr);
            RGB.color(status ? status->color : RGB_COLOR_MAGENTA);
            SPARK_FLASH_UPDATE = 1;
            TimingFlashUpdateTimeout = 0;
            system_notify_event(firmware_update, firmware_update_begin, &file);
            HAL_FLASH_Begin(file.file_address, file.file_length, NULL);
        }
    }
    else {
    	result = 1;
    }
    return result;
}

void system_pending_shutdown()
{
    uint8_t was_set = false;
    system_get_flag(SYSTEM_FLAG_RESET_PENDING, &was_set, nullptr);
    if (!was_set) {
        system_set_flag(SYSTEM_FLAG_RESET_PENDING, 1, nullptr);
        system_notify_event(reset_pending);
    }
}

inline bool canShutdown()
{
    return (System.resetPending() && System.resetEnabled());
}

void system_shutdown_if_enabled(void* data=nullptr)
{
    // shutdown if user initiated poweroff or system reset is allowed
    if (canShutdown())
    {
        if (SYSTEM_POWEROFF) {              // shutdown network module too.
            system_sleep(SLEEP_MODE_SOFTPOWEROFF, 0, 0, NULL);
        }
        else {
            System.reset();
        }
    }
}

void system_shutdown_if_needed()
{
    static bool in_shutdown = false;
    if (canShutdown() && !in_shutdown)
    {
        in_shutdown = true;
        system_notify_event(reset, 0, nullptr, system_shutdown_if_enabled);

#if PLATFORM_THREADING
        // timeout for 30 seconds. Keep the system thread pumping queue messages and the background task running
        system_tick_t start = millis();
        while (canShutdown() && (millis()-start)<30000)
        {
            // todo - find a more enapsulated way for the SystemThread to take care of re-entranly
            // doing work.
            spark_process();
            SystemThread.process();
        }
        in_shutdown = false;
        system_shutdown_if_enabled();
#endif
    }
}

int Spark_Finish_Firmware_Update(FileTransfer::Descriptor& file, uint32_t flags, void* module)
{
    using namespace particle::protocol;
    SPARK_FLASH_UPDATE = 0;
    TimingFlashUpdateTimeout = 0;
    //DEBUG("update finished flags=%d store=%d", flags, file.store);
    int res = 1;

    hal_module_t mod;

    if ((flags & (UpdateFlag::VALIDATE_ONLY | UpdateFlag::SUCCESS)) == (UpdateFlag::VALIDATE_ONLY | UpdateFlag::SUCCESS)) {
        res = HAL_FLASH_OTA_Validate(module ? (hal_module_t*)module : &mod, true, (module_validation_flags_t)(MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES_FULL), NULL);
        return res;
    }

    if (flags & UpdateFlag::SUCCESS) {    // update successful
        if (file.store==FileTransfer::Store::FIRMWARE)
        {
            hal_update_complete_t result = HAL_FLASH_End(module ? (hal_module_t*)module : &mod);
            system_notify_event(firmware_update, result<=HAL_UPDATE_ERROR ? firmware_update_complete : firmware_update_failed, &file);
            res = (result <= HAL_UPDATE_ERROR);

            // always restart for now
            if ((true || result==HAL_UPDATE_APPLIED_PENDING_RESTART) && !(flags & UpdateFlag::DONT_RESET))
            {
                spark_protocol_command(sp, ProtocolCommands::DISCONNECT);
                system_pending_shutdown();
            }
        }
    }
    else
    {
        system_notify_event(firmware_update, firmware_update_failed, &file);
    }

    RGB.control(false);
    return res;
}

int Spark_Save_Firmware_Chunk(FileTransfer::Descriptor& file, const uint8_t* chunk, void* reserved)
{
    TimingFlashUpdateTimeout = 0;
    int result = -1;
    system_notify_event(firmware_update, firmware_update_progress, &file);
    if (file.store==FileTransfer::Store::FIRMWARE)
    {
        result = HAL_FLASH_Update(chunk, file.chunk_address, file.chunk_size, NULL);
        LED_Toggle(LED_RGB);
    }
    return result;
}


class AppendBase {

    appender_fn fn;
    void* data;

protected:

    template<typename T> inline bool writeDirect(const T& value) {
		return fn(data, (const uint8_t*)&value, sizeof(value));
	}

public:

    AppendBase(appender_fn fn, void* data) {
        this->fn = fn; this->data = data;
    }

    bool write(const char* string) const {
        return fn(data, (const uint8_t*)string, strlen(string));
    }

    bool write(char* string) const {
        return fn(data, (const uint8_t*)string, strlen(string));
    }

    bool write(char c) {
        return writeDirect(c);
    }
};


class AppendData : public AppendBase {
public:
	AppendData(appender_fn fn, void* data) : AppendBase(fn, data) {}

    bool write(uint16_t value) {
    		return writeDirect(value);
    }

    bool write(int32_t value) {
    		return writeDirect(value);
    }

};


class AppendJson : public AppendBase
{
	using super = AppendBase;
public:
	AppendJson(appender_fn fn, void* data) : AppendBase(fn, data) {}

    bool write_quoted(const char* value) {
        return write('"') &&
               write(value) &&
               write('"');
    }

    bool write_attribute(const char* name) {
        return
                write_quoted(name) &&
                write(':');
    }

    bool write_string(const char* name, const char* value) {
        return write_attribute(name) &&
               write_quoted(value) &&
               next();
    }

    bool newline() { return true; /*return write("\r\n");*/ }

    bool write_value(const char* name, int value) {
        return write_attribute(name) &&
               write(value) &&
               next();
    }

    bool end_list() {
        return write_attribute("_") &&
               write_quoted("");
    }

    bool write(int value) {
        char buf[12];
        return write(itoa(value, buf, 10));
    }

    inline bool write(char c) {
    		return super::write(c);
    }

    inline bool write(const char* c) {
    		return super::write(c);
    }


    bool next() { return write(',') && newline(); }

    bool write_key_values(size_t count, const key_value* key_values)
    {
        bool result = true;
        while (count-->0) {
            result = result && write_key_value(key_values++);
        }
        return result;
    }

    bool write_key_value(const key_value* kv)
    {
        return write_string(kv->key, kv->value);
    }
};


const char* module_function_string(module_function_t func) {
    switch (func) {
        case MODULE_FUNCTION_NONE: return "n";
        case MODULE_FUNCTION_RESOURCE: return "r";
        case MODULE_FUNCTION_BOOTLOADER: return "b";
        case MODULE_FUNCTION_MONO_FIRMWARE: return "m";
        case MODULE_FUNCTION_SYSTEM_PART: return "s";
        case MODULE_FUNCTION_USER_PART: return "u";
        case MODULE_FUNCTION_NCP_FIRMWARE: return "c";
        default: return "_";
    }
}

const char* module_store_string(module_store_t store) {
    switch (store) {
        case MODULE_STORE_MAIN: return "m";
        case MODULE_STORE_BACKUP: return "b";
        case MODULE_STORE_FACTORY: return "f";
        case MODULE_STORE_SCRATCHPAD: return "t";
        default: return "_";
    }
}

bool is_module_function_valid(module_function_t func) {
    switch (func) {
        case MODULE_FUNCTION_RESOURCE:
        case MODULE_FUNCTION_BOOTLOADER:
        case MODULE_FUNCTION_MONO_FIRMWARE:
        case MODULE_FUNCTION_SYSTEM_PART:
        case MODULE_FUNCTION_USER_PART:
        case MODULE_FUNCTION_NCP_FIRMWARE: {
            return true;
        }
        case MODULE_FUNCTION_NONE:
        default: {
            return false;
        }
    }
}

const char* module_name(uint8_t index, char* buf)
{
    return itoa(index, buf, 10);
}

bool module_info_to_json(appender_fn append, void* append_data, const hal_module_t* module, uint32_t flags)
{
    AppendJson json(append, append_data);
    char buf[65];
    bool result = true;
    const module_info_t* info = module->info;

    buf[64] = 0;
    bool output_uuid = module->suffix && module_function(info)==MODULE_FUNCTION_USER_PART;
    result &= json.write('{') && json.write_value("s", module->bounds.maximum_size) && json.write_string("l", module_store_string(module->bounds.store))
            && json.write_value("vc",module->validity_checked) && json.write_value("vv", module->validity_result)
      && (!output_uuid || json.write_string("u", bytes2hexbuf(module->suffix->sha, 32, buf)))
      && (!info || (json.write_string("f", module_function_string(module_function(info)))
                    && json.write_string("n", module_name(module_index(info), buf))
                    && json.write_value("v", info->module_version)
                    && (!(flags & MODULE_INFO_JSON_INCLUDE_PLATFORM_ID) || json.write_value("p", info->platform_id))))
    // on the photon we have just one dependency, this will need generalizing for other platforms
      && json.write_attribute("d") && json.write('[');

    bool hasDependencies = false;
    for (unsigned int d=0; d<2; d++) {
        const module_dependency_t& dependency = d == 0 ? info->dependency : info->dependency2;
        module_function_t function = module_function_t(dependency.module_function);
        if (is_module_function_valid(function)) {
            // skip empty dependents
            hasDependencies = true;
        }
    }

    bool cont = false;
    for (unsigned int d=0; d<2 && info && hasDependencies; d++) {
        const module_dependency_t& dependency = d == 0 ? info->dependency : info->dependency2;
        module_function_t function = module_function_t(dependency.module_function);
        if (!is_module_function_valid(function)) {
            // Skip empty dependencies to save on space
            continue;
        }
        if (cont) {
            result &= json.write(',');
        }
        result &= json.write('{')
                && json.write_string("f", module_function_string(function))
                && json.write_string("n", module_name(dependency.module_index, buf))
                && json.write_value("v", dependency.module_version)
                && json.end_list() && json.write('}');
        cont = true;
    }
    result &= json.write("]}");

    return result;
}

bool system_info_to_json(appender_fn append, void* append_data, hal_system_info_t& system)
{
    AppendJson json(append, append_data);
    bool result = true;
    result &= json.write_value("p", system.platform_id)
        && json.write_key_values(system.key_value_count, system.key_values)
        && json.write_attribute("m")
        && json.write('[');

    bool cont = false;
    for (unsigned i=0; i<system.module_count; i++) {
        const hal_module_t& module = system.modules[i];
#ifdef HYBRID_BUILD
        // FIXME: skip, otherwise we overflow MBEDTLS_SSL_MAX_CONTENT_LEN
        if (module.info->module_function == MODULE_FUNCTION_MONO_FIRMWARE) {
            continue;
        }
#endif // HYBRID_BUILD
        if (!module.info || !is_module_function_valid((module_function_t)module.info->module_function)) {
            // Skip modules that do not contain binary at all, otherwise we easily overflow
            // system describe message
            continue;
        }
        if (cont) {
            result &= json.write(',');
        }
        result &= module_info_to_json(append, append_data, &module, 0);
        cont = true;
    }

    result &= json.write(']');
    return result;
}

bool ota_update_info(appender_fn append, void* append_data, void* mod, bool full, void* reserved)
{
    bool result = true;
    AppendJson json(append, append_data);
    //result &= json.write('{');
    result &= json.write_attribute("u");
    result &= module_info_to_json(append, append_data, (hal_module_t*)mod, MODULE_INFO_JSON_INCLUDE_PLATFORM_ID);
    if (full) {
        result &= json.write(",");
        result &= json.write_attribute("s");
        result &= json.write('{');
        result &= system_module_info(append, append_data, NULL);
        result &= json.write('}');
    }
    //result &= json.write('}');
    return result;
}

bool system_module_info(appender_fn append, void* append_data, void* reserved)
{
    hal_system_info_t info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    info.flags = HAL_SYSTEM_INFO_FLAGS_CLOUD;
    HAL_System_Info(&info, true, NULL);
    bool result = system_info_to_json(append, append_data, info);
    HAL_System_Info(&info, false, NULL);
    return result;
}

bool append_system_version_info(Appender* appender)
{
    bool result = appender->append("system firmware version: " stringify(SYSTEM_VERSION_STRING)
#if  defined(SYSTEM_MINIMAL)
" minimal"
#endif
    "\n");

    return result;
}

namespace {

using namespace particle;

template <typename T>
class AbstractDiagnosticsFormatter {


protected:

	inline T& formatter() {
		return formatter(this);
	}

	static inline T& formatter(void* fmt) {
		return *reinterpret_cast<T*>(fmt);
	}

	static int formatSourceData(const diag_source* src, void* fmt) {
		return formatter(fmt).formatSource(src);
	}

	int formatSource(const diag_source* src) {
		T& fmt = formatter();
		if (!fmt.isSourceOk(src)) {
			return 0;
		}
	    switch (src->type) {
	    case DIAG_TYPE_INT: {
	        AbstractIntegerDiagnosticData::IntType val = 0;
	        const int ret = AbstractIntegerDiagnosticData::get(src, val);
	        if ((ret == 0 && !fmt.formatSourceInt(src, val)) || (ret != 0 && !fmt.formatSourceError(src, ret))) {
	            return SYSTEM_ERROR_TOO_LARGE;
	        }
	        break;
	    }
	    default:
	        return SYSTEM_ERROR_NOT_SUPPORTED;
	    }
	    return 0;
	}

	static int formatSources(T& formatter, const uint16_t* id, size_t count, unsigned flags) {
	    if (!formatter.openDocument()) {
			return SYSTEM_ERROR_TOO_LARGE;
	    }
		if (id) {
			// Dump specified data sources
			for (size_t i = 0; i < count; ++i) {
				const diag_source* src = nullptr;
				int ret = diag_get_source(id[i], &src, nullptr);
				if (ret != 0) {
					return ret;
				}
				ret = formatter.formatSource(src);
				if (ret != 0) {
					return ret;
				}
			}
		} else {
			// Dump all data sources
			const int ret = diag_enum_sources(formatSourceData, nullptr, &formatter, nullptr);
			if (ret != 0) {
				return ret;
			}
		}
		if (!formatter.closeDocument()) {
			return SYSTEM_ERROR_TOO_LARGE;
		}
		return 0;
	}

public:

	int format(const uint16_t* id, size_t count, unsigned flags) {
		return formatSources(formatter(this), id, count, flags);
	}
};


class JsonDiagnosticsFormatter : public AbstractDiagnosticsFormatter<JsonDiagnosticsFormatter> {

	AppendJson& json;

public:
	JsonDiagnosticsFormatter(AppendJson& appender_) : json(appender_) {}

	inline bool openDocument() {
	    return json.write('{');
	}

	inline bool closeDocument() {
		return json.end_list() && json.write('}'); // TODO: Use spark::JSONWriter
	}

	bool formatSourceError(const diag_source* src, int error) {
	    return json.write_attribute(src->name) &&
	            json.write('{') &&
	            json.write_attribute("err") &&
	            json.write(error) &&
	            json.write('}') &&
	            json.next();
	}

	inline bool isSourceOk(const diag_source* src) {
	    return (src->name);
	}

	inline bool formatSourceInt(const diag_source* src, AbstractIntegerDiagnosticData::IntType val) {
		return json.write_value(src->name, val);
	}
};


class BinaryDiagnosticsFormatter : public AbstractDiagnosticsFormatter<BinaryDiagnosticsFormatter> {

	AppendData& data;

	using value = AbstractIntegerDiagnosticData::IntType;
	using id = typeof(diag_source::id);

public:
	BinaryDiagnosticsFormatter(AppendData& appender_) : data(appender_) {}


	inline bool openDocument() {
		return data.write(uint16_t(sizeof(id))) && data.write(uint16_t(sizeof(value)));
	}

	inline bool closeDocument() {
		return true;
	}

	/**
	 *
	 */
	bool formatSourceError(const diag_source* src, int error) {
		static_assert(sizeof(src->id)==2, "expected diagnostic id to be 16-bits");
		return data.write(decltype(src->id)(src->id | 1<<15)) && data.write(int32_t(error));
	}

	inline bool isSourceOk(const diag_source* src) {
	    return true;
	}

	inline bool formatSourceInt(const diag_source* src, AbstractIntegerDiagnosticData::IntType val) {
		return data.write(src->id) && data.write(val);
	}

};


} // namespace



int system_format_diag_data(const uint16_t* id, size_t count, unsigned flags, appender_fn append, void* append_data,
        void* reserved) {
	if (flags & 1) {
		AppendData data(append, append_data);
		BinaryDiagnosticsFormatter fmt(data);
	    return fmt.format(id, count, flags);
	}
	else {
	    AppendJson json(append, append_data);
	    JsonDiagnosticsFormatter fmt(json);
	    return fmt.format(id, count, flags);
	}
}

bool system_metrics(appender_fn appender, void* append_data, uint32_t flags, uint32_t page, void* reserved) {
    const int ret = system_format_diag_data(nullptr, 0, flags, appender, append_data, nullptr);
    return ret == 0;
};
