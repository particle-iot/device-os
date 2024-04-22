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

#define REALTEK_WIFI_LOG_ENABLE 0
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_INFO

// Uncomment to enable coex/tdma debug
// #define RTL_DEBUG_COEX

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cctype>
extern "C" {
#include "rtl8721d.h"
}
#include "rtl_sdk_support.h"
#include "strproc.h"
#include "service_debug.h"
#include "km0_km4_ipc.h"

#include "logging.h"
#include "interrupts_hal.h"
#include "osdep_service.h"
#include "concurrent_hal.h"
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#include "delay_hal.h"
#include "wifi_conf.h"
#include "ble_hal.h"
#include "spark_wiring_thread.h"
#endif
#include "freertos/wrapper.h"
#include "bt_intf.h"

extern "C" {

// Declaring here in order not to include headers that have a lot of unnecessary includes
// and also not exactly correct declarations.
int _rtl_vsprintf(char *buf, size_t size, const char *fmt, va_list args);
int _rtl_printf(const char* fmt, ...);
int _rtl_sprintf(char* str, const char* fmt, ...);
int _rtl_snprintf(char* str, size_t size, const char* fmt, ...);
int _rtl_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int _rtl_sscanf(const char *buf, const char *fmt, ...);

}

struct pcoex_reveng {
    uint32_t state;
    uint32_t unknown;
    _mutex* mutex;
};

namespace {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
uint8_t radioStatus = RTW_RADIO_NONE;
RecursiveMutex radioMutex;

volatile bool s_wifiConnectionState = false;
volatile bool s_tdmaSkip = false;

uint32_t s_coexTable[3] = {0x55555555, 0xaaaa5a5a, 0xf0ffffff};
uint8_t s_tdmaTable[5] = {0x51, 0x45, 0x03, 0x10, 0x50};
void* s_coex_struct = nullptr;
#endif
}

extern "C" pcoex_reveng* pcoex[4];

extern "C" int rtw_coex_wifi_enable(void* priv, uint32_t state);
extern "C" int rtw_coex_bt_enable(void* priv, uint32_t state);

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
extern "C" Rltk_wlan_t rltk_wlan_info[NET_IF_NUM];
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

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

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

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
    std::unique_lock<RecursiveMutex> lk(radioMutex);
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

extern "C" void __copy_rtl8721d_set_pstdma_cmd(void* coex, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5);
extern "C" void rtl8721d_set_pstdma_cmd(void* coex, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5) {
    // Skip setting TDMA parameters in some cases as to keep current settings and not reset the internal state
    // even if the values (old/new) are matching.
    if (s_tdmaSkip) {
#ifdef RTL_DEBUG_COEX
        LOG(INFO, "tdma skip");
#endif // RTL_DEBUG_COEX
        s_tdmaSkip = false;
        return;
    }
    if (b1 != 0x00) {
        // Force our own
        b1 = s_tdmaTable[0];
        b2 = s_tdmaTable[1];
        b3 = s_tdmaTable[2];
        b4 = s_tdmaTable[3];
        b5 = s_tdmaTable[4];
    }
    // Pass through to actual implementation
    __copy_rtl8721d_set_pstdma_cmd(coex, b1, b2, b3, b4, b5);

#ifdef RTL_DEBUG_COEX
    LOG(INFO, "tdma set %02x %02x %02x %02x %02x", b1, b2, b3, b4, b5);
#endif // RTL_DEBUG_COEX
    if (!s_coex_struct) {
        s_coex_struct = coex;
    }
}

void rtwCoexSetWifiConnectedState(bool state) {
    s_wifiConnectionState = state;
}

bool rtwCoexWifiConnectedState() {
    return s_wifiConnectionState;
}

extern "C" void __copy_rtl8721d_set_coex_table(void* coex, uint32_t v0, uint32_t v1, uint32_t v2);
extern "C" void rtl8721d_set_coex_table(void* coex, uint32_t v0, uint32_t v1, uint32_t v2) {
    if (v0 == 0x5a5a5a5a && v1 == 0x5a5a5a5a && v2 == 0xf3ffffff && rtwCoexWifiConnectedState()) {
#ifdef RTL_DEBUG_COEX
        LOG(INFO, "Disallow bt high > wifi > bt low mode when WiFi is in connected state");
#endif // RTL_DEBUG_COEX
        s_tdmaSkip = true;
        return;
    }

    // WiFi connected and BLE scan state
    // BT > WiFi in BT slot; WLAN high-Priority > BT high-Priority > WLAN low-Priority> BT low-Priority in WiFi slot
    // with TDMA working (default 0xf3ffffff break table does not work correctly)
    if (v0 == 0x55555555 && (v1 == 0x5a5a5a5a || v1 == 0xaaaa5a5a || v1 == 0xaaaaaaaa)) {
        v0 = s_coexTable[0];
        v1 = s_coexTable[1];
        v2 = s_coexTable[2];
    }

#ifdef RTL_DEBUG_COEX
    constexpr auto REG_BT_COEX_TABLE1 = 0x06c0;
    constexpr auto REG_BT_COEX_TABLE2 = 0x06c4;
    constexpr auto REG_BT_COEX_TABLE3 = 0x06c8;
    uint32_t t1 = HAL_READ32(WIFI_REG_BASE, REG_BT_COEX_TABLE1);
    uint32_t t2 = HAL_READ32(WIFI_REG_BASE, REG_BT_COEX_TABLE2);
    uint32_t t3 = HAL_READ32(WIFI_REG_BASE, REG_BT_COEX_TABLE3);
#endif // RTL_DEBUG_COEX

    // Pass through to actual implementation
    __copy_rtl8721d_set_coex_table(coex, v0, v1, v2);

#ifdef RTL_DEBUG_COEX
    LOG(INFO, "coex table %08x %08x %08x (before %08x %08x %08x) ", v0, v1, v2, t1, t2, t3);
#endif // RTL_DEBUG_COEX
    if (!s_coex_struct) {
        s_coex_struct = coex;
    }
}

