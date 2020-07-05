/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

// This code is partially based on OpenThread nRF52840-based alarm implementation

/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include "timer_hal.h"
#include <nrf_rtc.h>
#include <nrf_drv_clock.h>
#include <core_cmFunc.h>
#include "service_debug.h"
#include "hw_ticks.h"

namespace {

volatile uint32_t sOverflowCounter = 0; ///< Counter of RTC overflowCounter, incremented by 2 on each OVERFLOW event.
volatile uint8_t  sMutex = 0;           ///< Mutex for write access to @ref sOverflowCounter.
volatile uint64_t sTimeOffset = 0;  ///< Time overflowCounter to keep track of current time (in millisecond).
volatile bool     sEventPending = false;    ///< Timer fired and upper layer should be notified.
volatile uint32_t sDwtTickCountAtLastOverflow = 0; ///< DWT->CYCCNT value at the time latest overflow occurred
volatile uint32_t sTimerTicksAtLastOverflow = 0; ///< RTC ticks at the time latest overflow occured
volatile uint64_t sTimerMicrosBaseOffset = 0; ///< Base offset for Particle-specific microsecond counter

auto RTC_INSTANCE = NRF_RTC2;
constexpr auto RTC_IRQN = RTC2_IRQn;
constexpr auto RTC_IRQ_PRIORITY = 6;
constexpr auto RTC_USECS_PER_SEC = 1000000ULL;
constexpr auto US_PER_OVERFLOW = (512UL * RTC_USECS_PER_SEC);  ///< Time that has passed between overflow events. On full RTC speed, it occurs every 512 s.
#define RTC_IRQ_HANDLER RTC2_IRQHandler
constexpr auto RTC_FREQUENCY = 32768ULL;
constexpr auto RTC_FREQUENCY_US_PER_S_GCD_BITS = 6;

constexpr uint64_t divideAndCeil(uint64_t a, uint64_t b) {
    return ((a + b - 1) / b);
}

constexpr uint64_t ticksToTime(uint64_t ticks) {
    return divideAndCeil(ticks * (RTC_USECS_PER_SEC >> RTC_FREQUENCY_US_PER_S_GCD_BITS), RTC_FREQUENCY >> RTC_FREQUENCY_US_PER_S_GCD_BITS);
}

inline bool mutexGet() {
    do {
        volatile uint8_t mutexValue = __LDREXB(&sMutex);

        if (mutexValue) {
            __CLREX();
            return false;
        }
    } while (__STREXB(1, &sMutex));

    // Disable OVERFLOW interrupt to prevent lock-up in interrupt context while mutex is locked from lower priority
    // context and OVERFLOW event flag is stil up.
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

    __DMB();

    return true;
}

inline void mutexRelease() {
    // Re-enable OVERFLOW interrupt.
    nrf_rtc_int_enable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

    __DMB();
    sMutex = 0;
}

uint32_t getRtcCounter() {
    return nrf_rtc_counter_get(RTC_INSTANCE);
}

uint32_t getOverflowCounter() {
    uint32_t overflowCounter;

    // Get mutual access for writing to sOverflowCounter variable.
    if (mutexGet()) {
        bool increasing = false;

        // Check if interrupt was handled already.
        if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW)) {
            sOverflowCounter++;
            increasing = true;

            __DMB();

            // Mark that interrupt was handled.
            nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);

            // Result should be incremented. sOverflowCounter will be incremented after mutex is released.
        } else {
            // Either overflow handling is not needed OR we acquired the mutex just after it was released.
            // Overflow is handled after mutex is released, but it cannot be assured that sOverflowCounter
            // was incremented for the second time, so we increment the result here.
        }

        overflowCounter = (sOverflowCounter + 1) / 2;

        mutexRelease();

        if (increasing) {
            // It's virtually impossible that overflow event is pending again before next instruction is performed. It
            // is an error condition.
            SPARK_ASSERT(sOverflowCounter & 0x01);

            // Increment the counter for the second time, to allow instructions from other context get correct value of
            // the counter.
            sOverflowCounter++;

            // Store current DWT->CYCCNT and RTC counter value
            int pri = __get_PRIMASK();
            __disable_irq();
            sTimerTicksAtLastOverflow = getRtcCounter();
            sDwtTickCountAtLastOverflow = DWT->CYCCNT;
            __set_PRIMASK(pri);
        }
    } else {
        // Failed to acquire mutex.
        if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW) || (sOverflowCounter & 0x01)) {
            // Lower priority context is currently incrementing sOverflowCounter variable.
            overflowCounter = (sOverflowCounter + 2) / 2;
        } else {
            // Lower priority context has already incremented sOverflowCounter variable or incrementing is not needed
            // now.
            overflowCounter = sOverflowCounter / 2;
        }
    }

    return overflowCounter;
}

uint64_t getCurrentTimeWithTicks(uint32_t* dwtTicks, uint32_t* elapsedRtcTicks) {
    uint32_t offset1 = getOverflowCounter();
    __DMB();

    uint32_t rtcValue = getRtcCounter();
    *dwtTicks = sDwtTickCountAtLastOverflow;
    uint32_t overflowTicks = sTimerTicksAtLastOverflow;
    __DMB();

    uint32_t offset2 = getOverflowCounter();

    if (offset1 != offset2) {
        // Overflow occured between the calls
        rtcValue = getRtcCounter();
        *dwtTicks = sDwtTickCountAtLastOverflow;
        overflowTicks = sTimerTicksAtLastOverflow;
    }
    *elapsedRtcTicks = (rtcValue - overflowTicks);
    return (uint64_t)offset2 * US_PER_OVERFLOW + ticksToTime(rtcValue);
}

} // anonymous

