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

#pragma once

#include "spi_hal.h"
#include "service_debug.h"

namespace particle {

class SpiConfigurationLock {
public:
    SpiConfigurationLock(hal_spi_interface_t spi)
            : spi_{spi},
              conf_{},
              spiInfoCache_{},
              changed_{false},
              locked_{0} {
    }
    SpiConfigurationLock(hal_spi_interface_t spi, const hal_spi_info_t& conf)
            : SpiConfigurationLock(spi) {
        setConfiguration(conf);
    }

    SpiConfigurationLock() = delete;
    SpiConfigurationLock(const SpiConfigurationLock&) = delete;
    SpiConfigurationLock& operator=(const SpiConfigurationLock&) = delete;

    ~SpiConfigurationLock() = default;

    void lock() {
        // Make sure that the configuration has been set
        SPARK_ASSERT(conf_.version);

        if (!hal_spi_is_enabled(spi_)) {
            hal_spi_init(spi_);
            // Make sure the SPI peripheral is initialized with default settings
            hal_spi_acquire(spi_, nullptr);
            hal_spi_begin_ext(spi_, SPI_MODE_MASTER, PIN_INVALID, nullptr);
        } else {
            hal_spi_acquire(spi_, nullptr);
        }

        if (locked_++) {
            return;
        }

        spiInfoCache_.version = HAL_SPI_INFO_VERSION;
        hal_spi_info(spi_, &spiInfoCache_, nullptr);

        // Optimize a bit and not reconfigure unnecessarily
        if (!settingsEqual(conf_, spiInfoCache_)) {
            applySettings(conf_);
            changed_ = true;
        }
    }

    void unlock() {
        if (!conf_.version) {
            return;
        }

        if (!--locked_ && changed_) {
            applySettings(spiInfoCache_);
            changed_ = false;
        }
        hal_spi_release(spi_, nullptr);
    }

    hal_spi_interface_t interface() const {
        return spi_;
    }

    hal_spi_info_t conf() const {
        return conf_;
    }

    void setConfiguration(const hal_spi_info_t& conf) {
        conf_ = conf;
    }

private:
    static bool settingsEqual(const hal_spi_info_t& lh, const hal_spi_info_t& rh) {
        if (lh.default_settings && rh.default_settings) {
            return true;
        }

        if (lh.mode == rh.mode &&
                lh.default_settings == rh.default_settings &&
                lh.clock == rh.clock &&
                lh.bit_order == rh.bit_order &&
                lh.data_mode == rh.data_mode &&
                (lh.mode == SPI_MODE_MASTER || (lh.mode == SPI_MODE_SLAVE && lh.ss_pin == rh.ss_pin))) {
            return true;
        }

        return false;
    }

    void applySettings(const hal_spi_info_t& conf) {
        if (spiInfoCache_.mode != conf.mode) {
            // FIXME: this shouldn't probably be supported
            hal_spi_begin_ext(spi_, conf.mode, conf.ss_pin, nullptr);
        }

        auto div = hal_spi_get_clock_divider(spi_, conf_.clock, nullptr);
        if (div < 0) {
            // Default to slowest just in case
            div = SPI_CLOCK_DIV256;
        }

        hal_spi_set_settings(spi_, conf.default_settings, (uint8_t)div, conf.bit_order, conf.data_mode, nullptr);
    }

    hal_spi_interface_t spi_;
    hal_spi_info_t conf_;
    hal_spi_info_t spiInfoCache_;
    bool changed_;
    uint32_t locked_;
};

}