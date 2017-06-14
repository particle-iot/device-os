#pragma once

#include "core_hal_stm32f2xx.h"
#include "debug.h"

#include "FreeRTOS.h"
#include "semphr.h"

// TODO: Use xSemaphoreCreateMutexStatic() after upgrading to FreeRTOS 9.x.x
class StaticRecursiveMutex {
public:
    StaticRecursiveMutex() :
            mutex_(nullptr) {
    }

    ~StaticRecursiveMutex() {
        if (mutex_) {
            vSemaphoreDelete(mutex_);
        }
    }

    bool lock(unsigned timeout = 0) {
        if (!mutex_ && !initMutex()) {
            return true; // FreeRTOS is not initialized
        }
        const TickType_t t = (timeout > 0) ? (timeout / portTICK_PERIOD_MS) : portMAX_DELAY;
        return (xSemaphoreTakeRecursive(mutex_, t) == pdTRUE);
    }

    bool unlock() {
        if (!mutex_) {
            return true;
        }
        return (xSemaphoreGiveRecursive(mutex_) == pdTRUE);
    }

private:
    volatile SemaphoreHandle_t mutex_;

    bool initMutex() {
        const int32_t state = HAL_disable_irq();
        if (!mutex_ && rtos_started) {
            mutex_ = xSemaphoreCreateRecursiveMutex();
            SPARK_ASSERT(mutex_);
        }
        HAL_enable_irq(state);
        return (mutex_ != nullptr);
    }
};
