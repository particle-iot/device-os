#ifndef INTERFERER_H_
#define INTERFERER_H_

#include "application.h"

struct HighPriorityInterruptInterferer {
    HighPriorityInterruptInterferer() {
#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
        // Enable some high priority interrupt to run interference
        pinMode(D0, OUTPUT);
        // D0 uses TIM2 and channel 2 equally on Core and Photon/Electron
        // Run at 4khz (T=250us)
        analogWrite(D0, 1, 4000);
        randomSeed(micros());
        // Set higher than SysTick_IRQn priority for TIM4_IRQn
        NVIC_SetPriority(TIM4_IRQn, 3);
        TIM_ITConfig(TIM4, TIM_IT_CC2, ENABLE);
        NVIC_EnableIRQ(TIM4_IRQn);
        attachSystemInterrupt(SysInterrupt_TIM4_IRQ, [&] {
            // Do some work up to 200 microseconds
            delayMicroseconds(random(200));
            TIM_ClearITPendingBit(TIM4, TIM_IT_CC2);
        });
#endif
    };

    ~HighPriorityInterruptInterferer() {
#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
    NVIC_DisableIRQ(TIM4_IRQn);
    detachSystemInterrupt(SysInterrupt_TIM4_IRQ);
    TIM_ITConfig(TIM4, TIM_IT_CC2, DISABLE);
    digitalWrite(D0, HIGH);
#endif
    };
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