int rtwCoexSet(uint32_t coex[3], uint8_t tdma[5], bool apply) {
    memcpy(s_coexTable, coex, sizeof(s_coexTable));
    memcpy(s_tdmaTable, tdma, sizeof(s_tdmaTable));
    //constexpr size_t COEX_OFFSET = 2664;
    if (apply) {
        // rtl8721d_set_coex_table(*(void**)((uintptr_t)pcoex[0] + COEX_OFFSET), coex[0], coex[1], coex[2]);
        // rtl8721d_set_pstdma_cmd(*(void**)((uintptr_t)pcoex[0] + COEX_OFFSET), tdma[0], tdma[1], tdma[2], tdma[3], tdma[4]);
        rtl8721d_set_coex_table(s_coex_struct, coex[0], coex[1], coex[2]);
        rtl8721d_set_pstdma_cmd(s_coex_struct, tdma[0], tdma[1], tdma[2], tdma[3], tdma[4]);
    }
    return 0;
}

extern "C" void rtw_hal_fill_h2c_cmd(void* coex, uint8_t element_id, uint32_t cmd_len, uint8_t *cmdbuffer);
extern "C" void __copy_rtw_hal_fill_h2c_cmd(void* coex, uint8_t element_id, uint32_t cmd_len, uint8_t *cmdbuffer) {
    // Pass through to actual implementation
    rtw_hal_fill_h2c_cmd(coex, element_id, cmd_len, cmdbuffer);
}

extern "C" void rtw_write32(void* p, uint32_t offset, uint32_t val);
extern "C" void __copy_rtw_write32(void* p, uint32_t offset, uint32_t val) {
    // Pass through to actual implementation
    return rtw_write32(p, offset, val);
}

extern "C" int rltk_coex_set_wifi_slot(u8 wifi_slot);
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
        RCC_PeriphClockCmd(APBPeriph_WL, APBPeriph_WL_CLOCK, ENABLE);
        rtwCoexCleanup(0);
        SPARK_ASSERT(wifi_on(RTW_MODE_STA) == 0);
        LOG(INFO, "WiFi on");
        wifi_set_power_mode(0, 0);
        wifi_disable_powersave();
        rltk_coex_set_wlan_slot_preempting(0b000);
        rltk_coex_set_wifi_slot(50);
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
#endif


extern "C" u32 DiagPrintf(const char *fmt, ...);
extern "C" int DiagVSprintf(char *buf, const char *fmt, const int *dp);
extern u32 ConfigDebugBuffer;
extern u32 ConfigDebugClose;
extern u32 ConfigDebug[];
typedef u32 (*DIAG_PRINT_BUF_FUNC)(const char *fmt);
extern DIAG_PRINT_BUF_FUNC ConfigDebugBufferGet;

int _rtl_sscanf(const char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int n = vsscanf(buf, fmt, args);
    va_end(args);
    return n;
}

int _rtl_printf(const char* fmt, ...) {
	u32 ret;
	const char* fmt1;
	log_buffer_t *buf = NULL;

	fmt1 = fmt;
	
	for(; *fmt1 != '\0'; ++fmt1) {
		if(*fmt1 == '"') {
			do {
				fmt1 ++;
			} while(*fmt1 != '"');
			fmt1 ++;
		}
		
		if(*fmt1 != '%')
			continue;
		else
			fmt1 ++;
		
		while(isdigit(*fmt1)){
			fmt1 ++;
		}
		
		if((*fmt1  == 's') || (*fmt1 == 'x') || (*fmt1 == 'X') || (*fmt1 == 'p') || (*fmt1 == 'P') || (*fmt1 == 'd') || (*fmt1 == 'c') || (*fmt1 == '%'))
			continue;
		else {
			DiagPrintf("%s: format not support!\n", __func__);
			break;
		}
	}
	
	if (ConfigDebugClose == 1)
		return 0;

	if (ConfigDebugBuffer == 1 && ConfigDebugBufferGet != NULL) {
		buf = (log_buffer_t *)ConfigDebugBufferGet(fmt);
	}

	if (buf != NULL) {
		return DiagVSprintf(buf->buffer, fmt, ((const int *)&fmt)+1);
	} else {
		ret = DiagVSprintf(NULL, fmt, ((const int *)&fmt)+1);

		return ret;
	}
}

