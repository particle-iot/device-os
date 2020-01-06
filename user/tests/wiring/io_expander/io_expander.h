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

#ifndef IO_EXPANDER_H
#define IO_EXPANDER_H

#include "io_expander_impl.h"

namespace particle {

typedef void (*IoExpanderOnInterruptCallback)(void);

enum class IoExpanderPinValue : uint8_t {
    HIGH,
    LOW
};

enum class IoExpanderPinDir : uint8_t {
    INPUT,
    OUTPUT
};

enum class IoExpanderPinPull : uint8_t {
    NO_PULL,
    PULL_UP,
    PULL_DOWN
};

enum class IoExpanderIntTrigger : uint8_t {
    CHANGE,
    RISING,
    FALLING
};

struct IoExpanderPinConfig {
    IoExpanderPinConfig()
            : port(IoExpanderPort::INVALID),
              pin(IoExpanderPin::INVALID),
              dir(IoExpanderPinDir::INPUT),
              pull(IoExpanderPinPull::NO_PULL),
              drive(IoExpanderPinDrive::PERCENT100),
              inputLatch(0),
              inputInverted(0),
              intEn(0),
              trig(IoExpanderIntTrigger::CHANGE),
              callback(nullptr) {
    }
    ~IoExpanderPinConfig() {}

    IoExpanderPort port;
    IoExpanderPin pin;
    IoExpanderPinDir dir;
    IoExpanderPinPull pull;
    IoExpanderPinDrive drive;
    uint8_t inputLatch      : 1;
    uint8_t inputInverted   : 1;
    uint8_t intEn           : 1;
    IoExpanderIntTrigger trig;
    uint8_t reserved;
    IoExpanderOnInterruptCallback callback;
};
static_assert((sizeof(IoExpanderPinConfig) % 4) == 0, "Size of IoExpanderPinConfig should be 4-bytes aligned.");


class IoExpander {
public:
    int init(uint8_t addr, pin_t resetPin, pin_t intPin);
    int deinit();

    int configure(const IoExpanderPinConfig& config);

    int reset() const;

    int write(IoExpanderPort port, IoExpanderPin pin, IoExpanderPinValue val);
    int read(IoExpanderPort port, IoExpanderPin pin, IoExpanderPinValue& val);

    static IoExpander& getInstance();

private:
    IoExpander();
    ~IoExpander();

    bool initialized_;
};

#define IOExpander IoExpander::getInstance()

int io_expander_init(uint8_t addr, pin_t resetPin, pin_t intPin);
int io_expander_deinit();
int io_expander_hard_reset();
int io_expander_configure_pin(const IoExpanderPinConfig& config);
int io_expander_write_pin(IoExpanderPort port, IoExpanderPin pin, IoExpanderPinValue val);
int io_expander_read_pin(IoExpanderPort port, IoExpanderPin pin, IoExpanderPinValue& val);

} // namespace particle

#endif // IO_EXPANDER_H
