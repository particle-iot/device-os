/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "system_info.h"
#include "system_cloud_internal.h"
#include "check.h"
#include "scope_guard.h"
#include "bytes2hexbuf.h"
#include "spark_wiring_json.h"
#include "spark_wiring_diagnostics.h"
#include "spark_macros.h"
#include <cstdio>
#include <climits>

#include "control/common.h"
#if HAL_PLATFORM_PROTOBUF
#include "cloud/describe.pb.h"
using particle::control::common::EncodedString;
#endif // HAL_PLATFORM_PROTOBUF

#if HAL_PLATFORM_ASSETS
#include "asset_manager.h"
#endif // HAL_PLATFORM_ASSETS

#define PB(name) particle_cloud_##name

namespace {

using namespace particle;
using namespace particle::system;

class AppendBase {
protected:
    appender_fn fn_;
    void* data_;

    template<typename T> inline bool writeDirect(const T& value) {
        return fn_(data_, (const uint8_t*)&value, sizeof(value));
    }

public:

    AppendBase(appender_fn fn, void* data) {
        this->fn_ = fn; this->data_ = data;
    }

    bool write(const char* string) const {
        return fn_(data_, (const uint8_t*)string, strlen(string));
    }

    bool write(char* string) const {
        return fn_(data_, (const uint8_t*)string, strlen(string));
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

    bool write(uint32_t value) {
        return writeDirect(value);
    }

};


constexpr size_t APPENDJSON_PRINTF_BUFFER_SIZE = 64;

class AppendJson: public spark::JSONWriter, public AppendBase {
public:
    AppendJson(appender_fn fn, void* data)
            : AppendBase(fn, data),
              state_(true) {
    }

    bool isOk() const {
        return state_;
    }

protected:

    virtual void write(const char *data, size_t size) override {
        state_ = state_ && fn_(data_, (const uint8_t*)data, size);
    }
    virtual void printf(const char *fmt, ...) override {
        char buf[APPENDJSON_PRINTF_BUFFER_SIZE] = {};
        va_list args;
        va_start(args, fmt);
        const auto r = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (r >= (int)sizeof(buf)) {
            // Truncated
            state_ = false;
            return;
        }
        write(buf, r);
    }
private:
    bool state_;
};

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
		case DIAG_TYPE_UINT: {
			AbstractUnsignedIntegerDiagnosticData::IntType val = 0;
			const int ret = AbstractUnsignedIntegerDiagnosticData::get(src, val);
			if ((ret == 0 && !fmt.formatSourceUnsignedInt(src, val)) || (ret != 0 && !fmt.formatSourceError(src, ret))) {
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
		json.beginObject();
		return json.isOk();
	}

	inline bool closeDocument() {
		json.endObject();
		return json.isOk();
	}

	bool formatSourceError(const diag_source* src, int error) {
		json.name(src->name);
		json.beginObject();
		json.name("err").value(error);
		json.endObject();
		return json.isOk();
	}

	inline bool isSourceOk(const diag_source* src) {
	    return (src->name);
	}

	inline bool formatSourceInt(const diag_source* src, AbstractIntegerDiagnosticData::IntType val) {
		json.name(src->name).value(val);
		return json.isOk();
	}

	inline bool formatSourceUnsignedInt(const diag_source* src, AbstractUnsignedIntegerDiagnosticData::IntType val) {
		json.name(src->name).value(val);
		return json.isOk();
	}
};


class BinaryDiagnosticsFormatter : public AbstractDiagnosticsFormatter<BinaryDiagnosticsFormatter> {

	AppendData& data;

	// FIXME: single size for all the diagnostics is just plain wrong
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

