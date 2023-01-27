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

#include "mcp23s17.h"

#if HAL_PLATFORM_MCP23S17

//#define LOG_CHECKED_ERRORS 1

#include "check.h"
#include "system_error.h"
#include "gpio_hal.h"
#include "delay_hal.h"
#include "spi_lock.h"
#include <mutex>

using namespace particle;

namespace {
void ioExpanderInterruptHandler(void* data) {
    auto instance = static_cast<Mcp23s17*>(data);
    instance->sync();
}

const hal_spi_info_t sDefaultSettings = {
    .version = HAL_SPI_INFO_VERSION,
    .system_clock = 0,
    .default_settings = 0,
    .enabled = true,
    .mode = SPI_MODE_MASTER,
    .clock = HAL_PLATFORM_MCP23S17_SPI_CLOCK,
    .bit_order = MSBFIRST,
    .data_mode = SPI_MODE0,
    .ss_pin = PIN_INVALID
};

} // anonymous namespace


Mcp23s17::Mcp23s17()
        : initialized_(false),
          spi_(HAL_PLATFORM_MCP23S17_SPI),
          ioExpanderWorkerThread_(nullptr),
          ioExpanderWorkerThreadExit_(false),
          ioExpanderWorkerSemaphore_(nullptr),
          spiLock_(HAL_PLATFORM_MCP23S17_SPI, sDefaultSettings) {
    begin();
}

Mcp23s17::~Mcp23s17() {

}

