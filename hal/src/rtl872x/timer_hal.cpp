/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include <cstdint>
#include "timer_hal.h"
#include "service_debug.h"
#include "system_error.h"
#include "check.h"
#include "hw_ticks.h"
extern "C" {
#include "rtl8721d.h"
}

namespace {

static constexpr int TIMER_INDEX = 4;                   // Use TIM4
static constexpr int TIMER_PRESCALAR = 39;              // Input clock = 40MHz_XTAL / (39 + 1) = 1MHz
static constexpr int TIMER_PERIOD = 65535;              // Period = (65535 + 1) = 65536us
static constexpr int TIMER_OVERFLOW_US = 65536;
static constexpr int TIMER_COUNTER_US_THRESHOLD = 1;    // 1us = 1

class RtlTimer {
public:
    RtlTimer() : 
            enabled_(false),
            usSystemTimeBase_(0) {
    }
    ~RtlTimer() = default;

    int init() {
        CHECK_TRUE(enabled_ == false, SYSTEM_ERROR_INVALID_STATE);
        RCC_PeriphClockCmd(APBPeriph_GTIMER, APBPeriph_GTIMER_CLOCK, ENABLE);
        RTIM_TimeBaseInitTypeDef initStruct = {};
        RTIM_TimeBaseStructInit(&initStruct);
        initStruct.TIM_Idx = TIMER_INDEX;
        initStruct.TIM_Prescaler = TIMER_PRESCALAR;
        initStruct.TIM_Period = TIMER_PERIOD;
        RTIM_TimeBaseInit(TIMx[TIMER_INDEX], &initStruct, TIMx_irq[TIMER_INDEX], interruptHandler, (uint32_t)this);
        RTIM_INTConfig(TIMx[TIMER_INDEX], TIM_IT_Update, ENABLE);
        enabled_ = true;
        return SYSTEM_ERROR_NONE;
    }

    int deinit() {
        CHECK_TRUE(enabled_, SYSTEM_ERROR_INVALID_STATE);
        RTIM_INTConfig(TIMx[TIMER_INDEX], TIM_IT_Update, DISABLE);
        RTIM_Cmd(TIMx[TIMER_INDEX], DISABLE);
        RTIM_DeInit(TIMx[TIMER_INDEX]);
        enabled_ = false;
        return SYSTEM_ERROR_NONE;
    }

    int start() {
        CHECK_TRUE(enabled_, SYSTEM_ERROR_INVALID_STATE);
        RTIM_Cmd(TIMx[TIMER_INDEX], ENABLE);
        return SYSTEM_ERROR_NONE;
    }

    int stop() {
        CHECK_TRUE(enabled_, SYSTEM_ERROR_INVALID_STATE);
        RTIM_Cmd(TIMx[TIMER_INDEX], DISABLE);
        return SYSTEM_ERROR_NONE;
    }

    int setTime(uint64_t time) {
        CHECK_TRUE(enabled_, SYSTEM_ERROR_INVALID_STATE);
        usSystemTimeBase_ = time;
        resetCounter();
    }

    uint64_t getTime() const {
        return usSystemTimeBase_ + RTIM_GetCount(TIMx[TIMER_INDEX]) / TIMER_COUNTER_US_THRESHOLD;
    }

    void interruptHandlerImpl() {
        RTIM_INTClear(TIMx[TIMER_INDEX]);
        usSystemTimeBase_ += TIMER_OVERFLOW_US;
    }

    static uint32_t interruptHandler(void* context) {
        RtlTimer* timerInstance = (RtlTimer*)context;
        timerInstance->interruptHandlerImpl();
        return SYSTEM_ERROR_NONE;
    }

private:
    void resetCounter() {
        RTIM_Reset(TIMx[TIMER_INDEX]);
    }

private:
    bool enabled_;
    uint64_t usSystemTimeBase_;
};

RtlTimer* getInstance() {
    static RtlTimer timer;
    return &timer;
}

};  // Anonymouse

// Particle-specific
int hal_timer_init(const hal_timer_init_config_t* conf) {
    auto timerInstance = getInstance();
    CHECK(timerInstance->init());
    return timerInstance->start();
}

int hal_timer_deinit(void* reserved) {
    auto timerInstance = getInstance();
    return timerInstance->deinit();
}

uint64_t hal_timer_micros(void* reserved) {
    auto timerInstance = getInstance();
    return timerInstance->getTime();
}

uint64_t hal_timer_millis(void* reserved) {
    return hal_timer_micros(nullptr) / 1000ULL;
}

system_tick_t HAL_Timer_Get_Micro_Seconds(void) {
    return (system_tick_t)hal_timer_micros(nullptr);
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void) {
    return (system_tick_t)hal_timer_millis(nullptr);
}
