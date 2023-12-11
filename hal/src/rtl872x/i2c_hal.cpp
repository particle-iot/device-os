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

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <cstring>
#include "ringbuffer.h"
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "delay_hal.h"
#include "platforms.h"
#include "concurrent_hal.h"
#include "interrupts_hal.h"
#include "pinmap_impl.h"
#include "logging.h"
#include "system_error.h"
#include "system_tick_hal.h"
#include "timer_hal.h"
#include <memory>
#include "check.h"
#include "service_debug.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "rtl8721d.h"
#ifdef __cplusplus
}
#endif

#define WAIT_TIMED_ROUTINE(timeout_ms, what, func) ({ \
    system_tick_t _micros = HAL_Timer_Get_Micro_Seconds();                      \
    int res = 0;                                                                \
    while ((what)) {                                                            \
        system_tick_t dt = (HAL_Timer_Get_Micro_Seconds() - _micros);           \
        bool nok = (((timeout_ms * 1000) < dt) && (what));                      \
        if (nok) {                                                              \
            res = -1;                                                           \
            break;                                                              \
        }                                                                       \
        if (func()) {                                                           \
            res = 0;                                                            \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    res;                                                                        \
})

#define WAIT_TIMED(timeout_ms, what) ({ \
    system_tick_t _micros = HAL_Timer_Get_Micro_Seconds();                      \
    bool res = true;                                                            \
    while ((what)) {                                                            \
        system_tick_t dt = (HAL_Timer_Get_Micro_Seconds() - _micros);           \
        bool nok = (((timeout_ms * 1000) < dt) && (what));                      \
        if (nok) {                                                              \
            res = false;                                                        \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    res;                                                                        \
})

class I2cClass {
public:
    bool isConfigured() const {
        return configured_;
    }

    bool isEnabled() const {
        return state_ == HAL_I2C_STATE_ENABLED;
    }

    int init(const hal_i2c_config_t* conf) {
        os_thread_scheduling(false, nullptr);
        if (!mutex_) {
            os_mutex_recursive_create(&mutex_);
        }
        os_thread_scheduling(true, nullptr);
        lock();
        if (isConfigured()) {
            // Configured, but new buffers are invalid
            if (!isConfigValid(conf)){
                unlock();
                return SYSTEM_ERROR_INVALID_ARGUMENT;
            }
            // Configured, but new buffers are smaller
            if (conf->rx_buffer_size <= rxBuffer_.size() ||
               conf->tx_buffer_size <= txBuffer_.size()) {
               unlock();
               return SYSTEM_ERROR_NOT_ENOUGH_DATA;
            }
            CHECK(deInit());
        }
        if (isConfigValid(conf)) {
            rxBuffer_.init((uint8_t*)conf->rx_buffer, conf->rx_buffer_size);
            txBuffer_.init((uint8_t*)conf->tx_buffer, conf->tx_buffer_size);
            heapBuffer_ = ((conf->version >= HAL_I2C_CONFIG_VERSION_2) && (conf->flags & HAL_I2C_CONFIG_FLAG_FREEABLE));
        } else {
            int buffer_size = HAL_PLATFORM_I2C_BUFFER_SIZE(HAL_I2C_INTERFACE1);
            rxBuffer_.init((uint8_t*)malloc(buffer_size), buffer_size);
            txBuffer_.init((uint8_t*)malloc(buffer_size), buffer_size);
            SPARK_ASSERT(txBuffer_.buffer_ && rxBuffer_.buffer_);
            heapBuffer_ = true;
        }
        slaveRxCacheDepth_ = rxBuffer_.size() * 2;
        I2C_StructInit(&i2cInitStruct_);
        configured_ = true;
        unlock();
        return SYSTEM_ERROR_NONE;
    }

    int deInit() {
        if (heapBuffer_) {
            // The pointers are guaranteed to be non-nullptr when they are inited.
            free(txBuffer_.buffer_);
            free(rxBuffer_.buffer_);
            heapBuffer_ = false;
        }
        configured_ = false;
        state_ = HAL_I2C_STATE_DISABLED;
        return SYSTEM_ERROR_NONE;
    }

