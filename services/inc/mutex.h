#pragma once

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "hal_irq_flag.h"

#if PLATFORM_ID == 6 || PLATFORM_ID == 8
# include "wwd_rtos_interface.h"
#endif // PLATFORM_ID == 6 || PLATFORM_ID == 8

class StaticRecursiveMutex
{
public:
    StaticRecursiveMutex(uint32_t blockTime = 0)
        : blockTime_{blockTime / portTICK_PERIOD_MS} {
        init();
    }
    ~StaticRecursiveMutex() {
        if (mutex_) {
            vSemaphoreDelete(mutex_);
        }
    }

    void init() {
        int32_t state = HAL_disable_irq();
        if (!mutex_) {
            mutex_ = xSemaphoreCreateMutexStatic(&buffer_);
        }
        HAL_enable_irq(state);
    }

    int lock() {
        if (!mutex_) {
            init();
        }
        return xSemaphoreTakeRecursive(mutex_, blockTime_);
    }
    int unlock() {
        return xSemaphoreGiveRecursive(mutex_);
    }

private:
    SemaphoreHandle_t mutex_ = NULL;
    StaticSemaphore_t buffer_;
    uint32_t blockTime_ = 0;
};
