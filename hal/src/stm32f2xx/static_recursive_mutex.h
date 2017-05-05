#pragma once

#include "hal_irq_flag.h"
#include "debug.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"

#if PLATFORM_ID == 6 || PLATFORM_ID == 8
#include "wwd_rtos_interface.h"
#endif // PLATFORM_ID == 6 || PLATFORM_ID == 8

class StaticRecursiveMutex {
public:
    StaticRecursiveMutex() :
            mutex_(nullptr) {
        init();
    }

    ~StaticRecursiveMutex() {
        if (mutex_) {
            vSemaphoreDelete(mutex_);
        }
    }

    bool lock() {
        if (!mutex_) {
            init();
        }
        return (xSemaphoreTakeRecursive(mutex_, portMAX_DELAY) == pdTRUE);
    }

    bool unlock() {
        return (xSemaphoreGiveRecursive(mutex_) == pdTRUE);
    }

private:
    SemaphoreHandle_t mutex_;
    StaticSemaphore_t buffer_;

    void init() {
        const int32_t state = HAL_disable_irq();
        if (!mutex_) {
            mutex_ = xSemaphoreCreateMutexStatic(&buffer_);
            SPARK_ASSERT(mutex_);
        }
        HAL_enable_irq(state);
    }
};