    int begin(hal_i2c_mode_t mode, uint8_t address) {
        if (isEnabled()) {
            end();
        }
        // RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);
        Pinmux_Config(hal_pin_to_rtl_pin(sdaPin_), PINMUX_FUNCTION_I2C);
	    Pinmux_Config(hal_pin_to_rtl_pin(sclPin_), PINMUX_FUNCTION_I2C);
        PAD_PullCtrl(hal_pin_to_rtl_pin(sdaPin_), GPIO_PuPd_UP);
	    PAD_PullCtrl(hal_pin_to_rtl_pin(sclPin_), GPIO_PuPd_UP);

        hal_gpio_set_drive_strength(sclPin_, HAL_GPIO_DRIVE_HIGH);
        hal_gpio_set_drive_strength(sdaPin_, HAL_GPIO_DRIVE_HIGH);

        if (mode == I2C_MODE_MASTER) {
            i2cInitStruct_.I2CMaster  = I2C_MASTER_MODE;
            i2cInitStruct_.I2CAckAddr = 0; // This is the peer device's slave address in master mode, not needed herein.
        } else {
            if (hal_interrupt_is_isr()) {
                SPARK_ASSERT(false);
            }
            slaveRxCache_ = std::make_unique<uint8_t[]>(slaveRxCacheDepth_);
            if (!slaveRxCache_) {
                SPARK_ASSERT(false);
            }
            i2cInitStruct_.I2CMaster  = I2C_SLAVE_MODE;
            i2cInitStruct_.I2CAckAddr = address; // This is the local device's slave address in slave mode
        }
        I2C_Init(i2cDev_, &i2cInitStruct_);

        // Enable restart function
        i2cDev_->IC_CON |= BIT_CTRL_IC_CON_IC_RESTART_EN;
        i2cDev_->IC_RX_TL = std::min(rxBuffer_.size(), (size_t)0); // FIFO depth 1

	    I2C_Cmd(i2cDev_, ENABLE);

        if (i2cInitStruct_.I2CMaster == I2C_SLAVE_MODE) {
            InterruptRegister((IRQ_FUN)i2cSlaveIntHandler, I2C0_IRQ_LP, (uint32_t)this, 7);
            I2C_INTConfig(i2cDev_, (BIT_IC_INTR_MASK_M_START_DET | BIT_IC_INTR_MASK_M_RX_FULL | BIT_IC_INTR_MASK_M_STOP_DET |
                                    BIT_IC_INTR_MASK_M_RD_REQ | BIT_IC_INTR_MASK_M_RX_DONE), ENABLE);
            I2C_ClearAllINT(i2cDev_);
            InterruptEn(I2C0_IRQ_LP, 7);
        }

        if (state_ == HAL_I2C_STATE_DISABLED) {
            txBuffer_.reset();
            rxBuffer_.reset();
        }

        state_ = HAL_I2C_STATE_ENABLED;
        HAL_Delay_Milliseconds(10);
        return SYSTEM_ERROR_NONE;
    }

    int end() {
        if (isEnabled()) {
            hal_gpio_config_t conf = {};
            conf.size = sizeof(conf);
            conf.version = HAL_GPIO_VERSION;
            conf.mode = OUTPUT_OPEN_DRAIN_PULLUP;
            conf.set_value = true;
            conf.value = 1;
            conf.drive_strength = HAL_GPIO_DRIVE_DEFAULT;
            hal_gpio_configure(sclPin_, &conf, nullptr);
            hal_gpio_configure(sdaPin_, &conf, nullptr);
            if (i2cInitStruct_.I2CMaster == I2C_SLAVE_MODE) {
                InterruptDis(I2C0_IRQ_LP);
	            InterruptUnRegister(I2C0_IRQ_LP);
                I2C_INTConfig(i2cDev_, (BIT_IC_INTR_MASK_M_START_DET | BIT_IC_INTR_MASK_M_RX_FULL | BIT_IC_INTR_MASK_M_STOP_DET |
                                        BIT_IC_INTR_MASK_M_RD_REQ | BIT_IC_INTR_MASK_M_RX_DONE), DISABLE);
            }
            I2C_Cmd(i2cDev_, DISABLE);
            state_ = HAL_I2C_STATE_DISABLED;
            // RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, DISABLE);
        }
        return SYSTEM_ERROR_NONE;
    }

    int suspend() {
        CHECK_TRUE(state_ = HAL_I2C_STATE_ENABLED, SYSTEM_ERROR_INVALID_STATE);
        CHECK(end());
        state_ = HAL_I2C_STATE_SUSPENDED;
        return SYSTEM_ERROR_NONE;
    }

    int restore() {
        CHECK_TRUE(state_ = HAL_I2C_STATE_SUSPENDED, SYSTEM_ERROR_INVALID_STATE);
        return begin((i2cInitStruct_.I2CMaster == I2C_MASTER_MODE) ? I2C_MODE_MASTER : I2C_MODE_SLAVE, i2cInitStruct_.I2CAckAddr);
    }