// Particle-specific
int hal_timer_init(const hal_timer_init_config_t* conf) {
    if (conf) {
        sTimerMicrosBaseOffset = conf->base_clock_offset;
    }

    sOverflowCounter = 0;
    sMutex           = 0;
    sTimeOffset      = 0;
    sDwtTickCountAtLastOverflow = 0;
    sTimerTicksAtLastOverflow = 0;

    // Setup low frequency clock.
    nrf_drv_clock_lfclk_request(NULL);

    while (!nrf_drv_clock_lfclk_is_running()) {
        ;
    }

    // Setup RTC timer.
    NVIC_SetPriority(RTC_IRQN, RTC_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(RTC_IRQN);
    NVIC_EnableIRQ(RTC_IRQN);

    // This is a workaround for a situation we've seen happen:
    // despite the fact that the RTC should be stopped and prescaler should be R/W,
    // we've seen it stay R/O for some reason :|
    while (rtc_prescaler_get(RTC_INSTANCE) != 0) {
        nrf_rtc_prescaler_set(RTC_INSTANCE, 0);
        __DSB();
        __ISB();
    }

    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);
    nrf_rtc_event_enable(RTC_INSTANCE, RTC_EVTEN_OVRFLW_Msk);
    nrf_rtc_int_enable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_0);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_COMPARE0_Msk);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE0_MASK);

    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_1);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_COMPARE1_Msk);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE1_MASK);

    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_2);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_COMPARE2_Msk);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE2_MASK);

    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_3);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_COMPARE3_Msk);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE3_MASK);

    int pri = __get_PRIMASK();
    __disable_irq();
    nrf_rtc_task_trigger(RTC_INSTANCE, NRF_RTC_TASK_START);
    DWT->CYCCNT = 0;
    __set_PRIMASK(pri);

    return 0;
}

int hal_timer_deinit(void* reserved) {
    nrf_rtc_task_trigger(RTC_INSTANCE, NRF_RTC_TASK_STOP);

    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_TICK_MASK);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE0_MASK);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE1_MASK);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE2_MASK);
    nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_COMPARE3_MASK);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_OVRFLW_Msk);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTEN_TICK_Msk);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTENSET_COMPARE0_Msk);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTENSET_COMPARE1_Msk);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTENSET_COMPARE2_Msk);
    nrf_rtc_event_disable(RTC_INSTANCE, RTC_EVTENSET_COMPARE3_Msk);
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW);
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_TICK);
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_0);
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_1);
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_2);
    nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_3);

    NVIC_DisableIRQ(RTC_IRQN);
    NVIC_ClearPendingIRQ(RTC_IRQN);

    nrf_drv_clock_lfclk_release();

    return 0;
}

extern "C" void RTC_IRQ_HANDLER(void) {
    // Handle overflow.
    if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_OVERFLOW)) {
        // Disable OVERFLOW interrupt to prevent lock-up in interrupt context while mutex is locked from lower priority
        // context and OVERFLOW event flag is stil up. OVERFLOW interrupt will be re-enabled when mutex is released -
        // either from this handler, or from lower priority context, that locked the mutex.
        nrf_rtc_int_disable(RTC_INSTANCE, NRF_RTC_INT_OVERFLOW_MASK);

        // Handle OVERFLOW event by reading current value of overflow counter.
        (void)getOverflowCounter();
    } else if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_TICK)) {
        nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_TICK);
    } else if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_0)) {
        nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_0);
    } else if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_1)) {
        nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_1);
    } else if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_2)) {
        nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_2);
    } else if (nrf_rtc_event_pending(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_3)) {
        nrf_rtc_event_clear(RTC_INSTANCE, NRF_RTC_EVENT_COMPARE_3);
    }
}

uint64_t hal_timer_micros(void* reserved) {
    // Extends the resolution from 31us to about 5us using DWT->CYCCNT
    // Make sure that sDwtTickCountAtLastOverflow and current timer values are fetched atomically
    uint32_t lastOverflowDwtTicks;
    uint32_t elapsedRtcTicksSinceOverflow;
    uint64_t curUs = getCurrentTimeWithTicks(&lastOverflowDwtTicks, &elapsedRtcTicksSinceOverflow);

    int usTicks = SYSTEM_US_TICKS;

    uint64_t elapsedTicks = ticksToTime(elapsedRtcTicksSinceOverflow) * usTicks;
    uint32_t syncTicks = (uint32_t)((uint64_t)lastOverflowDwtTicks + elapsedTicks);
    uint32_t tickDiff = DWT->CYCCNT - syncTicks;
    int64_t tickDiffFinal;
    if (tickDiff > (US_PER_OVERFLOW / 10) * usTicks) {
        tickDiff = 0xffffffff - tickDiff;
        tickDiffFinal = -((int64_t)tickDiff);
    } else {
        tickDiffFinal = tickDiff;
    }

    return sTimerMicrosBaseOffset + ((int64_t)curUs + ((tickDiffFinal) / usTicks));
}

uint64_t hal_timer_millis(void* reserved) {
    return hal_timer_micros(nullptr) / 1000ULL;
}

system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    return (system_tick_t)hal_timer_micros(nullptr);
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    return (system_tick_t)hal_timer_millis(nullptr);
}