	inline bool formatSourceUnsignedInt(const diag_source* src, AbstractUnsignedIntegerDiagnosticData::IntType val) {
		return data.write(src->id) && data.write(val);
	}

};

#if HAL_PLATFORM_PROTOBUF
class PbAppenderStream: public pb_ostream_t {
public:
    PbAppenderStream(appender_fn append, void* ctx) :
            pb_ostream_t(),
            append_(append),
            ctx_(ctx) {
        pb_ostream_t::state = this;
        pb_ostream_t::max_size = SIZE_MAX;
        pb_ostream_t::callback = [](pb_ostream_t* strm, const pb_byte_t* buf, size_t size) {
            auto self = (PbAppenderStream*)strm->state;
            return self->append_(self->ctx_, buf, size);
        };
    }

private:
    appender_fn append_;
    void* ctx_;
};

PB(FirmwareModuleType) moduleFunctionToPb(module_function_t func) {
    switch (func) {
    case MODULE_FUNCTION_RESOURCE:
        return PB(FirmwareModuleType_RESOURCE_MODULE);
    case MODULE_FUNCTION_BOOTLOADER:
        return PB(FirmwareModuleType_BOOTLOADER_MODULE);
    case MODULE_FUNCTION_MONO_FIRMWARE:
        return PB(FirmwareModuleType_MONO_FIRMWARE_MODULE);
    case MODULE_FUNCTION_SYSTEM_PART:
        return PB(FirmwareModuleType_SYSTEM_PART_MODULE);
    case MODULE_FUNCTION_USER_PART:
        return PB(FirmwareModuleType_USER_PART_MODULE);
    case MODULE_FUNCTION_SETTINGS:
        return PB(FirmwareModuleType_SETTINGS_MODULE);
    case MODULE_FUNCTION_NCP_FIRMWARE:
        return PB(FirmwareModuleType_NCP_FIRMWARE_MODULE);
    case MODULE_FUNCTION_RADIO_STACK:
        return PB(FirmwareModuleType_RADIO_STACK_MODULE);
    case MODULE_FUNCTION_ASSET:
        return PB(FirmwareModuleType_ASSET_MODULE);
    default:
        return PB(FirmwareModuleType_INVALID_MODULE);
    }
}

PB(FirmwareModuleStore) moduleStoreToPb(module_store_t store) {
    switch (store) {
    case MODULE_STORE_FACTORY:
        return PB(FirmwareModuleStore_FACTORY_MODULE_STORE);
    case MODULE_STORE_BACKUP:
        return PB(FirmwareModuleStore_BACKUP_MODULE_STORE);
    case MODULE_STORE_SCRATCHPAD:
        return PB(FirmwareModuleStore_SCRATCHPAD_MODULE_STORE);
    case MODULE_STORE_MAIN:
    default:
        return PB(FirmwareModuleStore_MAIN_MODULE_STORE);
    }
}

#endif // HAL_PLATFORM_PROTOBUF

#if HAL_PLATFORM_ASSETS

bool encodeAssetDependencies(pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
    auto assets = (const spark::Vector<Asset>*)*arg;
    for (const auto& asset: *assets) {
        PB(FirmwareModuleAsset) pbModuleAsset = {};
        EncodedString pbName(&pbModuleAsset.name);
        EncodedString pbHash(&pbModuleAsset.hash);
        // TODO: hash type
        pbName.data = asset.name().c_str();
        pbName.size = asset.name().length();

        pbHash.data = asset.hash().hash().data();
        pbHash.size = asset.hash().hash().size();
        if (!pb_encode_tag_for_field(strm, field) || !pb_encode_submessage(strm, &PB(FirmwareModuleAsset_msg), &pbModuleAsset)) {
            return false;
        }
    }
    return true;
}

#endif // HAL_PLATFORM_ASSETS

} // anonymous

bool module_info_to_json(AppendJson& json, const hal_module_t* module, uint32_t flags)
{
    const module_info_t& info = module->info;

    bool output_uuid = (module_function(&info) == MODULE_FUNCTION_USER_PART);
    json.beginObject();
    json.name("s").value(module->bounds.maximum_size);
    json.name("l").value(module_store_string(module->bounds.store));
    json.name("vc").value(module->validity_checked);
    json.name("vv").value(module->validity_result);
    if (output_uuid) {
        char buf[sizeof(module_info_suffix_t::sha) * 2] = {};
        json.name("u").value(bytes2hexbuf(module->suffix.sha, sizeof(module->suffix.sha), buf), sizeof(buf));
    }
    json.name("f").value(module_function_string(module_function(&info)));
    char tmp[4] = {};
    snprintf(tmp, sizeof(tmp), "%u", (unsigned)module_index(&info));
    // NOTE: yes, module index is supposed to be a string, not a Number
    json.name("n").value(tmp);
    json.name("v").value(module_version(&info));
    if (flags & MODULE_INFO_JSON_INCLUDE_PLATFORM_ID) {
        json.name("p", module_platform_id(&info));
    }
    json.name("d");
    json.beginArray();
    for (unsigned d = 0; d < 2; d++) {
        const module_dependency_t& dependency = d == 0 ? info.dependency : info.dependency2;
        module_function_t function = module_function_t(dependency.module_function);
        if (is_module_function_valid(function)) {
            json.beginObject();
            json.name("f").value(module_function_string(function));
            snprintf(tmp, sizeof(tmp), "%u", (unsigned)dependency.module_index);
            // FIXME: is this really supposed to be a string?
            // json.name("n").value(dependency.module_index);
            json.name("n").value(tmp);
            json.name("v").value(dependency.module_version);
            json.endObject();
        }
    }
    json.endArray();
    json.endObject();
    return json.isOk();
}

bool system_info_to_json(appender_fn append, void* append_data, hal_system_info_t& system)
{
    AppendJson json(append, append_data);
    json.name("p").value(system.platform_id);
    for (unsigned i = 0; i < system.key_value_count; i++) {
        json.name(system.key_values[i].key).value(system.key_values[i].value);
    }
    // Modules
    json.name("m");
    json.beginArray();
    for (unsigned i=0; i<system.module_count; i++) {
        const hal_module_t& module = system.modules[i];
        if (!is_module_function_valid((module_function_t)module.info.module_function)) {
            // Skip modules that do not contain binary at all, otherwise we easily overflow
            // system describe message
            continue;
        }
        if (module.bounds.store == MODULE_STORE_FACTORY && (module.validity_result & MODULE_VALIDATION_INTEGRITY) == 0) {
            // Specifically skip factory modules that do not look valid
            continue;
        }
        module_info_to_json(json, &module, 0);
    }
    json.endArray();
    return json.isOk();
}

bool system_module_info(appender_fn append, void* append_data, void* reserved)
{
    hal_system_info_t info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    info.flags = HAL_SYSTEM_INFO_FLAGS_CLOUD;
    system_info_get_unstable(&info, 0, nullptr);
    bool result = system_info_to_json(append, append_data, info);
    system_info_free_unstable(&info, nullptr);
    return result;
}

int system_info_get_unstable(hal_system_info_t* info, uint32_t flags, void* reserved) {
    CHECK_TRUE(info, SYSTEM_ERROR_INVALID_ARGUMENT);
    return HAL_System_Info(info, true, nullptr);
}

int system_info_free_unstable(hal_system_info_t* info, void* reserved) {
    CHECK_TRUE(info, SYSTEM_ERROR_INVALID_ARGUMENT);
    return HAL_System_Info(info, false, nullptr);
}

bool append_system_version_info(Appender* appender)
{
    bool result = appender->append("system firmware version: " PP_STR(SYSTEM_VERSION_STRING)
#if  defined(SYSTEM_MINIMAL)
" minimal"
#endif
    "\n");

    return result;
}

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
}