    int reset() {
        end();

        // Just in case make sure that the pins are correctly configured (they should anyway be at this point)
        hal_gpio_config_t conf = {};
        conf.size = sizeof(conf);
        conf.version = HAL_GPIO_VERSION;
        conf.mode = OUTPUT_OPEN_DRAIN_PULLUP;
        conf.set_value = true;
        conf.value = 1;
        conf.drive_strength = HAL_GPIO_DRIVE_DEFAULT;
        hal_gpio_configure(sclPin_, &conf, nullptr);
        hal_gpio_configure(sdaPin_, &conf, nullptr);

        // Check if slave is stretching the SCL
        if (!WAIT_TIMED(HAL_I2C_DEFAULT_TIMEOUT_MS, hal_gpio_read(sclPin_) == 0)) {
            return SYSTEM_ERROR_I2C_BUS_BUSY;
        }

        // Generate up to 9 pulses on SCL to tell slave to release the bus
        for (int i = 0; i < 9; i++) {
            if (hal_gpio_read(sdaPin_) == 0) {
                hal_gpio_write(sclPin_, 0);
                HAL_Delay_Microseconds(50);
                hal_gpio_write(sclPin_, 1);
                HAL_Delay_Microseconds(50);
                hal_gpio_write(sclPin_, 0);
                HAL_Delay_Microseconds(50);
            } else {
                break;
            }
        }

        // Generate STOP condition: pull SDA low, switch to high
        hal_gpio_write(sdaPin_, 0);
        HAL_Delay_Microseconds(50);
        hal_gpio_write(sclPin_, 1);
        HAL_Delay_Microseconds(50);
        hal_gpio_write(sdaPin_, 1);
        HAL_Delay_Microseconds(50);

        hal_pin_set_function(sdaPin_, PF_I2C);
        hal_pin_set_function(sclPin_, PF_I2C);

        hal_i2c_mode_t mode = (i2cInitStruct_.I2CMaster  == I2C_MASTER_MODE) ? I2C_MODE_MASTER : I2C_MODE_SLAVE;
        begin(mode, i2cInitStruct_.I2CAckAddr);
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INTERNAL);
        return SYSTEM_ERROR_NONE;
    }

    int setSpeed(uint32_t speed) {
        bool enabled = isEnabled();
        if (enabled) {
            I2C_Cmd(i2cDev_, DISABLE);
            state_ = HAL_I2C_STATE_DISABLED;
        }
        i2cInitStruct_.I2CSpdMod = ((speed == CLOCK_SPEED_100KHZ) ? I2C_SS_MODE : I2C_FS_MODE);
        i2cInitStruct_.I2CClk    = ((speed == CLOCK_SPEED_100KHZ) ? 100 : 400);
        if (enabled) {
            I2C_Init(i2cDev_, &i2cInitStruct_);
            I2C_Cmd(i2cDev_, ENABLE);
            state_ = HAL_I2C_STATE_ENABLED;
        }
        return SYSTEM_ERROR_NONE;
    }

    int setAddress(uint8_t address) {
        if (i2cInitStruct_.I2CAckAddr != address) {
            bool enabled = isEnabled();
            if (enabled) {
                I2C_Cmd(i2cDev_, DISABLE);
                state_ = HAL_I2C_STATE_DISABLED;
            }
            i2cInitStruct_.I2CAckAddr = address;
            if (enabled) {
                I2C_Init(i2cDev_, &i2cInitStruct_);
                I2C_Cmd(i2cDev_, ENABLE);
                state_ = HAL_I2C_STATE_ENABLED;
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    ssize_t requestFrom(const hal_i2c_transmission_config_t* config) {
        if (i2cInitStruct_.I2CMaster != I2C_MASTER_MODE) {
            return 0;
        }
        rxBuffer_.reset();
        uint32_t quantity = std::min((size_t)config->quantity, rxBuffer_.size());

        // Dirty-hack: It may not generate the start signal when communicating with certain type of slave device.
        if (reEnableIfNeeded() != SYSTEM_ERROR_NONE) {
            return 0;
        }

        setAddress(config->address);

        bool waitStop = false;
        for (uint32_t i = 0; i < quantity; i++) {
            if(i >= quantity - 1) {
                if (config->flags & HAL_I2C_TRANSMISSION_FLAG_STOP) {
                    // Generate stop signal
                    i2cDev_->IC_DATA_CMD = 0x0003 << 8;
                    waitStop = true;
                } else {
                    // Generate restart signal
                    i2cDev_->IC_DATA_CMD = 0x0005 << 8;
                }
            } else {
                i2cDev_->IC_DATA_CMD = 0x0001 << 8;
            }
            // Wait for I2C_FLAG_RFNE flag
            if (WAIT_TIMED_ROUTINE(config->timeout_ms, I2C_CheckFlagState(i2cDev_, BIT_IC_STATUS_RFNE) == 0, checkAbrt) < 0) {
                reset();
                LOG_DEBUG(TRACE, "Wait BIT_IC_STATUS_RFNE timeout");
                goto ret;
            }
            if(checkAbrt()) {
                LOG(TRACE, "Abort: %08X", i2cDev_->IC_TX_ABRT_SOURCE);
                I2C_ClearAllINT(i2cDev_);
                goto ret;
            }
            if (waitStop && !WAIT_TIMED(config->timeout_ms, !stopDetected())) {
                reset();
                return SYSTEM_ERROR_I2C_STOP_TIMEOUT;
            }
            rxBuffer_.put((uint8_t)i2cDev_->IC_DATA_CMD);
        }
    ret:
        return rxBuffer_.data();
    }

    ssize_t available() {
        return rxBuffer_.data();
    }

    uint8_t read() {
        uint8_t data = 0xFF;
        if (rxBuffer_.get(&data) != 1) {
            return 0xFF;
        }
        return data;
    }

    uint8_t peek() {
        uint8_t data = 0xFF;
        if (rxBuffer_.peek(&data) != 1) {
            return 0xFF;
        }
        return data;
    }

    void setConfigOrDefault(const hal_i2c_transmission_config_t* config, uint8_t address) {
        memset(&transConfig_, 0, sizeof(transConfig_));
        if (config) {
            memcpy(&transConfig_, config, std::min((size_t)config->size, sizeof(transConfig_)));
        } else {
            transConfig_ = {
                .size = sizeof(hal_i2c_transmission_config_t),
                .version = 0,
                .address = address,
                .reserved = {0},
                .quantity = 0,
                .timeout_ms = HAL_I2C_DEFAULT_TIMEOUT_MS,
                .flags = HAL_I2C_TRANSMISSION_FLAG_STOP
            };
        }
    }

    ssize_t write(uint8_t data) {
        txBuffer_.put(data);
        return 1;
    }

    int endTransmission(uint8_t stop) {
        if (i2cInitStruct_.I2CMaster != I2C_MASTER_MODE) {
            return SYSTEM_ERROR_INVALID_STATE;
        }

        // Dirty-hack: It may not generate the start signal when communicating with certain type of slave device.
        CHECK(reEnableIfNeeded());

        setAddress(transConfig_.address);

        uint32_t quantity = txBuffer_.data();
        if (quantity == 0) {
            // Clear flags
            uint32_t temp = i2cDev_->IC_CLR_TX_ABRT;
            temp = i2cDev_->IC_CLR_STOP_DET;
            (void)temp;
            if ((i2cDev_->IC_TX_ABRT_SOURCE & BIT_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK)
                 || (i2cDev_->IC_RAW_INTR_STAT & BIT_IC_RAW_INTR_STAT_STOP_DET)) {
                return SYSTEM_ERROR_I2C_ABORT;
            }
            // Send the slave address only
            i2cDev_->IC_DATA_CMD = (transConfig_.address << 1) | BIT_CTRL_IC_DATA_CMD_NULLDATA | BIT_CTRL_IC_DATA_CMD_STOP;
            // If slave is not detected, the STOP_DET bit won't be set, and the TX_ABRT is set instead.
            if (!WAIT_TIMED(transConfig_.timeout_ms, ((i2cDev_->IC_TX_ABRT_SOURCE & BIT_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK) == 0)
                                                  && ((i2cDev_->IC_RAW_INTR_STAT & BIT_IC_RAW_INTR_STAT_STOP_DET) == 0))) {
                return SYSTEM_ERROR_I2C_TX_ADDR_TIMEOUT;
            }
            if (i2cDev_->IC_TX_ABRT_SOURCE & BIT_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK) {
                return SYSTEM_ERROR_I2C_ABORT;
            }
            return SYSTEM_ERROR_NONE;
        }

        bool waitStop = false;
        for (uint32_t i = 0; i < quantity; i++) {
            if (!WAIT_TIMED(transConfig_.timeout_ms, I2C_CheckFlagState(i2cDev_, BIT_IC_STATUS_TFNF) == 0)) {
                reset();
                return SYSTEM_ERROR_I2C_FILL_DATA_TIMEOUT;
            }
            uint8_t data = 0xFF;
            if (txBuffer_.get(&data) != 1) {
                return SYSTEM_ERROR_NOT_ENOUGH_DATA;
            }
            if(i >= quantity - 1) {
                if (stop) {
                    // Generate stop signal
                    i2cDev_->IC_DATA_CMD = data | BIT_CTRL_IC_DATA_CMD_STOP;
                    waitStop = true;
                } else {
                    // Generate restart signal
                    i2cDev_->IC_DATA_CMD = data | BIT_CTRL_IC_DATA_CMD_RESTART;
                }
            } else {
                // The address will be sent before sending the first data byte
                i2cDev_->IC_DATA_CMD = data;
            }
            if (WAIT_TIMED_ROUTINE(transConfig_.timeout_ms, I2C_CheckFlagState(i2cDev_, BIT_IC_STATUS_TFE) == 0, checkAbrt) < 0) {
                reset();
                return SYSTEM_ERROR_I2C_TX_DATA_TIMEOUT;
            }
            if(checkAbrt()) {
                LOG(TRACE, "Abort: %08X", i2cDev_->IC_TX_ABRT_SOURCE);
                I2C_ClearAllINT(i2cDev_);
                return SYSTEM_ERROR_CANCELLED;
            }
        }
        if (waitStop && !WAIT_TIMED(transConfig_.timeout_ms, !stopDetected())) {
            reset();
            return SYSTEM_ERROR_I2C_STOP_TIMEOUT;
        }
        return SYSTEM_ERROR_NONE;
    }

    void flush() {
        txBuffer_.reset();
        rxBuffer_.reset();
    }

    void setOnRequestedCallback(void (*function)(void)) {
        onRequested_ = function;
    }

    void setOnReceivedCallback(void (*function)(int)) {
        onReceived_ = function;
    }

    int lock() {
        if (mutex_ && !hal_interrupt_is_isr()) {
            return os_mutex_recursive_lock(mutex_);
        }
        return SYSTEM_ERROR_INVALID_STATE;
    }

    int unlock() {
        if (mutex_ && !hal_interrupt_is_isr()) {
            return os_mutex_recursive_unlock(mutex_);
        }
        return SYSTEM_ERROR_INVALID_STATE;
    }

    static I2cClass* getInstance(hal_i2c_interface_t i2c) {
        static I2cClass i2cs[] = {
            { I2C0_DEV, SDA, SCL }
        };
        CHECK_TRUE(i2c < sizeof(i2cs) / sizeof(i2cs[0]), nullptr);
        return &i2cs[i2c];
    }

private:
    enum I2cSlaveStatus {
        I2C_SLAVE_STOPPED,
        I2C_SLAVE_STARTED,
        I2C_SLAVE_RESTARTED,
        I2C_SLAVE_TX,
        I2C_SLAVE_RX,
    };

    I2cClass(I2C_TypeDef* i2cDev, hal_pin_t sda, hal_pin_t scl)
            : i2cDev_(i2cDev),
              sdaPin_(sda),
              sclPin_(scl),
              configured_(false),
              slaveStatus_(I2C_SLAVE_STOPPED),
              heapBuffer_(false),
              i2cInitStruct_(),
              onRequested_(nullptr),
              onReceived_(nullptr),
              mutex_(nullptr),
              slaveRxCache_(nullptr),
              slaveRxCacheLen_(0),
              slaveRxCacheDepth_(HAL_PLATFORM_I2C_BUFFER_SIZE(HAL_I2C_INTERFACE1) * 2) {
    }
    ~I2cClass() = default;

    bool masterRestarted() {
        return (i2cDev_->IC_RAW_INTR_STAT & BIT_IC_INTR_STAT_R_START_DET) &&
                !(i2cDev_->IC_RAW_INTR_STAT & BIT_IC_INTR_STAT_R_STOP_DET) &&
                (i2cDev_->IC_RAW_INTR_STAT & BIT_IC_INTR_STAT_R_ACTIVITY);
    }

    int reEnableIfNeeded() {
        if (!masterRestarted()) {
            I2C_Cmd(i2cDev_, DISABLE);
            if (!WAIT_TIMED(transConfig_.timeout_ms, I2C_CheckFlagState(i2cDev_, BIT_IC_STATUS_ACTIVITY) == 1)) {
                reset();
                LOG_DEBUG(TRACE, "SYSTEM_ERROR_I2C_BUS_BUSY");
                return SYSTEM_ERROR_I2C_BUS_BUSY;
            }
            I2C_Cmd(i2cDev_, ENABLE);
        }
        clearIntStatus();
        return SYSTEM_ERROR_NONE;
    }

    bool stopDetected() {
        return (i2cDev_->IC_RAW_INTR_STAT & BIT_IC_INTR_STAT_R_STOP_DET) == BIT_IC_INTR_STAT_R_STOP_DET;
    }

    void clearIntStatus() {
        uint32_t temp = i2cDev_->IC_CLR_INTR;
        (void)temp;
    }

    bool isConfigValid(const hal_i2c_config_t* config) {
        if ((config == nullptr) || (config->rx_buffer == nullptr || config->rx_buffer_size == 0 ||
                config->tx_buffer == nullptr || config->tx_buffer_size == 0)) {
            return false;
        }
        return true;
    }

    bool checkAbrt() {
        if(I2C_GetRawINT(i2cDev_) & BIT_IC_RAW_INTR_STAT_TX_ABRT) {
            return true;
        }
        return false;
    }

    bool handleStartStop() {
        // These bits may be both set in single ISR
        // start | stop
        //   0   |   1    : STOPPED
        //   1   |   0    : RESTARTED
        //   1   |   1    : STOPPED -> STARTED (most likely happen) or RESTARTED -> STOPPED
        uint32_t intStatus = I2C_GetINT(i2cDev_);
        if ((intStatus & BIT_IC_INTR_STAT_R_START_DET) || (intStatus & BIT_IC_INTR_STAT_R_STOP_DET)) {
            I2C_ClearINT(i2cDev_, BIT_IC_INTR_STAT_R_START_DET);
            I2C_ClearINT(i2cDev_, BIT_IC_INTR_STAT_R_STOP_DET);
            if ((intStatus & BIT_IC_INTR_STAT_R_START_DET) && !(intStatus & BIT_IC_INTR_STAT_R_STOP_DET)) {
                slaveStatus_ = I2C_SLAVE_RESTARTED;
            } else if (intStatus & BIT_IC_INTR_STAT_R_START_DET) {
                slaveStatus_ = I2C_SLAVE_STARTED;
                // FIXME: It might be RESTARTED -> STOPPED
            } else {
                slaveStatus_ = I2C_SLAVE_STOPPED;
            }
            return true;
        }
        return false;
    }

    void slaveRxDone(size_t len, bool reset = false) {
        rxBuffer_.put(slaveRxCache_.get(), len);
        if (onReceived_) {
            onReceived_(rxBuffer_.data());
        }
        rxBuffer_.reset();
        if (reset) {
            memset(slaveRxCache_.get(), 0xFF, slaveRxCacheDepth_);
            slaveRxCacheLen_ = 0;
        } else {
            slaveRxCacheLen_ = slaveRxCacheLen_ - len;
            std::memmove(slaveRxCache_.get(), &slaveRxCache_[len], slaveRxCacheLen_);
        }
    }

    // Return:
    // 1: some of the read data belongs to next session
    // 0: all of the read data belongs the current session
    bool slaveRead() {
        uint16_t reportLen = 0;
        uint32_t intStatus;
        while (I2C_CheckFlagState(i2cDev_, (BIT_IC_STATUS_RFNE | BIT_IC_STATUS_RFF))) {
            slaveRxCache_[slaveRxCacheLen_++] = (uint8_t)I2C_ReceiveData(i2cDev_);
            intStatus = I2C_GetINT(i2cDev_);
            if ((intStatus & BIT_IC_INTR_STAT_R_START_DET) || (intStatus & BIT_IC_INTR_STAT_R_STOP_DET)) {
                reportLen = slaveRxCacheLen_;
                // We don't clear start/stop INT flag and keep reading data that is already in FIFO.
            }
        }
        if (reportLen > 0) {
            reportLen = std::min(rxBuffer_.size(), (size_t)slaveRxCacheLen_);
            slaveRxDone(reportLen);
            handleStartStop();
            return true;
        }
        return false;
    }

    void slaveWrite() {
        if (txBuffer_.data() <= 0) {
            if (onRequested_) {
                onRequested_();
            }
        }
        uint8_t data = 0xFF;
        if (txBuffer_.get(&data) == 1) {
            I2C_SlaveSend(i2cDev_, data);
        } else {
            I2C_SlaveSend(i2cDev_, 0xFF);
        }
    }

    // WARNNING: critical timing section.
    static void i2cSlaveIntHandler(void* context) {
        auto instance = (I2cClass*)context;
        uint32_t intStatus = I2C_GetINT(instance->i2cDev_);

        switch (instance->slaveStatus_) {
            case I2C_SLAVE_STOPPED: {
                instance->handleStartStop();
                break;
            }
            case I2C_SLAVE_STARTED:
            case I2C_SLAVE_RESTARTED: {
                if ((intStatus & BIT_IC_INTR_STAT_R_RX_FULL)) {
                    // Slave receiver
                    if (instance->slaveStatus_ == I2C_SLAVE_STARTED) {
                        memset(instance->slaveRxCache_.get(), 0xFF, instance->slaveRxCacheDepth_);
                        instance->slaveRxCacheLen_ = 0;
                    }
                    instance->slaveStatus_ = I2C_SLAVE_RX;
                    if (instance->slaveRead()) {
                        break;
                    }
                } else if (intStatus & BIT_IC_INTR_STAT_R_RD_REQ) {
                    // Slave transmitter
                    I2C_ClearINT(instance->i2cDev_, BIT_IC_INTR_STAT_R_RD_REQ);
                    instance->slaveStatus_ = I2C_SLAVE_TX;
                    instance->slaveWrite();
                }
                instance->handleStartStop();
                break;
            }
            case I2C_SLAVE_RX: {
                if ((intStatus & BIT_IC_INTR_STAT_R_RX_FULL)) {
                    if (instance->slaveRead()) {
                        break;
                    }
                }
                if (instance->handleStartStop()) {
                    if (instance->slaveRxCacheLen_ > 0 && instance->onReceived_) {
                        instance->slaveRxDone(instance->slaveRxCacheLen_, true); // it will discard the data that cannot be filled
                    }
                }
                break;
            }
            case I2C_SLAVE_TX: {
                if (intStatus & BIT_IC_INTR_STAT_R_RD_REQ) {
                    I2C_ClearINT(instance->i2cDev_, BIT_IC_INTR_STAT_R_RD_REQ);
                    instance->slaveWrite();
                }
                if (intStatus & BIT_IC_INTR_STAT_R_RX_DONE) {
                    I2C_ClearINT(instance->i2cDev_, BIT_IC_INTR_STAT_R_RX_DONE);
                    instance->txBuffer_.reset();
                }
                if (instance->handleStartStop()) {
                    instance->txBuffer_.reset();
                }
                break;
            }
            default: break;
        }
    }

    I2C_TypeDef* i2cDev_;
    hal_pin_t sdaPin_;
    hal_pin_t sclPin_;

    bool configured_;
    volatile hal_i2c_state_t state_;
    volatile uint8_t slaveStatus_;

    particle::services::RingBuffer<uint8_t> txBuffer_;
    particle::services::RingBuffer<uint8_t> rxBuffer_;
    bool heapBuffer_;

    I2C_InitTypeDef i2cInitStruct_;
    hal_i2c_transmission_config_t transConfig_;

    void (*onRequested_)(void);
    void (*onReceived_)(int);

    os_mutex_recursive_t mutex_;

    std::unique_ptr<uint8_t[]> slaveRxCache_;
    volatile uint16_t slaveRxCacheLen_;
    uint16_t slaveRxCacheDepth_;
};

class I2cLock {
public:
    I2cLock() = delete;

    I2cLock(I2cClass* i2c)
            : i2c_(i2c) {
        i2c_->lock();
    }

    ~I2cLock() {
        i2c_->unlock();
    }

private:
    I2cClass* i2c_;
};


int hal_i2c_init(hal_i2c_interface_t i2c, const hal_i2c_config_t* config) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_NOT_FOUND);
    return instance->init(config);
}