int Mcp23s17::begin() {
    Mcp23s17Lock lock();
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);

    hal_gpio_mode(IOE_CS, OUTPUT);
    hal_gpio_write(IOE_CS, 1);
    hal_gpio_mode(IOE_RST, OUTPUT);
    hal_gpio_write(IOE_RST, 1);
    hal_gpio_mode(IOE_INT, INPUT_PULLUP);

    if (!hal_spi_is_enabled(spi_)) {
        hal_spi_init(spi_);
    }

    if (os_semaphore_create(&ioExpanderWorkerSemaphore_, 1, 0)) {
        ioExpanderWorkerSemaphore_ = nullptr;
        LOG(ERROR, "os_semaphore_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }
    if (os_thread_create(&ioExpanderWorkerThread_, "IO Expander Thread", OS_THREAD_PRIORITY_CRITICAL, ioInterruptHandleThread, this, 512)) {
        os_semaphore_destroy(ioExpanderWorkerSemaphore_);
        ioExpanderWorkerSemaphore_ = nullptr;
        LOG(ERROR, "os_thread_create() failed");
        return SYSTEM_ERROR_INTERNAL;
    }

    hal_interrupt_extra_configuration_t extra = {};
    extra.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1;
#if HAL_PLATFORM_SHARED_INTERRUPT
    extra.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION;
    extra.appendHandler = 1;
    extra.chainPriority = 0xFF; // Lowest priority
#endif
    CHECK(hal_interrupt_attach(IOE_INT, ioExpanderInterruptHandler, this, FALLING, &extra));

    initialized_ = true;
    CHECK(reset());

    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::end() {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);

    CHECK(reset());

    ioExpanderWorkerThreadExit_ = true;
    os_thread_join(ioExpanderWorkerThread_);
    os_thread_cleanup(ioExpanderWorkerThread_);
    os_semaphore_destroy(ioExpanderWorkerSemaphore_);
    ioExpanderWorkerSemaphore_ = nullptr;
    ioExpanderWorkerThreadExit_ = false;
    ioExpanderWorkerThread_ = nullptr;

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
    hal_gpio_write(IOE_RST, 0);
    HAL_Delay_Milliseconds(10);
    hal_gpio_write(IOE_RST, 1);
    HAL_Delay_Milliseconds(10);

#if HAL_PLATFORM_MCP23S17_MIRROR_INTERRUPTS || HAL_PLATFORM_MCP23S17_OPEN_DRAIN_INT
    uint8_t iocon {};

#if HAL_PLATFORM_MCP23S17_MIRROR_INTERRUPTS
    iocon |= 1u << 6;
#endif
#if HAL_PLATFORM_MCP23S17_OPEN_DRAIN_INT
    iocon |= 1u << 2;
#endif

    CHECK(writeRegister(IOCON_ADDR[0], iocon));
#endif

    if (verify) {
        uint8_t tmp[22] = {0x00};
        CHECK(readContinuousRegisters(IODIR_ADDR[0], tmp, sizeof(tmp)));
        // The value of INTCAP registers are unknown after reset, reading port returns current pin value.
        tmp[16] = tmp[17] = tmp[18] = tmp[19] = 0x00;
        CHECK_TRUE(memcmp(tmp, DEFAULT_REGS_VALUE, sizeof(tmp)) == 0, SYSTEM_ERROR_INTERNAL);
    }
    resetRegValue();
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::setPinMode(uint8_t port, uint8_t pin, PinMode mode, bool verify) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < MCP23S17_PORT_COUNT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < MCP23S17_PIN_COUNT_PER_PORT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(mode == INPUT_PULLUP || mode == INPUT || mode == OUTPUT, SYSTEM_ERROR_NOT_SUPPORTED);
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
    if (mode == OUTPUT) {
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
    CHECK(readRegister(IODIR_ADDR[port], &newVal));
    if (newVal & bitMask) { // Read input value
        CHECK(readRegister(GPIO_ADDR[port], &newVal));
    } else {
        CHECK(readRegister(OLAT_ADDR[port], &newVal));
    }
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
    CHECK_TRUE(callback, SYSTEM_ERROR_INVALID_ARGUMENT);

    // Avoid duplicated int configuration for the same pin.
    for (auto& config : intConfigs_) {
        if (config.port == port && config.pin == pin) {
            config.trig = trig;
            config.cb = callback;
            config.context = context;
            // The interrupt has been enabled previously.
            return SYSTEM_ERROR_NONE;
        }
    }

    IoPinInterruptConfig config = {};
    config.port = port;
    config.pin = pin;
    config.trig = trig;
    config.cb = callback;
    config.context = context;
    CHECK_TRUE(intConfigs_.append(config), SYSTEM_ERROR_NO_MEMORY);

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

int Mcp23s17::detachPinInterrupt(uint8_t port, uint8_t pin, bool verify) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(port < MCP23S17_PORT_COUNT, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(pin < MCP23S17_PIN_COUNT_PER_PORT, SYSTEM_ERROR_INVALID_ARGUMENT);
    
    for (auto& config : intConfigs_) {
        if (config.port == port && config.pin == pin) {
            intConfigs_.removeOne(config);

            uint8_t newVal = gpinten_[port];
            uint8_t bitMask = 0x01 << pin;
            CHECK_TRUE((newVal & bitMask), SYSTEM_ERROR_NONE);
            newVal &= ~bitMask;
            CHECK(writeRegister(GPINTEN_ADDR[port], newVal));
            if (verify) {
                uint8_t tmp;
                CHECK(readRegister(GPINTEN_ADDR[port], &tmp));
                CHECK_TRUE(tmp == newVal, SYSTEM_ERROR_INTERNAL);
            }
            gpinten_[port] = newVal;
        }
    }

    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::interruptsSuspend() {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    // Simply reset the registers, since we have cached current registers' value
    CHECK(writeRegister(GPINTEN_ADDR[0], 0x00));
    CHECK(writeRegister(GPINTEN_ADDR[1], 0x00));
    CHECK(writeRegister(INTCON_ADDR[0], 0x00));
    CHECK(writeRegister(INTCON_ADDR[1], 0x00));
    CHECK(writeRegister(DEFVAL_ADDR[0], 0x00));
    CHECK(writeRegister(DEFVAL_ADDR[1], 0x00));
    // Clear the pending interrupts flag
    uint8_t dummy;
    CHECK(readRegister(INTCAP_ADDR[0], &dummy));
    CHECK(readRegister(INTCAP_ADDR[1], &dummy));
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::interruptsRestore(uint8_t intStatus[2]) {
    Mcp23s17Lock lock();
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(readRegister(INTF_ADDR[0], &intStatus[0]));
    CHECK(readRegister(INTF_ADDR[1], &intStatus[1]));
    CHECK(writeRegister(INTCON_ADDR[0], intcon_[0]));
    CHECK(writeRegister(INTCON_ADDR[1], intcon_[1]));
    CHECK(writeRegister(DEFVAL_ADDR[0], defval_[0]));
    CHECK(writeRegister(DEFVAL_ADDR[1], defval_[1]));
    CHECK(writeRegister(GPINTEN_ADDR[0], gpinten_[0]));
    CHECK(writeRegister(GPINTEN_ADDR[1], gpinten_[1]));
    return SYSTEM_ERROR_NONE;
}

Mcp23s17& Mcp23s17::getInstance() {
    static Mcp23s17 mcp23s17;
    return mcp23s17;
}

int Mcp23s17::lock() {
    return hal_spi_acquire(spi_, nullptr);
}

int Mcp23s17::unlock() {
    return hal_spi_release(spi_, nullptr);
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
    std::lock_guard<SpiConfigurationLock> spiGuarder(spiLock_);
    hal_gpio_write(IOE_CS, 0);
    hal_spi_transfer(spi_, MCP23S17_CMD_WRITE);
    hal_spi_transfer(spi_, addr);
    hal_spi_transfer(spi_, val);
    hal_gpio_write(IOE_CS, 1);
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::readRegister(const uint8_t addr, uint8_t* const val) const {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    CHECK_TRUE(val != nullptr, SYSTEM_ERROR_INVALID_ARGUMENT);
    std::lock_guard<SpiConfigurationLock> spiGuarder(spiLock_);
    hal_gpio_write(IOE_CS, 0);
    hal_spi_transfer(spi_, MCP23S17_CMD_READ);
    hal_spi_transfer(spi_, addr);
    *val = hal_spi_transfer(spi_, 0xFF);
    hal_gpio_write(IOE_CS, 1);
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::readContinuousRegisters(const uint8_t start_addr, uint8_t* const val, uint8_t len) const {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    CHECK_TRUE(val != nullptr, SYSTEM_ERROR_INVALID_ARGUMENT);
    std::lock_guard<SpiConfigurationLock> spiGuarder(spiLock_);
    hal_gpio_write(IOE_CS, 0);
    hal_spi_transfer(spi_, MCP23S17_CMD_READ);
    hal_spi_transfer(spi_, start_addr);
    for (uint8_t i = 0; i < len; i++) {
        val[i] = hal_spi_transfer(spi_, 0xFF);
    }
    hal_gpio_write(IOE_CS, 1);
    return SYSTEM_ERROR_NONE;
}

int Mcp23s17::sync() {
    if (ioExpanderWorkerSemaphore_) {
        return os_semaphore_give(ioExpanderWorkerSemaphore_, false);
    }
    return SYSTEM_ERROR_NONE;
}

os_thread_return_t Mcp23s17::ioInterruptHandleThread(void* param) {
    auto instance = static_cast<Mcp23s17*>(param);
    while(!instance->ioExpanderWorkerThreadExit_) {
        os_semaphore_take(instance->ioExpanderWorkerSemaphore_, CONCURRENT_WAIT_FOREVER, false);
        {
            Mcp23s17Lock lock();
            uint8_t intStatus[2];
            uint8_t portValue[2];
            if (instance->readRegister(instance->INTF_ADDR[0], &intStatus[0]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (instance->readRegister(instance->INTF_ADDR[1], &intStatus[1]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            // This will clear the interrupt flag.
            if (instance->readRegister(instance->INTCAP_ADDR[0], &portValue[0]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            if (instance->readRegister(instance->INTCAP_ADDR[1], &portValue[1]) != SYSTEM_ERROR_NONE) {
                continue;
            }
            for (const auto& config : instance->intConfigs_) {
                uint8_t bitMask = 0x01 << config.pin;
                if ((intStatus[config.port] & bitMask) && (config.cb != nullptr)) {
                    if ( ((config.trig == RISING) && (portValue[config.port] & bitMask)) ||
                         ((config.trig == FALLING) && !(portValue[config.port] & bitMask)) ||
                          (config.trig == CHANGE) ) {
                        config.cb(config.context);
                    }
                }
            }
        }
    }
    os_thread_exit(instance->ioExpanderWorkerThread_);
}

constexpr uint8_t Mcp23s17::IODIR_ADDR[2];
constexpr uint8_t Mcp23s17::IPOL_ADDR[2];
constexpr uint8_t Mcp23s17::GPINTEN_ADDR[2];
constexpr uint8_t Mcp23s17::DEFVAL_ADDR[2];
constexpr uint8_t Mcp23s17::INTCON_ADDR[2];
constexpr uint8_t Mcp23s17::IOCON_ADDR[2];
constexpr uint8_t Mcp23s17::GPPU_ADDR[2];
constexpr uint8_t Mcp23s17::INTF_ADDR[2];
constexpr uint8_t Mcp23s17::INTCAP_ADDR[2];
constexpr uint8_t Mcp23s17::GPIO_ADDR[2];
constexpr uint8_t Mcp23s17::OLAT_ADDR[2];

constexpr uint8_t Mcp23s17::MCP23S17_CMD_READ;
constexpr uint8_t Mcp23s17::MCP23S17_CMD_WRITE;

constexpr uint8_t Mcp23s17::DEFAULT_REGS_VALUE[22];

#endif // HAL_PLATFORM_MCP23S17
