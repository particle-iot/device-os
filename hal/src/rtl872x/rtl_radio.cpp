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

#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_INFO

// Uncomment to enable coex/tdma debug
// #define RTL_DEBUG_COEX

#include "module_info.h"

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <mutex>
extern "C" {
#include "rtl8721d.h"
}
#include "rtl_radio.h"
#include "rtl_sdk_support.h"
#include "km0_km4_ipc.h"
#include "logging.h"
#include "service_debug.h"
#include "osdep_service.h"
#include "concurrent_hal.h"
#include "delay_hal.h"
#include "wifi_conf.h"
#include "ble_hal.h"
#include "spark_wiring_thread.h"
#include "bt_intf.h"
#include "system_error.h"
#include "unique_lock.h"
#include "call_once.h"
#include "freertos/wrapper.h"

struct pcoex_reveng {
    uint32_t state;
    uint32_t unknown;
    _mutex* mutex;
};

namespace {
uint8_t radioStatus = RTW_RADIO_NONE;
RecursiveMutex radioMutex;

volatile bool s_wifiConnectionState = false;
volatile bool s_tdmaSkip = false;

uint32_t s_coexTable[4] = {0x55555555, 0xaaaaaaaa, 0xf0ffffff, 0xb};
uint8_t s_tdmaTable[5] = {0x51, 0x00 /* no override */, 0x03 /* unknown */, 0x11 /* no tx pause in bt slot */, 0x11 /* not blocking bt low priority packets */};
void* s_coex_struct = nullptr;
}

extern "C" pcoex_reveng* pcoex[4];

extern "C" int rtw_coex_wifi_enable(void* priv, uint32_t state);
extern "C" int rtw_coex_bt_enable(void* priv, uint32_t state);

extern "C" Rltk_wlan_t rltk_wlan_info[NET_IF_NUM];

void rtwCoexRunDisable(int idx) {
    os_thread_scheduling(false, nullptr);
    auto p = pcoex[idx];
    if (p && (p->state & 0x000000ff) != 0x00) {
        p->state &= 0xffffff00;
    }
    os_thread_scheduling(true, nullptr);
}

void rtwCoexPreventCleanup(int idx) {
    os_thread_scheduling(false, nullptr);
    auto p = pcoex[idx];
    if (p) {
        p->state = 0xff00ff00;
    }
    os_thread_scheduling(true, nullptr);
}

void rtwCoexRunEnable(int idx) {
    os_thread_scheduling(false, nullptr);
    auto p = pcoex[idx];
    if (p) {
        p->state |= 0x01;
    }
    os_thread_scheduling(true, nullptr);
}

void rtwCoexCleanupMutex(int idx) {
    int start = idx;
    int end = idx;
    if (idx < 0) {
        start = 0;
        end = sizeof(pcoex)/sizeof(pcoex[0]) - 1;
    }
    for (int i = start; i <= end; i++) {
        auto p = pcoex[i];
        if (p && p->mutex) {
            auto m = p->mutex;
            p->mutex = nullptr;
            rtw_mutex_free(m);
        }
    }
}

void rtwCoexCleanup(int idx) {
    rtwCoexCleanupMutex(idx);
    int start = idx;
    int end = idx;
    if (idx < 0) {
        start = 0;
        end = sizeof(pcoex)/sizeof(pcoex[0]) - 1;
    }
    for (int i = start; i <= end; i++) {
        if (pcoex[i]) {
            free(pcoex[i]);
            pcoex[i] = nullptr;
        }
    }
}

extern "C" int rltk_coex_set_wifi_slot(u8 wifi_slot);
extern "C" u8 rltk_wlan_btcoex_lps_enabled(void);
extern "C" void rtw_write8(void* p, uint32_t offset, uint32_t val);
extern "C" void rtw_write16(void* p, uint32_t offset, uint32_t val);
extern "C" uint8_t rtw_read8(void* p, uint32_t offset);
extern "C" uint16_t rtw_read16(void* p, uint32_t offset);
extern "C" void rtw_hal_fill_h2c_cmd(void* coex, uint8_t element_id, uint32_t cmd_len, uint8_t *cmdbuffer);
extern "C" void rtw_write32(void* p, uint32_t offset, uint32_t val);