void hal_i2c_set_speed(hal_i2c_interface_t i2c, uint32_t speed, void* reserved) {
    auto instance = I2cClass::getInstance(i2c);
    if (!instance) {
        return;
    }
    I2cLock lk(instance);
    instance->setSpeed(speed);
}

void hal_i2c_stretch_clock(hal_i2c_interface_t i2c, bool stretch, void* reserved) {
    // always enabled
}

void hal_i2c_begin(hal_i2c_interface_t i2c, hal_i2c_mode_t mode, uint8_t address, void* reserved) {
    auto instance = I2cClass::getInstance(i2c);
    if (!instance) {
        return;
    }
    I2cLock lk(instance);
    instance->begin(mode, address);
}

void hal_i2c_end(hal_i2c_interface_t i2c, void* reserved) {
    auto instance = I2cClass::getInstance(i2c);
    if (!instance) {
        return;
    }
    I2cLock lk(instance);
    if (hal_i2c_is_enabled(i2c, nullptr)) {
        instance->end();
    }
}

uint32_t hal_i2c_request(hal_i2c_interface_t i2c, uint8_t address, uint8_t quantity, uint8_t stop, void* reserved) {
    hal_i2c_transmission_config_t conf = {
        .size = sizeof(hal_i2c_transmission_config_t),
        .version = 0,
        .address = address,
        .reserved = {0},
        .quantity = quantity,
        .timeout_ms = HAL_I2C_DEFAULT_TIMEOUT_MS,
        .flags = (uint32_t)(stop ? HAL_I2C_TRANSMISSION_FLAG_STOP : 0)
    };
    return hal_i2c_request_ex(i2c, &conf, nullptr);
}

