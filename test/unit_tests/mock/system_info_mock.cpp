/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "system_info_mock.h"
#include <algorithm>

namespace particle {

namespace test {

namespace {

} // namespace

SystemInfo::SystemInfo(MockRepository* mocks) :
        mocks_(mocks),
        platformId_(0) {
    mocks->OnCallFunc(system_info_get_unstable).Do([this](hal_system_info_t* info, uint32_t flags, void* reserved) {
        return this->info_get(info, flags, reserved);
    });
    mocks->OnCallFunc(system_info_free_unstable).Do([this](hal_system_info_t* info, void* reserved) {
        return this->info_free(info, reserved);
    });
}

SystemInfo::~SystemInfo() noexcept(false) {
}

void SystemInfo::addModule(module_function_t function, module_store_t store, bool present) {
    auto module = genRandomModule(function, store);
    addModule(module, present);
}

void SystemInfo::addModule(hal_module_t module, bool present) {
    // Use as a marker
    module.bounds.mcu_identifier = present;
    moduleStore_.push_back(module);
}

void SystemInfo::addModuleThatShouldNotBePresentInJson(module_function_t function, module_store_t store) {
    addModule(function, store, false);
}

void SystemInfo::addModuleThatShouldNotBePresentInJson(hal_module_t module) {
    addModule(module, false);
}

void SystemInfo::setPlatformId(int platformId) {
    platformId_ = platformId;
}

void SystemInfo::addKeyValue(const std::string& key, const std::string& value) {
    keyValues_.push_back(KeyValue(key, value));
}

void SystemInfo::addKeyValue() {
    addKeyValue(randString(32), randString(sizeof(KeyValue::value) - 1));
}

void SystemInfo::reset() {
    moduleStore_.clear();
    keyValues_.clear();
}

int SystemInfo::info_get(hal_system_info_t* info, uint32_t flags, void* reserved) {
    info->module_count = moduleStore_.size();
    if (moduleStore_.size() > 0) {
        info->modules = moduleStore_.data();
    } else {
        info->modules = nullptr;
    }
    info->key_value_count = keyValues_.size();
    if (keyValues_.size() > 0) {
        info->key_values = keyValues_.data();
    } else {
        info->key_values = nullptr;
    }
    info->platform_id = platformId_;
    return 0;
}

int SystemInfo::info_free(hal_system_info_t* info, void* reserved) {
    return 0;
}

} // namespace test

} // namespace particle
