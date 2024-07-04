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

#ifndef MCP23S17_H
#define MCP23S17_H

#include "application.h"
// #include "static_recursive_mutex.h"
// #include "pinmap_defines.h"
// #include "pinmap_hal.h"
// #include "spi_hal.h"
// #include "interrupts_hal.h"
// #include "concurrent_hal.h"
// #include "spark_wiring_vector.h"
#include "spi_lock.h"

/* Custom definitions */
#ifdef HAL_PLATFORM_MCP23S17_MIRROR_INTERRUPTS
#undef HAL_PLATFORM_MCP23S17_MIRROR_INTERRUPTS
#endif
#define HAL_PLATFORM_MCP23S17_MIRROR_INTERRUPTS (1)

#ifdef HAL_PLATFORM_MCP23S17_OPEN_DRAIN_INT
#undef HAL_PLATFORM_MCP23S17_OPEN_DRAIN_INT
#endif
#define HAL_PLATFORM_MCP23S17_OPEN_DRAIN_INT (0)

#ifdef HAL_PLATFORM_SHARED_INTERRUPT
#undef HAL_PLATFORM_SHARED_INTERRUPT
#endif
#define HAL_PLATFORM_SHARED_INTERRUPT (0)

#ifdef HAL_PLATFORM_MCP23S17_SPI_CLOCK
#undef HAL_PLATFORM_MCP23S17_SPI_CLOCK
#endif
#define HAL_PLATFORM_MCP23S17_SPI_CLOCK (1000000)

#ifdef HAL_PLATFORM_MCP23S17_SPI
#undef HAL_PLATFORM_MCP23S17_SPI
#endif
#define HAL_PLATFORM_MCP23S17_SPI (HAL_SPI_INTERFACE1)

#ifndef IOE_CS
#define IOE_CS (D23)
#endif

#ifndef IOE_INT
#define IOE_INT (D8)
#endif

// D22 on Muon below v0.5
#ifndef IOE_RST
#define IOE_RST PIN_INVALID
#endif

#ifndef MCP23S17_PORT_A
#define MCP23S17_PORT_A ((uint8_t)0u)
#endif

#ifndef MCP23S17_PORT_B
#define MCP23S17_PORT_B ((uint8_t)1u)
#endif
/* End of custom definitions */

#define MCP23S17_PORT_COUNT             (2)
#define MCP23S17_PIN_COUNT_PER_PORT     (8)

namespace particle {

typedef void (*Mcp23s17InterruptCallback)(void* context);

class Mcp23s17 {
public:
    int begin();
    int end();
    bool initialized() const;
    int reset(bool verify = true);
    int sync();

    int setPinMode(uint8_t port, uint8_t pin, PinMode mode, bool verify = true);
    int setPinInputInverted(uint8_t port, uint8_t pin, bool enable, bool verify = true);
    int writePinValue(uint8_t port, uint8_t pin, uint8_t value, bool verify = true);
    int readPinValue(uint8_t port, uint8_t pin, uint8_t* value);
    int attachPinInterrupt(uint8_t port, uint8_t pin, InterruptMode trig, Mcp23s17InterruptCallback callback, void* context, bool verify = true);
    int detachPinInterrupt(uint8_t port, uint8_t pin, bool verify = true);
    int interruptsSuspend();
    int interruptsRestore(uint8_t intStatus[2]);

    void resetRegValue();
    int writeRegister(const uint8_t addr, const uint8_t val) const;
    int writeContinuousRegisters(const uint8_t start_addr, const uint8_t* val, uint8_t len) const;
    int readRegister(const uint8_t addr, uint8_t* const val) const;
    int readContinuousRegisters(const uint8_t start_addr, uint8_t* const val, uint8_t len) const;

    int lock();
    int unlock();

    static Mcp23s17& getInstance();

    // Resister address
    static constexpr uint8_t IODIR_ADDR[2]     = {0x00, 0x01};
    static constexpr uint8_t IPOL_ADDR[2]      = {0x02, 0x03};
    static constexpr uint8_t GPINTEN_ADDR[2]   = {0x04, 0x05};
    static constexpr uint8_t DEFVAL_ADDR[2]    = {0x06, 0x07};
    static constexpr uint8_t INTCON_ADDR[2]    = {0x08, 0x09};
    static constexpr uint8_t IOCON_ADDR[2]     = {0x0A, 0x0B};
    static constexpr uint8_t GPPU_ADDR[2]      = {0x0C, 0x0D};
    static constexpr uint8_t INTF_ADDR[2]      = {0x0E, 0x0F};
    static constexpr uint8_t INTCAP_ADDR[2]    = {0x10, 0x11};
    static constexpr uint8_t GPIO_ADDR[2]      = {0x12, 0x13};
    static constexpr uint8_t OLAT_ADDR[2]      = {0x14, 0x15};

private:
    struct IoPinInterruptConfig {
        IoPinInterruptConfig()
                : port(PIN_INVALID),
                  pin(PIN_INVALID),
                  cb(nullptr),
                  context(nullptr) {
        }
        ~IoPinInterruptConfig() {}

        bool operator==(const IoPinInterruptConfig& config) const {
            return (port == config.port && pin == config.pin);
        }

        uint8_t port;
        uint8_t pin;
        InterruptMode trig;
        Mcp23s17InterruptCallback cb;
        void* context;
    };

    Mcp23s17();
    ~Mcp23s17();

    static os_thread_return_t ioInterruptHandleThread(void* param);

    // Read/write coomand
    static constexpr uint8_t MCP23S17_CMD_READ = 0x41;
    static constexpr uint8_t MCP23S17_CMD_WRITE = 0x40;

#if HAL_PLATFORM_MCP23S17_MIRROR_INTERRUPTS
    static constexpr uint8_t DEFAULT_REGS_VALUE[22] = {
        0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
#else
    // FIXME: IOCON value depends on HAL_PLATFORM_MCP23S17_OPEN_DRAIN_INT
    static constexpr uint8_t DEFAULT_REGS_VALUE[22] = {
        0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
#endif

    uint8_t iodir_[2];
    uint8_t ipol_[2];
    uint8_t gpinten_[2];
    uint8_t defval_[2];
    uint8_t intcon_[2];
    uint8_t iocon_[2];
    uint8_t gppu_[2];
    uint8_t intf_[2];
    uint8_t intcap_[2];
    uint8_t gpio_[2];
    uint8_t olat_[2];

    bool initialized_;
    hal_spi_interface_t spi_;
    os_thread_t ioExpanderWorkerThread_;
    bool ioExpanderWorkerThreadExit_;
    os_semaphore_t ioExpanderWorkerSemaphore_;
    Vector<IoPinInterruptConfig> intConfigs_;
    mutable SpiConfigurationLock spiLock_;
}; // class Mcp23s17

class Mcp23s17Lock {
public:
    Mcp23s17Lock()
            : locked_(false) {
        lock();
    }

    ~Mcp23s17Lock() {
        if (locked_) {
            unlock();
        }
    }

    Mcp23s17Lock(Mcp23s17Lock&& lock)
            : locked_(lock.locked_) {
        lock.locked_ = false;
    }

    void lock() {
        Mcp23s17::getInstance().lock();
        locked_ = true;
    }

    void unlock() {
        Mcp23s17::getInstance().unlock();
        locked_ = false;
    }

    Mcp23s17Lock(const Mcp23s17Lock&) = delete;
    Mcp23s17Lock& operator=(const Mcp23s17Lock&) = delete;

private:
    bool locked_;
};

} // namespace particle

#endif // MCP23S17_H