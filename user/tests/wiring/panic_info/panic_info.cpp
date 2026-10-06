/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"
#include "unit-test/unit-test.h"

#include "service_debug.h"

#if HAL_PLATFORM_FILESYSTEM

namespace {
    constexpr size_t kFragBlocks = 1000;
    void* frag_[kFragBlocks];

    os_thread_t hammer_;
    volatile uint32_t hammerLoops_ = 0;

    __attribute__((noinline)) void touchHeap() {
        void* volatile p = malloc(16);
        free(p);
    }

    // Leave ~kFragBlocks/2 small holes in the free list so every lock hold walks a long list
    void fragmentHeap() {
        for (size_t i = 0; i < kFragBlocks; i++) {
            frag_[i] = malloc(32);
        }
        for (size_t i = 0; i < kFragBlocks; i += 2) {
            free(frag_[i]);
            frag_[i] = nullptr;
        }
    }

    void mallocHammer(void*) {
        for (;;) {
            void* volatile p = malloc(256);
            free(p);
            hammerLoops_++;
        }
    }
} // anonymous

test(PANIC_INFO_01_expected_panic) {
    // Notify the runner about the upcoming panic and wait for the ACK, so the runner
    // expects the reboot and verifies the panic info stored by the panic handler
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::PANIC_PENDING), 20000));
    // A real assertion panic: code AssertionFailure (10) with the assertion text, pc and lr
    // pointing at this test - the same data the runner reports for unexpected panics
    SPARK_ASSERT(false);
}

test(PANIC_INFO_02_no_panic_info_left) {
    // The runner consumed and cleared the panic record after the expected panic; the
    // record must not reappear after the reboot that followed
}

#if HAL_PLATFORM_STRICT_HEAP_LOCK_PANIC

test(PANIC_INFO_03_heap_access_with_scheduling_disabled) {
    pushMailboxMsg("HAL_PLATFORM_STRICT_HEAP_LOCK_PANIC = 1", 5000);

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::PANIC_PENDING), 20000));
    // Strict mode: any heap lock taken while the scheduler is suspended must panic with HeapError
    SINGLE_THREADED_BLOCK() {
        touchHeap();
    }
    Test::out->println("Heap access with scheduling disabled did not panic");
    fail();
}

test(PANIC_INFO_04_heap_access_with_scheduling_disabled_reason) {
    assertEqual((int)System.resetReason(), (int)RESET_REASON_PANIC);
    assertEqual(System.resetReasonData(), (uint32_t)HeapError);
}

test(PANIC_INFO_05_heap_access_contention_panic) {
    // Test is redundant in strict mode with test 3, skip it
    skip();
}

test(PANIC_INFO_06_heap_access_contention_did_panic) {
    // No panic expected from previous test
    skip();
}

#else

test(PANIC_INFO_03_heap_access_with_scheduling_disabled) {
    pushMailboxMsg("HAL_PLATFORM_STRICT_HEAP_LOCK_PANIC = 0", 5000);
    // Default mode: the zero-timeout recursive take succeeds when the heap mutex is uncontended
    SINGLE_THREADED_BLOCK() {
        touchHeap();
    }
}

test(PANIC_INFO_04_heap_access_with_scheduling_disabled_reason) {
    // No panic expected in default mode
}

test(PANIC_INFO_05_heap_access_contention_panic) {
    // Expect panic
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::PANIC_PENDING), 20000));

    // Intentionally fragment heap so that hammer thread will spend more time holding the mutex
    // which will increase the chance of forcing a panic when contention happens
    fragmentHeap();
    os_thread_create(&hammer_, "hammer", OS_THREAD_PRIORITY_DEFAULT, mallocHammer, nullptr, 1024);
    auto i = 0;
    for (i = 0; i < 1000; i++) {
        // Block for a tick to allow hammer thread to run
        HAL_Delay_Milliseconds(1);
        // Suspend scheduler, hopefully while malloc hammer thread holds the lock
        SINGLE_THREADED_BLOCK() {
            // Attempt to take the heap lock while another thread holds it and scheduling is disabled
            touchHeap();
        }
    }
    pushMailboxMsg(String::format("Heap contention did not panic, iterations=%d hammer loops=%lu", i, (unsigned long)hammerLoops_, 5000));
    fail();
}

test(PANIC_INFO_06_heap_access_contention_did_panic) {
    assertEqual((int)System.resetReason(), (int)RESET_REASON_PANIC);
    assertEqual(System.resetReasonData(), (uint32_t)HeapError);
}

#endif // HAL_PLATFORM_STRICT_HEAP_LOCK_PANIC

#endif // HAL_PLATFORM_FILESYSTEM
