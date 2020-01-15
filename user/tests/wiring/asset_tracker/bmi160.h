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

#ifndef BMI160_H
#define BMI160_H

#include "Particle.h"
#include "pcal6416a.h"

#define BMI160_I2C_ADDRESS                      (0x68)


namespace particle {

enum class Bmi160Register : uint8_t {
    CHIP_ID = 0x00,
};


class Bmi160Lock {
public:
    Bmi160Lock(RecursiveMutex& mutex)
            : locked_(false),
              mutex_(mutex) {
        lock();
    }

    Bmi160Lock(Bmi160Lock&& lock)
            : locked_(lock.locked_),
              mutex_(lock.mutex_) {
        lock.locked_ = false;
    }

    Bmi160Lock(const Bmi160Lock&) = delete;
    Bmi160Lock& operator=(const Bmi160Lock&) = delete;

    ~Bmi160Lock() {
        if (locked_) {
            unlock();
        }
    }

    void lock() {
        mutex_.lock();
        locked_ = true;
    }

    void unlock() {
        mutex_.unlock();
        locked_ = false;
    }

private:
    bool locked_;
    RecursiveMutex& mutex_;
};


class Bmi160 {
public:
    int begin(uint8_t address);
    int end();
    int sleep();
    int wakeup();

    int getChipId(uint8_t* val);

    static Bmi160& getInstance();

private:
    const uint8_t INVALID_I2C_ADDRESS = 0x7F;

    Bmi160();
    ~Bmi160();

    int writeRegister(const Bmi160Register reg, const uint8_t val);
    int readRegister(const Bmi160Register reg, uint8_t* const val);

    uint8_t address_;
    bool initialized_;
    IoExpanderPinObj pwrEnPin_;
    static RecursiveMutex mutex_;
}; // class Bmi160

#define BMI160 Bmi160::getInstance()

} // namespace particle

#endif // BMI160_H