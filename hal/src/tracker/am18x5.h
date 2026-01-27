/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#ifndef AM18X5_H
#define AM18X5_H

#include "hal_platform.h"

#include "time.h"
#include "concurrent_hal.h"
#include "i2c_hal.h"
#include "gpio_hal.h"
#include "enumclass.h"

#define HAL_AM18X5_CONFIG_VERSION               1
#define HAL_EXRTC_ID_STR_LEN                    18

namespace particle {

enum class Am18x5Register : uint8_t {
    // Time and Date Registers
    HUNDREDTHS                      = 0x00,
    SECONDS                         = 0x01,
    MINUTES                         = 0x02,
    HOURS                           = 0x03,
    DATE                            = 0x04,
    MONTHS                          = 0x05,
    YEARS                           = 0x06,
    WEEKDAY                         = 0x07,
    // Alarm Registers
    HUNDREDTHS_ALARM                = 0x08,
    SECONDS_ALARM                   = 0x09,
    MINUTES_ALARM                   = 0x0A,
    HOURS_ALARM                     = 0x0B,
    DATE_ALARM                      = 0x0C,
    MONTHS_ALARM                    = 0x0D,
    WEEKDAY_ALARM                   = 0x0E,
    // Configuration Registers
    STATUS                          = 0x0F,
    CONTROL1                        = 0x10,
    CONTROL2                        = 0x11,
    INT_MASK                        = 0x12,
    SQW                             = 0x13,
    // Calibration Registers
    CAL_XT                          = 0x14,
    CAL_RC_HI                       = 0x15,
    CAL_RC_LO                       = 0x16,
    // Sleep Control Register
    SLEEP_CONTROL                   = 0x17,
    // Timer Registers
    TIMER_CONTROL                   = 0x18,
    TIMER                           = 0x19,
    TIMER_INITIAL                   = 0x1A,
    WDT                             = 0x1B,
    // Oscillator Registers
    OSC_CONTROL                     = 0x1C,
    OSC_STATUS                      = 0x1D,
    // Miscellaneous Registers
    CONFIG_KEY                      = 0x1F,
    // Analog Control Registers
    TRICKLE                         = 0x20,
    BREF_CONTROL                    = 0x21,
    AFCTRL                          = 0x26,
    BATMODE_IO                      = 0x27,
    ANALOG_STATUS                   = 0X2F,
    OUTPUT_CTRL                     = 0x30,
    // ID Registers
    ID0                             = 0x28,
    ID1                             = 0x29,
    ID2                             = 0x2A,
    ID3                             = 0x2B,
    ID4                             = 0x2C,
    ID5                             = 0x2D,
    ID6                             = 0x2E,
    // RAM Registers
    EXTENSION_RAM_ADDRESS           = 0x3F
};

enum class HourFormat : uint8_t {
    HOUR24,
    HOUR12_AM,
    HOUR12_PM
};

enum class Weekday : uint8_t {
    MONDAY = 0,
    TUESDAY = 1,
    WEDNESDAY = 2,
    THURSDAY = 3,
    FRIDAY = 4,
    SATURDAY = 5,
    SUNDAY = 6
};

enum class Am18x5Oscillator : uint8_t {
    INTERNAL_RC = 0x00,
    EXTERNAL_CRYSTAL = 0x01
};

enum class Am18x5WatchdogFrequency : uint8_t {
    HZ_16 = 0x00,
    HZ_4 = 0x01,
    HZ_1 = 0x02,
    HZ_1_4 = 0x03
};

enum class Am18x5TimerFrequency : uint8_t {
    HZ_4096 = 0x00,
    HZ_64 = 0x01,
    HZ_1 = 0x02,
    HZ_1_60 = 0x03
};

enum class Am18x5ExtiPolarity : uint8_t {
    NONE = 0,
    FALLING = 1,
    RISING = 2
};

enum class Am18x5SqwFrequency : uint8_t {
    HZ_32768 = 0x01,
    HZ_8192 = 0x02,
    HZ_4096 = 0x03,
    HZ_2048 = 0x04,
    HZ_1024 = 0x05,
    HZ_512 = 0x06,
    HZ_256 = 0x07,
    HZ_128 = 0x08,
    HZ_64 = 0x09,
    HZ_32 = 0x0A,
    HZ_16 = 0x0B,
    HZ_8 = 0x0C,
    HZ_4 = 0x0D,
    HZ_2 = 0x0E,
    HZ_1 = 0x0F,
    // TODO: Add more values
};

enum class Am18x5AutoCalibration : uint8_t {
    AUTO_CAL_DISABLE = 0x00,
    AUTO_CAL_EVERY_1024_SEC = 0x02,
    AUTO_CAL_EVERY_512_SEC = 0x03,
};

enum Am18x5OscEvent : uint8_t {
    XT_OSC_FAILURE = 0x01,
    AUTO_CAL_FAILURE = 0x02
};

typedef void (*Am18x5OscEventHandler)(uint8_t event, void* context);

typedef struct hal_am18x5_config_t {
    uint16_t version;
    uint16_t size;
    uint8_t default_rtc;
    uint8_t wdi_pin;
    uint8_t int_pin;
    hal_i2c_interface_t i2c_if;
    uint8_t rc_fallback;
    uint8_t rc_on_battery;
    Am18x5Oscillator osc_src;
    int8_t osc_cal_xt;
    uint8_t clk_out_en;
    Am18x5SqwFrequency clk_out_freq;
    Am18x5AutoCalibration auto_calibration;
} __attribute__((packed)) hal_am18x5_config_t;

typedef struct hal_am18x5_sleep_config_t {
    uint16_t version;
    uint16_t size;
    Am18x5ExtiPolarity exti_polarity;
    bool exti_trigger_latched;
    system_tick_t duration; // in seconds
} __attribute__((packed)) hal_am18x5_sleep_config_t;

class Am18x5 {
public:
    typedef void (*AlarmHandler)(void* context);

