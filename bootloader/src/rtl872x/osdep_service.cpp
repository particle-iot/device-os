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

#include <cstdlib>
#include "osdep_service.h"
#include "simple_pool_allocator.h"
#include <atomic>
#include "static_recursive_cs.h"
#include "timer_hal.h"

namespace {

uint8_t __attribute__((aligned(4))) sStaticPool[64 * 1024];
AtomicSimpleStaticPool sPool(sStaticPool, sizeof(sStaticPool));
particle::StaticRecursiveCriticalSectionLock sCsLock;
thread_func_t sFunc = nullptr;
void *sCtx = nullptr;

const size_t RTW_USBD_TASK_MAIN_LOOP_SEMAPHORE_OFFSET = 400;
const uint32_t RTW_USBD_TASK_MAIN_LOOP_PERIOD_MS = 1;

class AtomicCountingSemaphore {
public:
    AtomicCountingSemaphore(int initVal = 0)
            : counter_(initVal) {
    }

    bool acquire(uint32_t timeout = 0xffffffff) {
        auto start = hal_timer_millis(nullptr);
        do {
            auto v = counter_.load();
            if (v > 0) {
                if (counter_.compare_exchange_weak(v, v - 1, std::memory_order_acquire, std::memory_order_relaxed)) {
                    return true;
                }
            }
        } while (hal_timer_millis(nullptr) < start + timeout);
        std::atomic_thread_fence(std::memory_order_acquire);
        return false;
    }

    void release() {
        std::atomic_thread_fence(std::memory_order_release);
        counter_.fetch_add(1, std::memory_order_relaxed);
    }
private:
    std::atomic_int counter_;
};

} // anonymous

extern "C" {
// To simplify access from assembly to avoid mangling
__attribute__((used)) void* rtwUsbBackupSp = nullptr;
__attribute__((used)) void* rtwUsbBackupLr = nullptr;
__attribute__((used)) void rtw_usb_task();
} // extern "C"

uint8_t* rtw_zvmalloc(uint32_t sz) {
    auto p = sPool.alloc(sz);
    if (p) {
        memset(p, 0, sz);
    }
    return (uint8_t*)p;
}

uint8_t* rtw_zmalloc(uint32_t sz) {
    return rtw_zvmalloc(sz);
}

void rtw_mfree(uint8_t* pbuf, uint32_t size) {
    sPool.free(pbuf);
}

void rtw_usleep_os(int us) {
    // XXX: this is only used by the USB driver
    // There appears to be some issues with the ROM DelayXxx usage
    // DelayUs(us);
}

void rtw_msleep_os(int ms) {
    // XXX: this is only used by the USB driver
    // There appears to be some issues with the ROM DelayXxx usage
    // DelayMs(ms);
}

void rtw_init_sema(_sema *sema, int init_val) {
    auto ptr = rtw_zmalloc(sizeof(AtomicCountingSemaphore));
    if (ptr) {
        AtomicCountingSemaphore* sem = new(ptr) AtomicCountingSemaphore(init_val);
        *sema = (void*)sem;
    } else {
        *sema = nullptr;
    }
}

void rtw_free_sema(_sema* sema) {
    auto sem = static_cast<AtomicCountingSemaphore*>(*sema);
    sem->~AtomicCountingSemaphore();
    rtw_mfree((uint8_t*)sem, 0);
}


void rtw_up_sema_from_isr(_sema* sema) {
    auto sem = static_cast<AtomicCountingSemaphore*>(*sema);
    sem->release();
}

u32	rtw_down_sema(_sema* sema) {
    auto sem = static_cast<AtomicCountingSemaphore*>(*sema);
    uint8_t* tmp = (uint8_t*)sCtx;
    if ((void*)sema == (void*)(tmp + RTW_USBD_TASK_MAIN_LOOP_SEMAPHORE_OFFSET)) {
        bool v = sem->acquire(RTW_USBD_TASK_MAIN_LOOP_PERIOD_MS);
        if (!v) {
            // Break out of USB driver task loop if there is nothing to process from the USB peripheral
            // by restoring stack pointer captured just before jumping into
            // it and jumping back.
            asm volatile ("mov sp, %0\n\t"
                          "bx %1\n\t" :: "r" (rtwUsbBackupSp), "r" (rtwUsbBackupLr));
            // Unreachable
            SPARK_ASSERT(false);
        }
        return v;
    }
    return sem->acquire();
}

void rtw_spinlock_init(_lock *plock) {
    rtw_init_sema((_sema*)plock, 1);
}

void rtw_spinlock_free(_lock *plock) {
    rtw_free_sema((_sema*)plock);
}

void rtw_spin_lock(_lock *plock) {
    rtw_down_sema((_sema*)plock);
}

void rtw_spin_unlock(_lock *plock) {
    rtw_up_sema_from_isr((_sema*)plock);
}

void rtw_enter_critical(_lock *plock, _irqL *pirqL) {
    sCsLock.lock();
}

void rtw_exit_critical(_lock *plock, _irqL *pirqL) {
    sCsLock.unlock();
}

void rtw_memcpy(void* dst, void* src, uint32_t sz) {
    memcpy(dst, src, sz);
}

void rtw_thread_exit(void) {
    sFunc = nullptr;
}

int	rtw_create_task(struct task_struct* task, const char* name, uint32_t stack_size, uint32_t priority, thread_func_t func, void *thctx) {
    sFunc = func;
    sCtx = thctx;
    return 1;
}

void rtw_delete_task(struct task_struct* task) {
    sFunc = nullptr;
}

void rtw_usb_task() {
    if (sFunc) {
        sFunc(sCtx);
    }
}