int _rtl_sprintf(char* str, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int n = vsprintf(str, fmt, args) ;
    va_end(args);
    return n;
}

int _rtl_vsprintf(char *buf, size_t size, const char *fmt, va_list args) {
    return vsnprintf(buf, size, fmt, args);
}

int _rtl_vsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
    return _rtl_vsprintf(buf, size, fmt, args);
}

int _rtl_snprintf(char* str, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int n = _rtl_vsprintf(str, size, fmt, args);
    va_end(args);
    return n;
}

extern "C" void rtl_wifi_log(const char* fmt, ...) {
#if REALTEK_WIFI_LOG_ENABLE
    va_list args;
    va_start(args, fmt);
    LogAttributes attr = {};
    log_message_v(LOG_LEVEL_INFO, "rtl", &attr, nullptr, fmt, args);
    va_end(args);
#endif
}

#ifndef MBEDTLS_PLATFORM_MEMORY
extern "C" int mbedtls_platform_set_calloc_free(void*(*)(size_t, size_t), void (*)(void*)) {
    LOG(INFO, "mbedtls_platform_set_calloc_free");
    // mbedtls_calloc_func = calloc_func;
    // mbedtls_free_func = free_func;
    return 0;
}
#endif // MBEDTLS_PLATFORM_MEMORY

void ipc_table_init() {
    // stub
}


void ipc_send_message(uint8_t channel, uint32_t message) {
    // stub
}

uint32_t ipc_get_message(uint8_t channel) {
    // stub
    return 0;
}


int ipc_channel_init(uint8_t channel, rtl_ipc_callback_t callback) {
    if (channel > 15) {
        return -1;
    }
    IPC_INTUserHandler(channel, (void*)callback, NULL);
    return 0;
}

void ipc_send_message_alt(uint8_t channel, uint32_t message) {
    IPCM4_DEV->IPCx_USR[channel] = message;	
	IPC_INTRequest(IPCM4_DEV, channel);
}

uint32_t ipc_get_message_alt(uint8_t channel) {
    uint32_t msgAddr = IPCM0_DEV->IPCx_USR[channel];
    return msgAddr;
}

// FIXME: move it somewhere proper
extern "C" void HAL_Core_System_Reset(void) {
    __DSB();
    __ISB();

    // Disable systick
    SysTick->CTRL = SysTick->CTRL & ~SysTick_CTRL_ENABLE_Msk;

    // Disable global interrupt
    __disable_irq();

    BKUP_Write(BKUP_REG1, 0xdeadbeef);

    WDG_InitTypeDef WDG_InitStruct = {};
    u32 CountProcess = 0;
    u32 DivFacProcess = 0;
    BKUP_Set(BKUP_REG0, BIT_KM4SYS_RESET_HAPPEN);
    WDG_Scalar(50, &CountProcess, &DivFacProcess);
    WDG_InitStruct.CountProcess = CountProcess;
    WDG_InitStruct.DivFacProcess = DivFacProcess;
    WDG_InitStruct.RstAllPERI = 0;
    WDG_Init(&WDG_InitStruct);

    __DSB();
    __ISB();

    WDG_Cmd(ENABLE);
    DelayMs(500);

    // It should have reset the device after this amount of delay. If not, try resetting device by KM0
    km0_km4_ipc_send_request(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_RESET, NULL, 0, NULL, NULL);
    while (1) {
        __WFE();
    }
}

extern "C" {

#if 0 // Enable to get btgap logs

/* Internal function that is used by internal macro DBG_DIRECT. */
void log_direct(uint32_t info, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LOG_C_VARG(INFO, "rtl", fmt, args);
    va_end(args);
}

/* Internal function that is used by internal macro DBG_BUFFER. */
void trace_log_buffer(uint32_t info, uint32_t log_str_index, uint8_t param_num, ...) {
    va_list args;
    va_start(args, param_num);
    LOG_C_VARG(INFO, "rtl", (const char*)log_str_index, args);
    va_end(args);
}

/* Internal function that is used by internal macro DBG_SNOOP. */
void log_snoop(uint32_t info, uint16_t length, uint8_t *p_snoop) {
    LOG_DUMP(INFO, p_snoop, length);
    LOG_PRINT(INFO, "\r\n");
}

/* Internal function that is used by public macro TRACE_BDADDR. */
const char *trace_bdaddr(uint32_t info, char *bd_addr) {
    LOG(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
        bd_addr[0],
        bd_addr[1],
        bd_addr[2],
        bd_addr[3],
        bd_addr[4],
        bd_addr[5]);
    return "<bdaddr>";
}

/* Internal function that is used by public macro TRACE_STRING. */
const char *trace_string(uint32_t info, char *p_data) {
    LOG(INFO, "%s", p_data);
    return p_data;
}

/* Internal function that is used by public macro TRACE_BINARY. */
const char *trace_binary(uint32_t info, uint16_t length, uint8_t *p_data) {
    LOG_DUMP(INFO, p_data, length);
    LOG_PRINT(INFO, "\r\n");
    return "<binary>";
}

#endif // 0

}
