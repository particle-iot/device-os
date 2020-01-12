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

#include "io_expander.h"

using namespace particle;

IoExpanderPinObj::IoExpanderPinObj()
        : instance_(nullptr),
          port_(PIN_INVALID),
          pin_(PIN_INVALID),
          configured_(false) {
}

IoExpanderPinObj::IoExpanderPinObj(IoExpanderBase& instance, IoExpanderPort port, IoExpanderPin pin) {
    instance_ = &instance;
    port_ = static_cast<uint8_t>(port);
    pin_ = static_cast<uint8_t>(pin);
    configured_ = true;
}

IoExpanderPinObj::~IoExpanderPinObj() {
    if (configured_) {
        mode(IoExpanderPinMode::INPUT);
    }
}

int IoExpanderPinObj::mode(IoExpanderPinMode mode) const {
    CHECK_TRUE(configured_, SYSTEM_ERROR_INVALID_STATE);
    return instance_->setPinMode(port_, pin_, mode);
}

int IoExpanderPinObj::outputDrive(IoExpanderPinDrive drive) const {
    CHECK_TRUE(configured_, SYSTEM_ERROR_INVALID_STATE);
    return instance_->setPinOutputDrive(port_, pin_, drive);
}

int IoExpanderPinObj::inputInverted(bool enable) const {
    CHECK_TRUE(configured_, SYSTEM_ERROR_INVALID_STATE);
    return instance_->setPinInputInverted(port_, pin_, enable);
}

int IoExpanderPinObj::inputLatch(bool enable) const {
    CHECK_TRUE(configured_, SYSTEM_ERROR_INVALID_STATE);
    return instance_->setPinInputLatch(port_, pin_, enable);
}

int IoExpanderPinObj::write(IoExpanderPinValue value) const {
    CHECK_TRUE(configured_, SYSTEM_ERROR_INVALID_STATE);
    return instance_->writePinValue(port_, pin_, value);
}

int IoExpanderPinObj::read(IoExpanderPinValue& value) const {
    CHECK_TRUE(configured_, SYSTEM_ERROR_INVALID_STATE);
    return instance_->readPinValue(port_, pin_, value);
}

int IoExpanderPinObj::attachInterrupt(IoExpanderIntTrigger trig, IoExpanderOnInterruptCallback callback) const {
    CHECK_TRUE(configured_, SYSTEM_ERROR_INVALID_STATE);
    return instance_->attachPinInterrupt(port_, pin_, trig, callback);
}