void rtwCoexStop() {
    std::lock_guard<RecursiveMutex> lk(radioMutex);
    // This call makes rtw_coex_run_enable(123, false) not cleanup the mutex
    // which might be still accessed by other coexistence tasks
    // but still allows rtw_coex_bt_enable to deinitialize BT coexistence.
    // Subsequently we restore the state with rtwCoexRunEnable() and rtw_coex_wifi_enable
    // will call into rtw_coex_run_enable(123, false) which will finally cleanup the mutex
    rtwCoexRunDisable(0);
    rtw_coex_bt_enable(*(void**)rltk_wlan_info[0].dev->priv, 0);
    HAL_Delay_Milliseconds(100);
    rtwCoexRunEnable(0);
    rtw_coex_wifi_enable(*(void**)rltk_wlan_info[0].dev->priv, 0);
    rtwCoexRunDisable(0);
    rtw_coex_wifi_enable(*(void**)rltk_wlan_info[0].dev->priv, 1);
    rtwCoexCleanupMutex(0);
}

void rtwRadioReset() {
    // XXX: the order of the locks has to be BLE -> radioMutex
    hal_ble_lock(nullptr);
    particle::UniqueLock<RecursiveMutex> lk(radioMutex);
    bool bleInitialized = hal_ble_is_initialized(nullptr);
    bool advertising = hal_ble_gap_is_advertising(nullptr) ||
                       hal_ble_gap_is_connecting(nullptr, nullptr) ||
                       hal_ble_gap_is_connected(nullptr, nullptr);
    if (bleInitialized) {
        hal_ble_stack_deinit(nullptr);
    }

    rtwRadioRelease(RTW_RADIO_WIFI);
    rtwRadioAcquire(RTW_RADIO_WIFI);

    if (bleInitialized) {
        if (hal_ble_stack_init(nullptr) == 0 && advertising) {
            hal_ble_gap_start_advertising(nullptr);
        }
    }
    lk.unlock();
    hal_ble_unlock(nullptr);
}

void rtwCoexSetWifiConnectedState(bool state) {
    if (state == s_wifiConnectionState) {
        return;
    }
    s_wifiConnectionState = state;

    // XXX XXX
    // Disabling this for now, for some reason calling rltk_coex_set_wlan_slot_preempting()
    // while BLE is already in a scanning state (at the very least) breaks coexistence.
    // There is a call rltk_coex_set_wlan_slot_preempting() when we enable WiFi, so
    // it should be sufficient.
#if 0
    if (state) {
        hal_ble_lock(nullptr);

        // Just in case, forces coex to re-run
        rltk_coex_set_wlan_slot_preempting(0b111);

        hal_ble_unlock(nullptr);
    } else {
        hal_ble_lock(nullptr);

        // Just in case, forces coex to re-run
        rltk_coex_set_wlan_slot_preempting(0b111);

        hal_ble_unlock(nullptr);
    }
#endif
}

bool rtwCoexWifiConnectedState() {
    return s_wifiConnectionState;
}

extern "C" void __copy_rtl8721d_set_pstdma_cmd(void* coex, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5);
extern "C" void rtl8721d_set_pstdma_cmd(void* coex, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5) {
#ifdef RTL_DEBUG_COEX
    LOG(INFO, "original %02x %02x %02x %02x %02x", b1, b2, b3, b4, b5);
#endif
    if (b1 != 0x00 && !s_tdmaSkip) {
        // Force our own
        b1 = s_tdmaTable[0];
        b2 = s_tdmaTable[1] ? s_tdmaTable[1] : b2;
        b3 = s_tdmaTable[2];
        b4 = s_tdmaTable[3];
        b5 = s_tdmaTable[4];
    }

    // Pass through to actual implementation
    __copy_rtl8721d_set_pstdma_cmd(coex, b1, b2, b3, b4, b5);

#ifdef RTL_DEBUG_COEX
    LOG(INFO, "tdma set %02x %02x %02x %02x %02x tbtt=%02x lps=%d", b1, b2, b3, b4, b5, rtw_read8(coex, 0x550), rltk_wlan_btcoex_lps_enabled());
#endif // RTL_DEBUG_COEX
    if (!s_coex_struct) {
        s_coex_struct = coex;
    }
}

