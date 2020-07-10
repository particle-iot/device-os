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

#include "firmware_update.h"

#include "scope_guard.h"
#include "check.h"
#include "debug.h"

#include <cstdio>
#include <cstdarg>

namespace particle::system {

namespace {

} // namespace

FirmwareUpdate::FirmwareUpdate() :
        validResult_(0),
        validChecked_(false),
        updating_(false) {
    SPARK_ASSERT(init() == 0);
}

FirmwareUpdate::~FirmwareUpdate() {
    destroy();
}

int FirmwareUpdate::startUpdate(size_t fileSize, const char* fileHash, size_t* fileOffset, FirmwareUpdateFlags flags) {
    if ((flags & FirmwareUpdateFlag::NON_RESUMABLE) && (!fileHash || !fileOffset)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (updating_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return 0;
}

int FirmwareUpdate::validateUpdate() {
    return 0;
}

int FirmwareUpdate::finishUpdate(FirmwareUpdateFlags flags) {
    return 0;
}

int FirmwareUpdate::saveChunk(const char* chunkData, size_t chunkSize, size_t chunkOffset, size_t partialSize) {
    return 0;
}

bool FirmwareUpdate::isInProgress() const {
    return false;
}

void FirmwareUpdate::reset() {
    errMsg_ = CString();
}

FirmwareUpdate* FirmwareUpdate::instance() {
    static FirmwareUpdate instance;
    return &instance;
}

int FirmwareUpdate::init() {
    return 0;
}

void FirmwareUpdate::destroy() {
}

void FirmwareUpdate::errorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args2;
    va_copy(args2, args);
    SCOPE_GUARD({
        va_end(args2);
        va_end(args);
    });
    const auto n = vsnprintf(nullptr, 0, fmt, args);
    if (n < 0) {
        return;
    }
    const auto buf = (char*)malloc(n + 1);
    if (!buf) {
        return;
    }
    vsnprintf(buf, n + 1, fmt, args2);
    errMsg_ = CString::wrap(buf);
}

} // namespace particle::system
