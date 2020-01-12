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

#ifndef PCAL6416A_H
#define PCAL6416A_H

#include "Particle.h"
#include "io_expander.h"

#define IO_EXPANDER_PORT_COUNT_MAX              (2)
#define IO_EXPANDER_PIN_COUNT_PER_PORT_MAX      (8)

#define IO_EXPANDER_I2C_ADDRESS                 (0x20)
#define IO_EXPANDER_RESET_PIN                   (D23)
#define IO_EXPANDER_INT_PIN                     (D22)


class Pcal6416a : public IoExpanderBase {
public:
    int begin(uint8_t address, pin_t resetPin, pin_t interruptPin);
    int end();
    int reset();
    int sleep();
    int wakeup();
    int sync();

    int setPinMode(uint8_t port, uint8_t pin, IoExpanderPinMode mode);
    int setPinOutputDrive(uint8_t port, uint8_t pin, IoExpanderPinDrive drive);
    int setPinInputInverted(uint8_t port, uint8_t pin, bool enable);
    int setPinInputLatch(uint8_t port, uint8_t pin, bool enable);
    int writePinValue(uint8_t port, uint8_t pin, IoExpanderPinValue value);
    int readPinValue(uint8_t port, uint8_t pin, IoExpanderPinValue& value);
    int attachPinInterrupt(uint8_t port, uint8_t pin, IoExpanderIntTrigger trig, IoExpanderOnInterruptCallback callback);

    static Pcal6416a& getInstance();

private:
    struct IoPinInterruptConfig {
        IoPinInterruptConfig()
                : port(PIN_INVALID),
                  pin(PIN_INVALID),
                  cb(nullptr) {
        }
        ~IoPinInterruptConfig() {}

        uint8_t port;
        uint8_t pin;
        IoExpanderIntTrigger trig;
        IoExpanderOnInterruptCallback cb;
    };

    const uint8_t INVALID_I2C_ADDRESS = 0x7F;

    // Resister address
    const uint8_t INPUT_PORT_REG[2]       = { 0x00, 0x01 };
    const uint8_t OUTPUT_PORT_REG[2]      = { 0x02, 0x03 };
    const uint8_t POLARITY_REG[2]         = { 0x04, 0x05 };
    const uint8_t DIR_REG[2]              = { 0x06, 0x07 };
    const uint8_t OUTPUT_DRIVE_REG[4]     = { 0x40, 0x41, 0x42, 0x43 };
    const uint8_t INPUT_LATCH_REG[2]      = { 0x44, 0x45 };
    const uint8_t PULL_ENABLE_REG[2]      = { 0x46, 0x47 };
    const uint8_t PULL_SELECT_REG[2]      = { 0x48, 0x49 };
    const uint8_t INT_MASK_REG[2]         = { 0x4A, 0x4B };
    const uint8_t INT_STATUS_REG[2]       = { 0x4C, 0x4D };
    const uint8_t OUTPUT_PORT_CONFIG_REG  = 0x4F;

    Pcal6416a();
    ~Pcal6416a();

    void resetRegValue();
    int writeRegister(uint8_t reg, uint8_t val);
    int readRegister(uint8_t reg, uint8_t* val);
    int configurePullAbility(uint8_t port, uint8_t pin, IoExpanderPinMode mode);
    int configureDirection(uint8_t port, uint8_t pin, IoExpanderPinMode mode);
    int configureOutputDrive(uint8_t port, uint8_t pin, IoExpanderPinDrive drive);
    int configureInputInverted(uint8_t port, uint8_t pin, bool enable);
    int configureInputLatch(uint8_t port, uint8_t pin, bool enable);
    int configureIntMask(uint8_t port, uint8_t pin, bool enable);
    int writePin(uint8_t port, uint8_t pin, IoExpanderPinValue val);
    int readPin(uint8_t port, uint8_t pin, IoExpanderPinValue& val);
    static os_thread_return_t ioInterruptHandleThread(void* param);

    uint8_t outputRegValue_[2];
    uint8_t inputInvertedRegValue_[2];
    uint8_t dirRegValue_[2];
    uint8_t outputDriveRegValue_[4];
    uint8_t inputLatchRegValue_[2];
    uint8_t pullEnableRegValue_[2];
    uint8_t pullSelectRegValue_[2];
    uint8_t intMaskRegValue_[2];
    uint8_t portPullRegValue_;

    bool initialized_;
    uint8_t address_;
    pin_t resetPin_;
    pin_t intPin_;
    os_thread_t ioExpanderWorkerThread_;
    os_queue_t ioExpanderWorkerQueue_;
    bool ioExpanderWorkerThreadExit_;
    Vector<IoPinInterruptConfig> intConfigs_;

    static RecursiveMutex mutex_;
}; // class Pcal6416a

#define PCAL6416A Pcal6416a::getInstance()


#endif // PCAL6416A_H