extern "C" void __copy_rtl8721d_set_coex_table(void* coex, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3);
extern "C" void rtl8721d_set_coex_table(void* coex, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3) {
#ifdef RTL_DEBUG_COEX
    LOG(INFO, "original %08x %08x %08x %08x", v0, v1, v2, v3);
#endif

    s_tdmaSkip = true;

    // WiFi connected and BLE scan state
    if (v0 == 0x55555555 && v1 == 0xaaaaaaaa) {
        v0 = s_coexTable[0];
        v1 = s_coexTable[1];
        v2 = s_coexTable[2];
        v3 = s_coexTable[3];
        s_tdmaSkip = false;
    }

#ifdef RTL_DEBUG_COEX
    constexpr auto REG_BT_COEX_TABLE1 = 0x06c0;
    constexpr auto REG_BT_COEX_TABLE2 = 0x06c4;
    constexpr auto REG_BT_COEX_TABLE3 = 0x06c8;
    constexpr auto REG_BT_COEX_TABLE4 = 0x06cc;
    uint32_t t1 = HAL_READ32(WIFI_REG_BASE, REG_BT_COEX_TABLE1);
    uint32_t t2 = HAL_READ32(WIFI_REG_BASE, REG_BT_COEX_TABLE2);
    uint32_t t3 = HAL_READ32(WIFI_REG_BASE, REG_BT_COEX_TABLE3);
    uint32_t t4 = HAL_READ8(WIFI_REG_BASE, REG_BT_COEX_TABLE4);
#endif // RTL_DEBUG_COEX

    // Pass through to actual implementation
    __copy_rtl8721d_set_coex_table(coex, v0, v1, v2, v3);

#ifdef RTL_DEBUG_COEX
    LOG(INFO, "coex table %08x %08x %08x %08x (before %08x %08x %08x %08x) lps=%d %08x %08x", v0, v1, v2, v3, t1, t2, t3, t4, rltk_wlan_btcoex_lps_enabled(), HAL_READ8(WIFI_REG_BASE, REG_BT_COEX_TABLE4), coex);
#endif // RTL_DEBUG_COEX
    if (!s_coex_struct) {
        s_coex_struct = coex;
    }
}

int rtwCoexSet(uint32_t coex[4], uint8_t tdma[5], bool apply) {
    memcpy(s_coexTable, coex, sizeof(s_coexTable));
    memcpy(s_tdmaTable, tdma, sizeof(s_tdmaTable));

    if (apply && s_coex_struct) {
        rtl8721d_set_coex_table(s_coex_struct, coex[0], coex[1], coex[2], coex[3]);
        rtl8721d_set_pstdma_cmd(s_coex_struct, tdma[0], tdma[1], tdma[2], tdma[3], tdma[4]);
    }
    return 0;
}

extern "C" void __copy_rtw_hal_fill_h2c_cmd(void* coex, uint8_t element_id, uint32_t cmd_len, uint8_t *cmdbuffer) {
    // Pass through to actual implementation
    rtw_hal_fill_h2c_cmd(coex, element_id, cmd_len, cmdbuffer);
}

extern "C" void __real_rtw_hal_fill_h2c_cmd(void* coex, uint8_t element_id, uint32_t cmd_len, uint8_t *cmdbuffer);
extern "C" void __wrap_rtw_hal_fill_h2c_cmd(void* coex, uint8_t element_id, uint32_t cmd_len, uint8_t *cmdbuffer) {
#ifdef RTL_DEBUG_COEX
    LOG(INFO, "h2c cmd element=%02x cmd_len=%u return=%08x %08x", element_id, cmd_len, __builtin_extract_return_addr (__builtin_return_address (0)), HAL_READ8(WIFI_REG_BASE, 0x06cc));
    LOG_DUMP(INFO, cmdbuffer, cmd_len);
    LOG_PRINTF(INFO, "\r\n");
#endif // RTL_DEBUG_COEX
    // Pass through to actual implementation
    __real_rtw_hal_fill_h2c_cmd(coex, element_id, cmd_len, cmdbuffer);
}

extern "C" void __real_rtw_write32(void* p, uint32_t offset, uint32_t val);
extern "C" void __wrap_rtw_write32(void* p, uint32_t offset, uint32_t val) {
// #ifdef RTL_DEBUG_COEX
//     LOG(INFO, "write %08x (%08x + %08x)=%08x", (uint32_t)p + offset, p, offset, val);
// #endif // RTL_DEBUG_COEX
    __real_rtw_write32(p, offset, val);
}

