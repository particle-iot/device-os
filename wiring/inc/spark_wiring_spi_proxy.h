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

namespace particle {

template <hal_spi_interface_t Interface>
class SpiProxy {
public:
    static SPIClass& instance() {
        static SPIClass instance(Interface);
        return instance;
    }

    hal_spi_interface_t interface() {
         return instance().interface();
    }
    void begin() {
        instance().begin();
    }
    void begin(uint16_t ss_pin) {
        instance().begin(ss_pin);
    }
    void begin(hal_spi_mode_t mode, uint16_t ss_pin = SPI_DEFAULT_SS) {
        instance().begin(mode, ss_pin);
    }
    void end() {
        instance().end();
    }
    void setBitOrder(uint8_t order) {
        instance().setBitOrder(order);
    }
    void setDataMode(uint8_t mode) {
        instance().setDataMode(mode);
    }
    static void usingInterrupt(uint8_t arg) {
        instance().usingInterrupt(arg);
    }
    int32_t beginTransaction() {
        return instance().beginTransaction();
    }
    int32_t beginTransaction(const particle::SPISettings& settings) {
        return instance().beginTransaction(settings);
    }
    void endTransaction() {
        instance().endTransaction();
    }
    void setClockDividerReference(unsigned value, unsigned scale=HZ) {
        instance().setClockDividerReference(value, scale);
    }
    void setClockDivider(uint8_t divider) {
        instance().setClockDivider(divider);
    }
    unsigned setClockSpeed(unsigned value, unsigned scale=HZ) {
        return instance().setClockSpeed(value, scale);
    }
    static void computeClockDivider(unsigned reference, unsigned targetSpeed, uint8_t& divider, unsigned& clock) {
        instance().computeClockDivider(reference, targetSpeed, divider, clock);
    }
    byte transfer(byte data) {
        return instance().transfer(data);
    }
    void transfer(const void* tx_buffer, void* rx_buffer, size_t length, wiring_spi_dma_transfercomplete_callback_t user_callback) {
        instance().transfer(tx_buffer, rx_buffer, length, user_callback);
    }
    void attachInterrupt() {
        instance().attachInterrupt();
    }
    void detachInterrupt() {
        instance().detachInterrupt();
    }
    bool isEnabled(void) {
        return instance().isEnabled();
    }
    void onSelect(wiring_spi_select_callback_t user_callback) {
        instance().onSelect(user_callback);
    }
    void transferCancel() {
        instance().transferCancel();
    }
    int32_t available() {
        return instance().available();
    }
    bool trylock() {
        return instance().trylock();
    }
    int lock() {
        return instance().lock();
    }
    void unlock() {
        return instance().unlock();
    }

    operator SPIClass&() {
        return instance();
    }

    SPIClass* operator&() {
        return &instance();
    }
};

} // particle