int32_t hal_i2c_request_ex(hal_i2c_interface_t i2c, const hal_i2c_transmission_config_t* config, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), 0);
    if (!hal_i2c_is_enabled(i2c, nullptr) || !config) {
        return 0;
    }
    I2cLock lk(instance);
    return instance->requestFrom(config);
}

void hal_i2c_begin_transmission(hal_i2c_interface_t i2c, uint8_t address, const hal_i2c_transmission_config_t* config) {
    auto instance = I2cClass::getInstance(i2c);
    if (!instance) {
        return;
    }
    I2cLock lk(instance);
    if (!hal_i2c_is_enabled(i2c, nullptr)) {
        return;
    }
    instance->setConfigOrDefault(config, address);
}

uint8_t hal_i2c_end_transmission(hal_i2c_interface_t i2c, uint8_t stop, void* reserved) {
    return hal_i2c_compat_error_from(hal_i2c_end_transmission_ext(i2c, stop, reserved));
}

int hal_i2c_end_transmission_ext(hal_i2c_interface_t i2c, uint8_t stop, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_INVALID_ARGUMENT);
    if (!hal_i2c_is_enabled(i2c, nullptr)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    I2cLock lk(instance);
    return instance->endTransmission(stop);
}

uint32_t hal_i2c_write(hal_i2c_interface_t i2c, uint8_t data, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), 0);
    if (!hal_i2c_is_enabled(i2c, nullptr)) {
        return 0;
    }
    I2cLock lk(instance);
    return instance->write(data);
}

