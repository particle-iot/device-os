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

#include "check.h"
#include "system_error.h"
#include "mcp23s17.h"
#include "gpio_hal.h"
#include "delay_hal.h"

using namespace particle;

namespace {
void ioExpanderInterruptHandler(void* data) {
    auto instance = static_cast<Mcp23s17*>(data);
    instance->sync();
}
} // anonymous namespace


Mcp23s17::Mcp23s17()
        : initialized_(false),
          spi_(MCP23S17_SPI_INTERFACE),
          ioExpanderWorkerThread_(nullptr),
          ioExpanderWorkerQueue_(nullptr),
          ioExpanderWorkerThreadExit_(false) {
}

Mcp23s17::~Mcp23s17() {

}

int Mcp23s17::begin() {
    Mcp23s17Lock lock();
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);

    if (os_queue_create(&ioExpanderWorkerQueue_, 1, 1, nullptr)) {
        ioExpanderWorkerQueue_ = nullptr;
        LOG(ERROR, "os_queue_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    if (os_thread_create(&ioExpanderWorkerThread_, "IO Expander Thread", OS_THREAD_PRIORITY_NETWORK, ioInterruptHandleThread, this, 512)) {
        os_queue_destroy(ioExpanderWorkerQueue_, nullptr);
        ioExpanderWorkerQueue_ = nullptr;
        LOG(ERROR, "os_thread_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    HAL_Pin_Mode(IOE_CS, OUTPUT);
    HAL_GPIO_Write(IOE_CS, 1);
    HAL_Pin_Mode(IOE_RST, OUTPUT);
    HAL_GPIO_Write(IOE_RST, 1);
    HAL_Pin_Mode(IOE_INT, INPUT_PULLUP);

    HAL_InterruptExtraConfiguration extra = {0};
    extra.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1;
    CHECK(HAL_Interrupts_Attach(IOE_INT, ioExpanderInterruptHandler, this, FALLING, &extra));

    HAL_SPI_Init(spi_);
    HAL_SPI_Set_Bit_Order(spi_, MSBFIRST);
    HAL_SPI_Set_Data_Mode(spi_, SPI_MODE0);
    HAL_SPI_Set_Clock_Divider(spi_, SPI_CLOCK_DIV2);
    HAL_SPI_Begin(spi_, PIN_INVALID);

    initialized_ = true;
    CHECK(reset());

    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::end() {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);

    ioExpanderWorkerThreadExit_ = true;
    os_thread_join(ioExpanderWorkerThread_);
    os_thread_cleanup(ioExpanderWorkerThread_);
    os_queue_destroy(ioExpanderWorkerQueue_, nullptr);
    ioExpanderWorkerThreadExit_ = false;
    ioExpanderWorkerThread_ = nullptr;
    ioExpanderWorkerQueue_ = nullptr;

    CHECK(reset());
    intConfigs_.clear();
    initialized_ = false;

    return SYSTEM_ERROR_NONE;
}

bool Mcp23s17::initialized() const {
    return initialized_;
}

int Mcp23s17::reset(bool verify) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(IOE_RST != PIN_INVALID, SYSTEM_ERROR_INVALID_ARGUMENT);
    // Assert reset pin
    HAL_GPIO_Write(IOE_RST, 0);
    HAL_Delay_Milliseconds(10);
    HAL_GPIO_Write(IOE_RST, 1);
    HAL_Delay_Milliseconds(10);

    if (verify) {
        uint8_t tmp[22];
        CHECK(readContinuousRegisters(IODIR_ADDR[0], tmp, sizeof(tmp)));
        CHECK_TRUE(tmp[0] == 0xFF, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[1] == 0xFF, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[2] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[3] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[4] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[5] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[6] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[7] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[8] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[9] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[10] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[11] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[12] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[13] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[14] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[15] == 0x00, SYSTEM_ERROR_INTERNAL);
        // The value of INTCAP registers are unknown after reset
        CHECK_TRUE(tmp[18] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[19] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[20] == 0x00, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(tmp[21] == 0x00, SYSTEM_ERROR_INTERNAL);
    }
    resetRegValue();
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::setPinMode(uint8_t port, uint8_t pin, PinMode mode, bool verify) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < MCP23S17_PORT_COUNT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < MCP23S17_PIN_COUNT_PER_PORT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(mode != INPUT_PULLDOWN, SYSTEM_ERROR_NOT_SUPPORTED);
    // TODO: return on invalid pin mode
    uint8_t newVal = gppu_[port];
    uint8_t bitMask = 0x01 << pin;
    if (mode == INPUT_PULLUP) {
        if (!(newVal & bitMask)) {
            newVal |= bitMask;
        }
    } else {
        if (newVal & bitMask) {
            newVal &= ~bitMask;
        }
    }
    if (newVal != gppu_[port]) {
        CHECK(writeRegister(GPPU_ADDR[port], newVal));
        if (verify) {
            uint8_t tmp;
            CHECK(readRegister(GPPU_ADDR[port], &tmp));
            CHECK_TRUE(tmp == newVal, SYSTEM_ERROR_INTERNAL);
        }
        gppu_[port] = newVal;
    }
    
    newVal = iodir_[port];
    if (mode == OUTPUT || mode == OUTPUT_OPEN_DRAIN) {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    } else {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    }
    CHECK(writeRegister(IODIR_ADDR[port], newVal));
    if (verify) {
        uint8_t tmp;
        CHECK(readRegister(IODIR_ADDR[port], &tmp));
        CHECK_TRUE(tmp == newVal, SYSTEM_ERROR_INTERNAL);
    }
    iodir_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::setPinInputInverted(uint8_t port, uint8_t pin, bool enable, bool verify) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < MCP23S17_PORT_COUNT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < MCP23S17_PIN_COUNT_PER_PORT, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t newVal = ipol_[port];
    uint8_t bitMask = 0x01 << pin;
    if (enable) {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    } else {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    }
    CHECK(writeRegister(IPOL_ADDR[port], newVal));
    if (verify) {
        uint8_t tmp;
        CHECK(readRegister(IPOL_ADDR[port], &tmp));
        CHECK_TRUE(tmp == newVal, SYSTEM_ERROR_INTERNAL);
    }
    ipol_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::writePinValue(uint8_t port, uint8_t pin, uint8_t value, bool verify) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < MCP23S17_PORT_COUNT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < MCP23S17_PIN_COUNT_PER_PORT, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t newVal = olat_[port];
    uint8_t bitMask = 0x01 << pin;
    if (value == 1) {
        CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal |= bitMask;
    } else {
        CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
        newVal &= ~bitMask;
    }
    CHECK(writeRegister(OLAT_ADDR[port], newVal));
    if (verify) {
        uint8_t tmp;
        CHECK(readRegister(OLAT_ADDR[port], &tmp));
        CHECK_TRUE(tmp == newVal, SYSTEM_ERROR_INTERNAL);
    }
    olat_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::readPinValue(uint8_t port, uint8_t pin, uint8_t* value) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < MCP23S17_PORT_COUNT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < MCP23S17_PIN_COUNT_PER_PORT, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint8_t newVal = 0x00;
    uint8_t bitMask = 0x01 << pin;
    CHECK(readRegister(GPIO_ADDR[port], &newVal));
    if (newVal & bitMask) {
        *value = 1;
    } else {
        *value = 0;
    }
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::attachPinInterrupt(uint8_t port, uint8_t pin, InterruptMode trig, Mcp23s17InterruptCallback callback, void* context, bool verify) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < MCP23S17_PORT_COUNT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < MCP23S17_PIN_COUNT_PER_PORT, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (callback != nullptr) {
        IoPinInterruptConfig config = {};
        config.port = port;
        config.pin = pin;
        config.trig = trig;
        config.cb = callback;
        config.context = context;
        CHECK_TRUE(intConfigs_.append(config), SYSTEM_ERROR_NO_MEMORY);
    }
    uint8_t newVal = gpinten_[port];
    uint8_t bitMask = 0x01 << pin;
    CHECK_FALSE((newVal & bitMask), SYSTEM_ERROR_NONE);
    newVal |= bitMask;
    CHECK(writeRegister(GPINTEN_ADDR[port], newVal));
    if (verify) {
        uint8_t tmp;
        CHECK(readRegister(GPINTEN_ADDR[port], &tmp));
        CHECK_TRUE(tmp == newVal, SYSTEM_ERROR_INTERNAL);
    }
    gpinten_[port] = newVal;
    return SYSTEM_ERROR_NONE;
}

Mcp23s17& Mcp23s17::getInstance() {
    static Mcp23s17 mcp23s17;
    if (!mcp23s17.initialized()) {
        mcp23s17.begin();
    }
    return mcp23s17;
}

int Mcp23s17::lock() {
    return !mutex_.lock();
}

int Mcp23s17::unlock() {
    return !mutex_.unlock();
}

void Mcp23s17::resetRegValue() {
    memset(iodir_, 0xff, sizeof(iodir_));
    memset(ipol_, 0x00, sizeof(ipol_));
    memset(gpinten_, 0x00, sizeof(gpinten_));
    memset(defval_, 0x00, sizeof(defval_));
    memset(intcon_, 0x00, sizeof(intcon_));
    memset(iocon_, 0x00, sizeof(iocon_));
    memset(gppu_, 0x00, sizeof(gppu_));
    memset(intf_, 0x00, sizeof(intf_));
    memset(intcap_, 0x00, sizeof(intcap_));
    memset(gpio_, 0x00, sizeof(gpio_));
    memset(olat_, 0x00, sizeof(olat_));
}

int Mcp23s17::writeRegister(const uint8_t addr, const uint8_t val) const {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    HAL_SPI_Acquire(spi_, nullptr);
    HAL_GPIO_Write(IOE_CS, 0);
    HAL_SPI_Send_Receive_Data(spi_, MCP23S17_CMD_WRITE);
    HAL_SPI_Send_Receive_Data(spi_, addr);
    HAL_SPI_Send_Receive_Data(spi_, val);
    HAL_GPIO_Write(IOE_CS, 1);
    HAL_SPI_Release(spi_, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::readRegister(const uint8_t addr, uint8_t* const val) const {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    HAL_SPI_Acquire(spi_, nullptr);
    HAL_GPIO_Write(IOE_CS, 0);
    HAL_SPI_Send_Receive_Data(spi_, MCP23S17_CMD_READ);
    HAL_SPI_Send_Receive_Data(spi_, addr);
    *val = HAL_SPI_Send_Receive_Data(spi_, 0xFF);
    HAL_GPIO_Write(IOE_CS, 1);
    HAL_SPI_Release(spi_, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::readContinuousRegisters(const uint8_t start_addr, uint8_t* const val, uint8_t len) const {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    HAL_SPI_Acquire(spi_, nullptr);
    HAL_GPIO_Write(IOE_CS, 0);
    HAL_SPI_Send_Receive_Data(spi_, MCP23S17_CMD_READ);
    HAL_SPI_Send_Receive_Data(spi_, start_addr);
    for (uint8_t i = 0; i < len; i++) {
        val[i] = HAL_SPI_Send_Receive_Data(spi_, 0xFF);
    }
    HAL_GPIO_Write(IOE_CS, 1);
    HAL_SPI_Release(spi_, nullptr);
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::sync() {
    if (ioExpanderWorkerQueue_) {
        bool flag = true;
        os_queue_put(ioExpanderWorkerQueue_, &flag, 0, nullptr);
    }
    return SYSTEM_ERROR_NONE;
}

os_thread_return_t Mcp23s17::ioInterruptHandleThread(void* param) {
    auto instance = static_cast<Mcp23s17*>(param);
    while(!instance->ioExpanderWorkerThreadExit_) {
        bool flag;
        os_queue_take(instance->ioExpanderWorkerQueue_, &flag, CONCURRENT_WAIT_FOREVER, nullptr);
        {
            Mcp23s17Lock lock();
            uint8_t intStatus;
            uint8_t portValue;
            if (instance->readRegister(instance->INTF_ADDR[0], &intStatus) != SYSTEM_ERROR_NONE) {
                continue;
            }
            // This will clear the interrupt flag.
            if (instance->readRegister(instance->INTCAP_ADDR[0], &portValue) != SYSTEM_ERROR_NONE) {
                continue;
            }
            for (const auto& config : instance->intConfigs_) {
                uint8_t bitMask = 0x01 << config.pin;
                if ((intStatus & bitMask) && (config.cb != nullptr)) {
                    if ( ((config.trig == RISING) && (portValue & bitMask)) ||
                         ((config.trig == FALLING) && !(portValue & bitMask)) ||
                          (config.trig == CHANGE) ) {
                        config.cb(config.context);
                    }
                }
            }
        }
    }
    os_thread_exit(instance->ioExpanderWorkerThread_);
}

StaticRecursiveMutex Mcp23s17::mutex_;