bool system_app_info(appender_fn appender, void* append_data, void* reserved) {
	AppendJson json(appender, append_data);
	json.name("f").beginArray();
	const auto fnCount = cloudFunctionCount();
	for (size_t i = 0; i < fnCount; ++i) {
		const char* name = nullptr;
		if (getCloudFunctionInfo(i, &name) != 0) {
			return false;
		}
		json.value(name);
	}
	json.endArray();
	json.name("v").beginObject();
	const auto varCount = cloudVariableCount();
	for (size_t i = 0; i < varCount; ++i) {
		const char* name = nullptr;
		int type = 0;
		if (getCloudVariableInfo(i, &name, &type) != 0) {
			return false;
		}
		json.name(name).value(type);
	}
	json.endObject();
	return true;
}

#if HAL_PLATFORM_PROTOBUF
bool system_module_info_pb(appender_fn appender, void* append_data, void* reserved) {
    hal_system_info_t sysInfo = {};
    sysInfo.size = sizeof(sysInfo);
    sysInfo.flags = HAL_SYSTEM_INFO_FLAGS_CLOUD;
    int r = system_info_get_unstable(&sysInfo, 0, nullptr);
    if (r != 0) {
        return false;
    }
    SCOPE_GUARD({
        system_info_free_unstable(&sysInfo, nullptr);
    });
    PB(SystemDescribe) pbDesc = {};
    // IMEI, ICCID, modem firmware version
    EncodedString pbImei(&pbDesc.imei);
    EncodedString pbIccid(&pbDesc.iccid);
    EncodedString pbModemFwVer(&pbDesc.modem_firmware_version);
    for (unsigned i = 0; i < sysInfo.key_value_count; ++i) {
        const auto& keyVal = sysInfo.key_values[i];
        if (strcmp(keyVal.key, "imei") == 0) {
            pbImei.data = keyVal.value;
            pbImei.size = strlen(keyVal.value);
        } else if (strcmp(keyVal.key, "iccid") == 0) {
            pbIccid.data = keyVal.value;
            pbIccid.size = strlen(keyVal.value);
        } else if (strcmp(keyVal.key, "cellfw") == 0) {
            pbModemFwVer.data = keyVal.value;
            pbModemFwVer.size = strlen(keyVal.value);
        }
    }
    // Firmware modules
    pbDesc.firmware_modules.arg = &sysInfo;
    pbDesc.firmware_modules.funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
        auto sysInfo = (const hal_system_info_t*)*arg;
        for (unsigned i = 0; i < sysInfo->module_count; ++i) {
            const auto& module = sysInfo->modules[i];
            if (!is_module_function_valid((module_function_t)module.info.module_function)) {
                continue;
            }
            if (module.bounds.store == MODULE_STORE_FACTORY && (module.validity_result & MODULE_VALIDATION_INTEGRITY) == 0) {
                continue; // See system_info_to_json()
            }
            PB(FirmwareModule) pbModule = {};
            pbModule.type = moduleFunctionToPb((module_function_t)module.info.module_function);
            pbModule.index = module.info.module_index;
            pbModule.version = module.info.module_version;
            pbModule.store = moduleStoreToPb((module_store_t)module.bounds.store);
            pbModule.max_size = module.bounds.maximum_size;
            pbModule.checked_flags = module.validity_checked;
            pbModule.passed_flags = module.validity_result;
            EncodedString pbHash(&pbModule.hash);
            if (module.info.module_function == MODULE_FUNCTION_USER_PART) {
                pbHash.data = (const char*)module.suffix.sha;
                pbHash.size = sizeof(module.suffix.sha);
            }
            pbModule.dependencies.arg = (void*)&module; // arg is a non-const pointer
            pbModule.dependencies.funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
                auto module = (const hal_module_t*)*arg;
                for (unsigned i = 0; i < 2; ++i) {
                    const auto& dep = (i == 0) ? module->info.dependency : module->info.dependency2;
                    PB(FirmwareModuleDependency) pbDep = {};
                    if (!is_module_function_valid((module_function_t)dep.module_function)) {
                        continue;
                    }
                    pbDep.type = moduleFunctionToPb((module_function_t)dep.module_function);
                    pbDep.index = dep.module_index;
                    pbDep.version = dep.module_version;
                    if (!pb_encode_tag_for_field(strm, field) || !pb_encode_submessage(strm, &PB(FirmwareModuleDependency_msg), &pbDep)) {
                        return false;
                    }
                }
                return true;
            };
#if HAL_PLATFORM_ASSETS
            spark::Vector<Asset> assets;
            if ((module.info.module_function == MODULE_FUNCTION_USER_PART) && (module.validity_result & MODULE_VALIDATION_INTEGRITY)) {
                if (!AssetManager::requiredAssetsForModule(&module, assets) && assets.size() > 0) {
                    pbModule.asset_dependencies.arg = (void*)&assets;
                    pbModule.asset_dependencies.funcs.encode = encodeAssetDependencies;
                }
            }
#endif // HAL_PLATFORM_ASSETS
            if (!pb_encode_tag_for_field(strm, field) || !pb_encode_submessage(strm, &PB(FirmwareModule_msg), &pbModule)) {
                return false;
            }
        }
        return true;
    };
#if HAL_PLATFORM_ASSETS
    auto availableAssets = AssetManager::instance().availableAssets();
    if (availableAssets.size() > 0) {
        pbDesc.assets.arg = (void*)&availableAssets;
        pbDesc.assets.funcs.encode = encodeAssetDependencies;
    }
#endif // HAL_PLATFORM_ASSETS
    PbAppenderStream strm(appender, append_data);
    return pb_encode(&strm, &PB(SystemDescribe_msg), &pbDesc);
}

#endif // HAL_PLATFORM_PROTOBUF