extern "C" void __real_rtw_write16(void* p, uint32_t offset, uint32_t val);
extern "C" void __wrap_rtw_write16(void* p, uint32_t offset, uint32_t val) {
// #ifdef RTL_DEBUG_COEX
//     LOG(INFO, "write %08x (%08x + %08x)=%04x", (uint32_t)p + offset, p, offset, val);
// #endif // RTL_DEBUG_COEX
    __real_rtw_write16(p, offset, val);
}

extern "C" void __real_rtw_write8(void* p, uint32_t offset, uint32_t val);
extern "C" void __wrap_rtw_write8(void* p, uint32_t offset, uint32_t val) {
// #ifdef RTL_DEBUG_COEX
//     LOG(INFO, "write %08x (%08x + %08x)=%02x", (uint32_t)p + offset, p, offset, val);
// #endif // RTL_DEBUG_COEX
    __real_rtw_write8(p, offset, val);
}

extern "C" void __copy_rtw_write32(void* p, uint32_t offset, uint32_t val) {
    // Pass through to actual implementation
// #ifdef RTL_DEBUG_COEX
//     LOG(INFO, "write %08x (%08x + %08x)=%08x", (uint32_t)p + offset, p, offset, val);
// #endif // RTL_DEBUG_COEX
    return rtw_write32(p, offset, val);
}

extern uintptr_t link_prebootloader_wifi_fw_ram_start;
extern uintptr_t link_prebootloader_wifi_fw_ram_end;

void rtwRadioAcquire(RtwRadio r) {
    std::lock_guard<RecursiveMutex> lk(radioMutex);
    LOG_DEBUG(INFO, "rtwRadioAcquire: %d", r);
    auto preStatus = radioStatus;
    radioStatus |= r;
    if (preStatus != RTW_RADIO_NONE) {
        LOG(INFO, "WiFi is already on");
        return;
    }
    if (radioStatus != RTW_RADIO_NONE) {
        static particle::OnceFlag once;
        particle::CallOnce(once, [](){
            static rtl_wifi_fw_ram_alloc __attribute__((aligned(32))) info;
            info.size = sizeof(info);
            info.start = (uint32_t)&link_prebootloader_wifi_fw_ram_start;
            info.end = (uint32_t)&link_prebootloader_wifi_fw_ram_end;
            DCache_CleanInvalidate((uint32_t)&info, sizeof(info));
            rtl_wifi_fw_resp resp = {};
            int r = km0_km4_ipc_send_request(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_WIFI_FW_INIT, &info, sizeof(info), [](km0_km4_ipc_msg_t* msg, void* context) -> void {
                memcpy(context, msg->data, std::min<size_t>(msg->size, sizeof(rtl_wifi_fw_resp)));
            }, &resp);
            LOG(INFO, "WiFi KM0 firmware initialization started   result=%d (RAM start=%08x end=%08x)", r, info.start, info.end);
            LOG(INFO, "WiFi KM0 firmware initialization completed result=%d (RAM start=%08x end=%08x reserved=%08x)", resp.result, resp.start, resp.end, resp.reserved_heap);
            if (resp.result == SYSTEM_ERROR_NO_MEMORY) {
                LOG(ERROR, "Not enough memory for KM0 WiFi firmware to run");
            }
        });

        RCC_PeriphClockCmd(APBPeriph_WL, APBPeriph_WL_CLOCK, ENABLE);
        rtwCoexCleanup(0);
        SPARK_ASSERT(wifi_on(RTW_MODE_STA) == 0);

        LOG(INFO, "WiFi/BLE radio on");
        rltk_coex_set_wlan_slot_preempting(0b111);
    }
}

void rtwRadioRelease(RtwRadio r) {
    std::lock_guard<RecursiveMutex> lk(radioMutex);
    LOG_DEBUG(INFO, "rtwRadioRelease: %d", r);
    auto preStatus = radioStatus;
    radioStatus &= ~r;
    if (preStatus == RTW_RADIO_NONE) {
        LOG(INFO, "WiFi is already off");
        return;
    }
    if (radioStatus == RTW_RADIO_NONE) {
        rtwCoexPreventCleanup(0);
        HAL_Delay_Milliseconds(100);
        wifi_off();
        RCC_PeriphClockCmd(APBPeriph_WL, APBPeriph_WL_CLOCK, DISABLE);
        LOG(INFO, "WiFi off");
    }
}

#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