    int setConfig(const hal_am18x5_config_t* config);
    int getConfig(hal_am18x5_config_t* config);

    bool isDefault() const;
    bool isPresent() const { return initialized_; }
    int begin();
    int end();
    int sync();
    int reset();

    int setTime(const struct timeval* tv) const;
    int getTime(struct timeval* tv) const;
    bool isTimeValid(struct timeval* tv = nullptr) const;

    int setAlarm(bool enable, uint32_t flags = 0, const struct timeval* tv = nullptr, AlarmHandler handler = nullptr, void* context = nullptr);
    int getAlarm(struct timeval* tv) const;

    int enableWatchdog(system_tick_t ms);
    int disableWatchdog() const;
    int feedWatchdog() const;
    void getWatchdogLimits(system_tick_t* low, system_tick_t* high) const;
    bool isWatchdogStarted() const;

    // If multiple wakeup sources are configured, sleep mode exits on one wakeup source satisfied.
    int sleep(const hal_am18x5_sleep_config_t* config);

    int getIdString(char* id, size_t len) const;

    int getOscillatorSource(Am18x5Oscillator* source);
    int onOscillatorEvent(uint8_t events, Am18x5OscEventHandler handler, void* context);

    int lock();
    int unlock();

    static Am18x5& getInstance();

private:
    Am18x5();
    ~Am18x5();

    int detect();
    int applyConfig();

    int setPsw(bool val) const; // This is dangerous, make it private for now!

    int setHundredths(uint8_t hundredths) const;
    int setSeconds(uint8_t seconds) const;
    int setMinutes(uint8_t minutes) const;
    int setHours(uint8_t hours, HourFormat format) const;
    int setDate(uint8_t datee) const;
    int setMonths(uint8_t months) const;
    int setYears(uint8_t years) const;
    int setWeekday(uint8_t weekday) const;

    int getHundredths() const;
    int getSeconds() const;
    int getMinutes() const;
    int getHours(HourFormat* format) const;
    int getDate() const;
    int getMonths() const;
    int getYears() const;
    int getWeekday() const;

