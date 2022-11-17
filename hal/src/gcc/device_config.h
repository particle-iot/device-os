/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <optional>
#include <cstring>
#include <cstdint>

#include "filesystem.h"
#include "spark_protocol_functions.h"
#include "module_info.h"

namespace particle {

namespace config {

class ModuleDependencyInfo {
public:
    ModuleDependencyInfo() :
            func_(MODULE_FUNCTION_NONE),
            index_(0),
            version_(0) {
    }

    ModuleDependencyInfo& function(module_function_t func) {
        func_ = func;
        return *this;
    }

    module_function_t function() const {
        return func_;
    }

    ModuleDependencyInfo& index(uint8_t index) {
        index_ = index;
        return *this;
    }

    uint8_t index() const {
        return index_;
    }

    ModuleDependencyInfo& version(uint16_t version) {
        version_ = version;
        return *this;
    }

    uint16_t version() const {
        return version_;
    }

private:
    module_function_t func_;
    uint8_t index_;
    uint16_t version_;
};

class ModuleInfo {
public:
    using Dependencies = std::vector<ModuleDependencyInfo>;

    ModuleInfo() :
            func_(MODULE_FUNCTION_NONE),
            store_(MODULE_STORE_MAIN),
            maxSize_(0),
            index_(0),
            version_(0),
            validChecked_(0),
            validResult_(0) {
    }

    ModuleInfo& function(module_function_t func) {
        func_ = func;
        return *this;
    }

    module_function_t function() const {
        return func_;
    }

    ModuleInfo& index(uint8_t index) {
        index_ = index;
        return *this;
    }

    uint8_t index() const {
        return index_;
    }

    ModuleInfo& version(uint16_t version) {
        version_ = version;
        return *this;
    }

    uint16_t version() const {
        return version_;
    }

    ModuleInfo& dependencies(const Dependencies& deps) {
        deps_ = deps;
        return *this;
    }

    const Dependencies& dependencies() const {
        return deps_;
    }

    ModuleInfo& store(module_store_t store) {
        store_ = store;
        return *this;
    }

    module_store_t store() const {
        return store_;
    }

    ModuleInfo& maximumSize(uint32_t size) {
        maxSize_ = size;
        return *this;
    }

    uint32_t maximumSize() const {
        return maxSize_;
    }

    ModuleInfo& validityChecked(uint16_t flags) {
        validChecked_ = flags;
        return *this;
    }

    uint16_t validityChecked() const {
        return validChecked_;
    }

    ModuleInfo& validityResult(uint16_t flags) {
        validResult_ = flags;
        return *this;
    }

    uint16_t validityResult() const {
        return validResult_;
    }

    // Expects a binary string
    ModuleInfo& hash(const std::string& hash) {
        hash_ = hash;
        return *this;
    }

    // Returns a binary string
    const std::string& hash() const {
        return hash_;
    }

private:
    Dependencies deps_;
    std::string hash_;
    module_function_t func_;
    module_store_t store_;
    uint32_t maxSize_;
    uint8_t index_;
    uint16_t version_;
    uint16_t validChecked_;
    uint16_t validResult_;
};

class Describe {
public:
    using Modules = std::vector<ModuleInfo>;

    Describe() = default;

    Describe& platformId(uint16_t platformId) {
        platformId_ = platformId;
        return *this;
    }

    uint16_t platformId() const {
        return platformId_.value_or(0);
    }

    Describe& modules(const Modules& modules) {
        modules_ = modules;
        return *this;
    }

    const Modules& modules() const {
        return modules_;
    }

    Describe& iccid(std::string iccid) {
        iccid_ = std::move(iccid);
        return *this;
    }

    std::string_view iccid() const {
        return iccid_.has_value() ? iccid_.value() : std::string_view();
    }

    bool has_iccid() const {
        return iccid_.has_value();
    }

    bool isValid() const {
        return platformId_.has_value();
    }

    int systemModuleVersion() const;

    std::string toString() const;
    static Describe fromString(const std::string& str);

private:
    Modules modules_;
    std::optional<std::string> iccid_;
    std::optional<uint16_t> platformId_;
};

} // namespace config

} // namespace particle

/**
 * Reads the device configuration and returns true if the device should start.
 * @param argc
 * @param argv
 * @return
 */
bool read_device_config(int argc, char* argv[]);

/**
 * The external configuration data.
 */
struct Configuration
{
    std::string device_id;
    std::string device_key;
    std::string server_key;
    std::string describe;
    uint16_t log_level;
    ProtocolFactory protocol;
    uint16_t platform_id;
    uint16_t product_version;
};

/**
 * The device configuration in internal form.
 */
struct DeviceConfig
{
    std::vector<std::string> argv;
    particle::config::Describe describe;
    uint8_t device_id[12];
    uint8_t device_key[1024];
    uint8_t server_key[1024];
    ProtocolFactory protocol;
    uint16_t platform_id;
    uint16_t product_version;

    void read(Configuration& configuration);

    size_t fetchDeviceID(uint8_t* dest, size_t destLen)
    {
        if (destLen>12)
            destLen = 12;
        if (dest)
            memcpy(dest, device_id, destLen);
        return 12;
    }

    ProtocolFactory get_protocol() { return protocol; }
};

extern DeviceConfig deviceConfig;