int32_t hal_i2c_available(hal_i2c_interface_t i2c, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_NOT_FOUND);
    I2cLock lk(instance);
    return instance->available();
}

int32_t hal_i2c_read(hal_i2c_interface_t i2c, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_NOT_FOUND);
    I2cLock lk(instance);
    return instance->read();
}

int32_t hal_i2c_peek(hal_i2c_interface_t i2c, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_NOT_FOUND);
    I2cLock lk(instance);
    return instance->peek();
}

void hal_i2c_flush(hal_i2c_interface_t i2c, void* reserved) {
    auto instance = I2cClass::getInstance(i2c);
    if (!instance) {
        return;
    }
    I2cLock lk(instance);
    instance->flush();
}

bool hal_i2c_is_enabled(hal_i2c_interface_t i2c, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), false);
    I2cLock lk(instance);
    return instance->isEnabled();
}

void hal_i2c_set_callback_on_received(hal_i2c_interface_t i2c, void (*function)(int),void* reserved) {
    auto instance = I2cClass::getInstance(i2c);
    if (!instance) {
        return;
    }
    I2cLock lk(instance);
    instance->setOnReceivedCallback(function);
}

void hal_i2c_set_callback_on_requested(hal_i2c_interface_t i2c, void (*function)(void),void* reserved) {
    auto instance = I2cClass::getInstance(i2c);
    if (!instance) {
        return;
    }
    I2cLock lk(instance);
    instance->setOnRequestedCallback(function);
}

void hal_i2c_enable_dma_mode(hal_i2c_interface_t i2c, bool enable, void* reserved) {
    // use DMA to send data by default
}

int hal_i2c_reset(hal_i2c_interface_t i2c, uint32_t reserved, void* reserved1) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_NOT_FOUND);
    I2cLock lk(instance);
    return instance->reset();
}

int32_t hal_i2c_lock(hal_i2c_interface_t i2c, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_NOT_FOUND);
    return instance->lock();
}

int32_t hal_i2c_unlock(hal_i2c_interface_t i2c, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_NOT_FOUND);
    return instance->unlock();
}

int hal_i2c_sleep(hal_i2c_interface_t i2c, bool sleep, void* reserved) {
    auto instance = CHECK_TRUE_RETURN(I2cClass::getInstance(i2c), SYSTEM_ERROR_NOT_FOUND);
    I2cLock lk(instance);
    if (sleep) {
        return instance->suspend();
    } else {
        return instance->restore();
    }
    return SYSTEM_ERROR_NONE;
}
