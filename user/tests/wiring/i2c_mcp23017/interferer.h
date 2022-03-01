#ifndef INTERFERER_H_
#define INTERFERER_H_

#include "application.h"

#if HAL_PLATFORM_NRF52840
#include "nrf_timer.h"
#endif // HAL_PLATFORM_NRF52840

struct HighPriorityInterruptInterferer {
    HighPriorityInterruptInterferer() {
#if (PLATFORM_ID>=6 && PLATFORM_ID<=10)
        // Enable some high priority interrupt to run interference
        pinMode(D0, OUTPUT);
        // D0 uses TIM2 and channel 2 equally on Core and Photon/Electron
        // Run at 4khz (T=250us)
        analogWrite(D0, 1, 4000);
        // Set higher than SysTick_IRQn priority for TIM4_IRQn
        NVIC_SetPriority(TIM4_IRQn, 3);
        TIM_ITConfig(TIM4, TIM_IT_CC2, ENABLE);
        NVIC_EnableIRQ(TIM4_IRQn);
        attachSystemInterrupt(SysInterrupt_TIM4_IRQ, [&] {
            // Do some work up to 200 microseconds
            delayMicroseconds(rand_r(&HighPriorityInterruptInterferer::randSeed) % 200);
            TIM_ClearITPendingBit(TIM4, TIM_IT_CC2);
        });
#elif HAL_PLATFORM_NRF52840
        nrf_timer_mode_set(NRF_TIMER2, NRF_TIMER_MODE_TIMER);
        nrf_timer_task_trigger(NRF_TIMER2, NRF_TIMER_TASK_STOP);
        nrf_timer_task_trigger(NRF_TIMER2, NRF_TIMER_TASK_CLEAR);
        nrf_timer_frequency_set(NRF_TIMER2, NRF_TIMER_FREQ_500kHz);
        nrf_timer_bit_width_set(NRF_TIMER2, NRF_TIMER_BIT_WIDTH_32);
        nrf_timer_cc_write(NRF_TIMER2, NRF_TIMER_CC_CHANNEL0, nrf_timer_us_to_ticks(250, NRF_TIMER_FREQ_500kHz));
        nrf_timer_int_disable(NRF_TIMER2, NRF_TIMER_INT_COMPARE0_MASK |
                NRF_TIMER_INT_COMPARE1_MASK |
                NRF_TIMER_INT_COMPARE2_MASK |
                NRF_TIMER_INT_COMPARE3_MASK |
                NRF_TIMER_INT_COMPARE4_MASK |
                NRF_TIMER_INT_COMPARE5_MASK);
        nrf_timer_int_enable(NRF_TIMER2, NRF_TIMER_INT_COMPARE0_MASK);
        nrf_timer_event_clear(NRF_TIMER2, NRF_TIMER_EVENT_COMPARE0);
        nrf_timer_shorts_enable(NRF_TIMER2, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
        NVIC_ClearPendingIRQ(TIMER2_IRQn);
        NVIC_SetPriority(TIMER2_IRQn, 2);
        attachInterruptDirect(TIMER2_IRQn, [](void) -> void {
            // Do some work up to 200 microseconds
            delayMicroseconds(rand_r(&HighPriorityInterruptInterferer::randSeed) % 200);
            nrf_timer_event_clear(NRF_TIMER2, NRF_TIMER_EVENT_COMPARE0);
        }, true);
        nrf_timer_task_trigger(NRF_TIMER2, NRF_TIMER_TASK_START);
#else
#warning "HighPriorityInterruptInterferer is not implemented for this platform"
#endif
    };

    ~HighPriorityInterruptInterferer() {
#if (PLATFORM_ID>=6 && PLATFORM_ID<=10)
        NVIC_DisableIRQ(TIM4_IRQn);
        detachSystemInterrupt(SysInterrupt_TIM4_IRQ);
        TIM_ITConfig(TIM4, TIM_IT_CC2, DISABLE);
        digitalWrite(D0, HIGH);
#elif HAL_PLATFORM_NRF52840
        nrf_timer_task_trigger(NRF_TIMER2, NRF_TIMER_TASK_STOP);
        detachInterruptDirect(TIMER2_IRQn, true);
        NVIC_ClearPendingIRQ(TIMER2_IRQn);
#endif
    };

    static unsigned int randSeed;
};

struct ContextSwitchBlockingInteferer {
    ContextSwitchBlockingInteferer() {
        exit_ = 0;
        bool& exitFlag = exit_;
        thread_ = new Thread("", [&exitFlag]{
            while (!exitFlag) {
                uint32_t block = random(50, 150);
                {
                    SINGLE_THREADED_SECTION();
                    uint32_t startMillis = millis();
                    while ((millis() - startMillis) < block);
                }
                delayMicroseconds(1000 * 1000);
            }
        });
    };

    ~ContextSwitchBlockingInteferer() {
        if (thread_) {
            exit_ = 1;
            thread_->join();
            delete thread_;
        }
    };

private:
    bool exit_;
    Thread* thread_;
};

#endif // INTERFERER_H_
