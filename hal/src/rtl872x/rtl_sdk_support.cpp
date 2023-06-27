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
#endif
}

extern "C" pcoex_reveng* pcoex[4];

extern "C" int rtw_coex_wifi_enable(void* priv, uint32_t state);
extern "C" int rtw_coex_bt_enable(void* priv, uint32_t state);

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
void rtwRadioReset() {
    std::lock_guard<RecursiveMutex> lk(radioMutex);
    hal_ble_lock(nullptr);
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
    hal_ble_unlock(nullptr);
}

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