    /*
     * The XT oscillator calibration value is determined by the following process:
     * 1. Set the OFFSETX, CMDX and XTCAL register fields to 0 to ensure calibration is not occurring.
     * 2. Select the XT oscillator by setting the OSEL bit to 0.
     * 3. Configure a 32768 Hz frequency square wave output on one of the output pins.
     * 4. Precisely measure the exact frequency, Fmeas, at the output pin in Hz.
     * 5. Compute the adjustment value required in ppm as ((32768 – Fmeas)*1000000)/32768 = PAdj
     * 6. Compute the adjustment value in steps as PAdj/(1000000/2^19) = PAdj/(1.90735) = Adj
     * 7. If Adj < -320, the XT frequency is too high to be calibrated
     * 8. Else if Adj < -256, set XTCAL = 3, CMDX = 1, OFFSETX = (Adj + 192) / 2
     * 9. Else if Adj < -192, set XTCAL = 3, CMDX = 0, OFFSETX = Adj + 192
     * 10. Else if Adj < -128, set XTCAL = 2, CMDX = 0, OFFSETX = Adj + 128
     * 11. Else if Adj < -64, set XTCAL = 1, CMDX = 0, OFFSETX = Adj + 64
     * 12. Else if Adj < 64, set XTCAL = 0, CMDX = 0, OFFSETX = Adj
     * 13. Else if Adj < 128, set XTCAL = 0, CMDX = 1, OFFSETX = Adj/2
     * 14. Else the XT frequency is too low to be calibrated
     */
    int xtOscillatorDigitalCalibration(int adjVal) const;

    int enableClkOut(Am18x5SqwFrequency freq);
    int disableClkOut();

    int writeRegister(const Am18x5Register reg, uint8_t val, bool bcd = false, bool rw = false, uint8_t mask = 0xFF, uint8_t shift = 0) const;
    int writeContinuousRegisters(const Am18x5Register start_reg, const uint8_t* buff, size_t len) const;
    int readRegister(const Am18x5Register reg, uint8_t* const val, bool bcd = false, uint8_t mask = 0xFF, uint8_t shift = 0) const;
    int readContinuousRegisters(const Am18x5Register start_reg, uint8_t* buff, size_t len) const;
    static os_thread_return_t exRtcInterruptHandleThread(void* param);

    static constexpr uint16_t AM1805_PART_NUMBER = 0x1805;
    static constexpr uint8_t AM18X5_I2C_ADDR = 0x69;
    static constexpr uint8_t AM18X5_ID_COUNT = 7;
    static constexpr time_t UNIX_TIME_20180101000000 = 1514764800UL;  // 2018/01/01 00:00:00

    bool initialized_;
    bool disableI2cOnEnded_;
    uint8_t alarmYear_;
    AlarmHandler alarmHandler_;
    void* alarmHandlerContext_;
    os_thread_t exRtcWorkerThread_;
    os_queue_t exRtcWorkerSemaphore_;
    bool exRtcWorkerThreadExit_;
    hal_am18x5_config_t config_;
    uint8_t watchdogValue_;
    uint8_t ids_[AM18X5_ID_COUNT];

    uint8_t subscribedOscEvents_;
    Am18x5OscEventHandler oscEventHandler_;
    void* oscEventHandlerContext_;
}; // class Am18x5


class Am18x5Lock {
public:
    Am18x5Lock()
            : locked_(false) {
        lock();
    }

    Am18x5Lock(Am18x5Lock&& lock)
            : locked_(lock.locked_) {
        lock.locked_ = false;
    }

    Am18x5Lock(const Am18x5Lock&) = delete;
    Am18x5Lock& operator=(const Am18x5Lock&) = delete;

    ~Am18x5Lock() {
        if (locked_) {
            unlock();
        }
    }

    void lock() {
        Am18x5::getInstance().lock();
        locked_ = true;
    }

    void unlock() {
        Am18x5::getInstance().unlock();
        locked_ = false;
    }

private:
    bool locked_;
};

} // namespace particle

#endif // AM18X5_H