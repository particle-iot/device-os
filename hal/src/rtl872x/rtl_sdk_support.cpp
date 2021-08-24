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

#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ERROR

#include <cstdio>
#include <cstdarg>
#include <cstdint>
extern "C" {
#include "device_lock.h"
#include "osdep_service.h"
#include "rtl8721d_freertos_pmu.h"
#include "rtl8721d_ipc_api.h"
#include "basic_types.h"
#include "hal_platform_rtl.h"
#include "rtl8721d_vector.h"
#include "rtl8721d_tim.h"
#include "rtl8721d_delay.h"
}
#include "static_recursive_mutex.h"
#include "static_recursive_cs.h"
#include <mbedtls/config.h>
#include <type_traits>
#include <malloc.h>
#include "concurrent_hal.h"
#include "delay_hal.h"
#include <atomic>
#include "timer_hal.h"
#include <algorithm>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "strproc.h"

// struct FreeRtosTask 			/* The old naming convention is used to prevent breaking kernel aware debuggers. */
// {
// 	volatile StackType_t	*pxTopOfStack;	/*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */

// 	#if ( portUSING_MPU_WRAPPERS == 1 )
// 		xMPU_SETTINGS	xMPUSettings;		/*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE TCB STRUCT. */
// 	#endif

// 	ListItem_t			xStateListItem;	/*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
// 	ListItem_t			xEventListItem;		/*< Used to reference a task from an event list. */
// 	UBaseType_t			uxPriority;			/*< The priority of the task.  0 is the lowest priority. */
// 	StackType_t			*pxStack;			/*< Points to the start of the stack. */
// 	char				pcTaskName[ configMAX_TASK_NAME_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */ /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

// 	#if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
// 		StackType_t		*pxEndOfStack;		/*< Points to the highest valid address for the stack. */
// 	#endif

// 	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
// 		UBaseType_t		uxCriticalNesting;	/*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
// 	#endif

// 	#if ( configUSE_TRACE_FACILITY == 1 )
// 		UBaseType_t		uxTCBNumber;		/*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
// 		UBaseType_t		uxTaskNumber;		/*< Stores a number specifically for use by third party trace code. */
// 	#endif

// 	#if ( configUSE_MUTEXES == 1 )
// 		UBaseType_t		uxBasePriority;		/*< The priority last assigned to the task - used by the priority inheritance mechanism. */
// 		UBaseType_t		uxMutexesHeld;
// 	#endif

// 	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
// 		TaskHookFunction_t pxTaskTag;
// 	#endif

// 	#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
// 		void			*pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
// 	#endif

// 	#if( configGENERATE_RUN_TIME_STATS == 1 )
// 		uint32_t		ulRunTimeCounter;	/*< Stores the amount of time the task has spent in the Running state. */
// 	#endif

// 	#if ( configUSE_NEWLIB_REENTRANT == 1 )
// 		/* Allocate a Newlib reent structure that is specific to this task.
// 		Note Newlib support has been included by popular demand, but is not
// 		used by the FreeRTOS maintainers themselves.  FreeRTOS is not
// 		responsible for resulting newlib operation.  User must be familiar with
// 		newlib and must provide system-wide implementations of the necessary
// 		stubs. Be warned that (at the time of writing) the current newlib design
// 		implements a system-wide malloc() that must be provided with locks. */
// 		struct	_reent xNewLib_reent;
// 	#endif

// 	#if( configUSE_TASK_NOTIFICATIONS == 1 )
// 		volatile uint32_t ulNotifiedValue;
// 		volatile uint8_t ucNotifyState;
// 	#endif

// 	/* See the comments in FreeRTOS.h with the definition of
// 	tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE. */
// 	#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 ) /*lint !e731 !e9029 Macro has been consolidated for readability reasons. */
// 		uint8_t	ucStaticallyAllocated; 		/*< Set to pdTRUE if the task is a statically allocated to ensure no attempt is made to free the memory. */
// 	#endif

// 	#if( INCLUDE_xTaskAbortDelay == 1 )
// 		uint8_t ucDelayAborted;
// 	#endif

// 	#if( configUSE_POSIX_ERRNO == 1 )
// 		int iTaskErrno;
// 	#endif

// };

// using namespace particle;

// extern "C" void* pxCurrentTCB;

// namespace {

// StaticRecursiveMutex sMutexes[RT_DEV_LOCK_MAX];
// StaticRecursiveCriticalSectionLock sCsLock;

// void dump() {
//     FreeRtosTask* task = (FreeRtosTask*)pxCurrentTCB;
//     LOG(INFO, "task=%s isr=%d", task->pcTaskName, hal_interrupt_is_isr());
// }

// } // anonymous

extern "C" {

// Declaring here in order not to include headers that have a lot of unnecessary includes
// and also not exactly correct declarations.
int _rtl_vsprintf(char *buf, size_t size, const char *fmt, va_list args);
int _rtl_printf(const char* fmt, ...);
int _rtl_sprintf(char* str, const char* fmt, ...);
int _rtl_snprintf(char* str, size_t size, const char* fmt, ...);
int _rtl_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int _rtl_sscanf(const char *buf, const char *fmt, ...);
void save_and_cli(void);
void restore_flags(void);
u8 pmu_get_roaming_awake(u8 *p);
void cli(void);
uint32_t pmu_yield_os_check(void);
void pmu_acquire_wakelock(uint32_t nDeviceId);
void pmu_release_wakelock(uint32_t nDeviceId);

}

// void save_and_cli(void) {
//     sCsLock.lock();
// }

// void cli(void) {
//     sCsLock.lock();
// }

// void restore_flags(void) {
//     sCsLock.unlock();
// }

// uint32_t pmu_yield_os_check(void) {
//     return 1;
// }

// void pmu_acquire_wakelock(uint32_t nDeviceId) {
// }
// void pmu_release_wakelock(uint32_t nDeviceId){
// }

#define LOG_BUFFER_SIZE				512
typedef struct {
	char buffer[LOG_BUFFER_SIZE];
	/* please define member after buffer */
} log_buffer_t;
extern log_buffer_t log_buffer[];
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

// void device_mutex_lock(RT_DEV_LOCK_E device) {
//     if (device >= RT_DEV_LOCK_MAX) {
//         return;
//     }
//     sMutexes[device].lock();
// }

// void device_mutex_unlock(RT_DEV_LOCK_E device) {
//     if (device >= RT_DEV_LOCK_MAX) {
//         return;
//     }
//     sMutexes[device].unlock();
// }

// namespace osdep {

// u8* malloc(u32 sz) {
//     return (u8*)::malloc(sz);
// }

// u8* zmalloc(u32 sz) {
//     return (u8*)calloc(1, sz);
// }

// void free(u8 *pbuf, u32 sz) {
//     ::free(pbuf);
// }

// void memcpy(void* dst, void* src, u32 sz) {
//     ::memcpy(dst, src, sz);
// }

// int	memcmp(void *dst, void *src, u32 sz) {
//     return ::memcmp(dst, src, sz);
// }

// void memset(void *pbuf, int c, u32 sz) {
//     ::memset(pbuf, c, sz);
// }

// void dummy_fail(const char* name) {
//     LOG(INFO, "osdep fail: %s", name);
//     while(true) {
//         asm volatile("nop");
//     }
// }

// u32 getFreeHeapSize(void) {
//     auto m = mallinfo();
//     const size_t total = m.uordblks + m.fordblks;
//     // LOG(TRACE, "Heap: %lu/%lu Kbytes used", m.uordblks / 1000, total / 1000);
//     return total - m.uordblks;
// }

// void mutex_init(_mutex *pmutex) {
//     os_mutex_create(pmutex);
// }

// void mutex_free(_mutex *pmutex) {
//     if(*pmutex != nullptr) {
//         os_mutex_destroy(*pmutex);
//     }

//     *pmutex = nullptr;
// }

// void mutex_get(_mutex *pmutex) {
//     os_mutex_lock(*pmutex);
//     dump();
//     LOG(INFO, "mutex get=%x count=%d", pmutex, uxSemaphoreGetCount(*pmutex));
// }

// void mutex_put(_mutex *pmutex) {
//     dump();
//     LOG(INFO, "mutex put=%x count=%d", pmutex, uxSemaphoreGetCount(*pmutex));
//     os_mutex_unlock(*pmutex);
// }

// void enter_critical(_lock *plock, _irqL *pirqL) {
// 	save_and_cli();
// }

// void exit_critical(_lock *plock, _irqL *pirqL) {
// 	restore_flags();
// }

// void enter_critical_from_isr(_lock *plock, _irqL *pirqL) {
//     LOG(INFO, "critical from isr");
//     enter_critical(plock, pirqL);
// }

// void exit_critical_from_isr(_lock *plock, _irqL *pirqL) {
//     LOG(INFO, "exit critical from isr");
//     exit_critical(plock, pirqL);
// }

// int enter_critical_mutex(_mutex *pmutex, _irqL *pirqL) {
//     dump();
//     LOG(INFO, "enter critical mutex=%x count=%d", pmutex, uxSemaphoreGetCount(*pmutex));
// 	return os_mutex_lock(*pmutex);
// }

// void exit_critical_mutex(_mutex *pmutex, _irqL *pirqL) {
//     dump();
//     LOG(INFO, "exit critical mutex=%x count=%d", pmutex, uxSemaphoreGetCount(*pmutex));
// 	os_mutex_unlock(*pmutex);
// }

// void mdelay_os(int ms) {
//     HAL_Delay_Milliseconds(ms);
// }

// void spinlock_init(_lock *plock) {
//     mutex_init(plock);
// }

// void spinlock_free(_lock *plock) {
//     mutex_free(plock);
// }

// void spin_lock(_lock *plock) {
//     dump();
//     LOG(INFO, "spin lock=%x count=%d", plock, uxSemaphoreGetCount(*plock));
//     mutex_get(plock);
// }

// void spin_unlock(_lock *plock) {
//     dump();
//     LOG(INFO, "spin unlock=%x count=%d", plock, uxSemaphoreGetCount(*plock));
//     mutex_put(plock);
// }

// void init_sema(_sema *sema, int init_val) {
//     os_semaphore_create(sema, 0xffffffff, init_val);
// }

// void free_sema(_sema *sema) {
// 	os_semaphore_destroy(*sema);
// 	*sema = NULL;
// }

// void up_sema(_sema *sema) {
//     dump();
//     LOG(INFO, "up sema=%x count=%d", sema, uxSemaphoreGetCount(*sema));
// 	int r = !os_semaphore_give(*sema, false);
//     if (!r) {
//         LOG(ERROR, "failed to up sema %x", sema);
//     }
// }

// void up_sema_from_isr(_sema *sema) {
//     LOG(INFO, "up sema from isr");
//     dump();
//     //LOG(INFO, "up isr sema=%x count=%d", sema, uxSemaphoreGetCount(*sema));
// 	up_sema(sema);
// }

// u32 down_timeout_sema(_sema *sema, u32 timeout) {
//     LOG(INFO, "down sema from isr");
//     dump();
//     //LOG(INFO, "down sema=%x count=%d", sema, uxSemaphoreGetCount(*sema));
// 	auto r = !os_semaphore_take(*sema, timeout, false);
//     if (!r) {
//         LOG(ERROR, "failed to down sema %x", sema);
//     }
//     return r;
// }


// void ATOMIC_SET(ATOMIC_T *v, int i) {
// 	static_assert(sizeof(ATOMIC_T) == sizeof(std::atomic_int), "ATOMIC_T should match std::atomic_int size");
//     std::atomic_int& atomic = *((std::atomic_int*)v);
//     atomic.store(i);
// }

// int ATOMIC_READ(ATOMIC_T *v) {
// 	std::atomic_int& atomic = *((std::atomic_int*)v);
//     return atomic.load();
// }

// void ATOMIC_ADD(ATOMIC_T *v, int i) {
// 	std::atomic_int& atomic = *((std::atomic_int*)v);
//     atomic += i;
// }

// void ATOMIC_SUB(ATOMIC_T *v, int i) {
// 	std::atomic_int& atomic = *((std::atomic_int*)v);
//     atomic -= i;
// }

// void ATOMIC_INC(ATOMIC_T *v) {
// 	std::atomic_int& atomic = *((std::atomic_int*)v);
//     atomic++;
// }

// void ATOMIC_DEC(ATOMIC_T *v) {
// 	std::atomic_int& atomic = *((std::atomic_int*)v);
//     atomic--;
// }

// int ATOMIC_ADD_RETURN(ATOMIC_T *v, int i) {
// 	std::atomic_int& atomic = *((std::atomic_int*)v);
//     return atomic.fetch_add(i) + i;
// }

// int ATOMIC_SUB_RETURN(ATOMIC_T *v, int i) {
//     std::atomic_int& atomic = *((std::atomic_int*)v);
// 	return atomic.fetch_sub(i) - i;
// }

// int ATOMIC_INC_RETURN(ATOMIC_T *v) {
// 	std::atomic_int& atomic = *((std::atomic_int*)v);
//     return ++atomic;
// }

// int ATOMIC_DEC_RETURN(ATOMIC_T *v) {
// 	std::atomic_int& atomic = *((std::atomic_int*)v);
//     return --atomic;
// }

// u32 get_current_time(void) {
// 	return HAL_Timer_Get_Milli_Seconds();
// }

// u32 systime_to_ms(u32 systime) {
// 	return systime;
// }

// u32 systime_to_sec(u32 systime) {
// 	return systime / 1000;
// }

// u32 ms_to_systime(u32 ms) {
// 	return ms;
// }

// u32 sec_to_systime(u32 sec) {
// 	return sec * 1000;
// }

// _timerHandle timerCreate(const signed char *pcTimerName, 
// 							  osdepTickType xTimerPeriodInTicks, 
// 							  u32 uxAutoReload, 
// 							  void * pvTimerID, 
// 							  TIMER_FUN pxCallbackFunction) {
//     os_timer_t timer = nullptr;
// 	os_timer_create(&timer, xTimerPeriodInTicks, pxCallbackFunction, pvTimerID, !uxAutoReload, nullptr);
//     LOG(INFO, "timer create %s %x", pcTimerName, timer);
//     return timer;
// }

// u32 timerDelete(_timerHandle xTimer, 
// 							   osdepTickType xBlockTime) {
// 	return !os_timer_destroy(xTimer, nullptr);
// }

// u32 timerIsTimerActive(_timerHandle xTimer) {
// 	return os_timer_is_active(xTimer, nullptr);
// }

// u32 timerStop(_timerHandle xTimer, 
// 							   osdepTickType xBlockTime) {
//     LOG(INFO, "timer stop %x", xTimer);
// 	return !os_timer_change(xTimer, OS_TIMER_CHANGE_STOP, hal_interrupt_is_isr(), 0, xBlockTime, nullptr);
// }

// u32  timerChangePeriod( _timerHandle xTimer, 
// 							   osdepTickType xNewPeriod, 
// 							   osdepTickType xBlockTime )
// {
// 	if(xNewPeriod == 0) {
// 		xNewPeriod++;
//     }
//     LOG(INFO, "timer change %x %u %u", xTimer, xNewPeriod, xBlockTime);
//     return !os_timer_change(xTimer, OS_TIMER_CHANGE_PERIOD, hal_interrupt_is_isr(), xNewPeriod, xBlockTime, nullptr);
// }

// void *timerGetID(_timerHandle xTimer) {
//     void* id = nullptr;
//     os_timer_get_id(xTimer, &id);
// 	return id;
// }

// u32 timerStart( _timerHandle xTimer, 
// 							   osdepTickType xBlockTime ) {
//     LOG(INFO, "timer start %x %u", xTimer, xBlockTime);
// 	return !os_timer_change(xTimer, OS_TIMER_CHANGE_START, hal_interrupt_is_isr(), 0, xBlockTime, nullptr);
// }

// u32 timerStartFromISR(_timerHandle xTimer, 
// 							   osdepBASE_TYPE *pxHigherPriorityTaskWoken) {
//     // FIXME
//     if (pxHigherPriorityTaskWoken) {
//         *pxHigherPriorityTaskWoken = 0;
//     }
//     LOG(ERROR, "timer start isr %x", xTimer);
// 	return timerStart(xTimer, 0);
// }

// u32  timerStopFromISR( _timerHandle xTimer, 
// 							   osdepBASE_TYPE *pxHigherPriorityTaskWoken ) {
// 	// FIXME
//     if (pxHigherPriorityTaskWoken) {
//         *pxHigherPriorityTaskWoken = 0;
//     }
//     LOG(ERROR, "timer stop isr %x", xTimer);
// 	return timerStop(xTimer, 0);
// }

// u32  timerReset( _timerHandle xTimer, 
// 							   osdepTickType xBlockTime ) {
//     LOG(INFO, "timer reset %x %u", xTimer, xBlockTime);
// 	return !os_timer_change(xTimer, OS_TIMER_CHANGE_RESET, hal_interrupt_is_isr(), 0, xBlockTime, nullptr);
// }


// u32  timerResetFromISR( _timerHandle xTimer, 
// 							   osdepBASE_TYPE *pxHigherPriorityTaskWoken ) {
//     // FIXME
//     if (pxHigherPriorityTaskWoken) {
//         *pxHigherPriorityTaskWoken = 0;
//     }
//     LOG(ERROR, "timer reset isr %x %u", xTimer);
// 	return timerReset(xTimer, 0);
// }

// u32  timerChangePeriodFromISR( _timerHandle xTimer, 
// 							   osdepTickType xNewPeriod, 
// 							   osdepBASE_TYPE *pxHigherPriorityTaskWoken ) {
//     // FIXME
//     if (pxHigherPriorityTaskWoken) {
//         *pxHigherPriorityTaskWoken = 0;
//     }
//     LOG(ERROR, "timer change period isr %x %u", xTimer, xNewPeriod);
// 	return timerChangePeriod(xTimer, xNewPeriod, 0);
// }

// void udelay_os(int us) {
//     return DelayUs(us);
// }

// void acquire_wakelock(void) {
// }

// void release_wakelock(void) {
// }

// void wakelock_timeout(uint32_t timeout) {
// }

// int create_task(struct task_struct *ptask, const char *name,
// 	u32  stack_size, u32 priority, thread_func_t func, void *thctx) {
//     LOG(INFO, "Creating task %s priority %u stack %u", name, priority, stack_size);
//     return !os_thread_create(&ptask->task, name, OS_THREAD_PRIORITY_DEFAULT + priority - 1, func, thctx, std::max<size_t>(stack_size, 4096));
// }

// void thread_enter(char *name) {
//     LOG(INFO, "thread_enter %s", name);
// }

// void thread_exit(void) {
//     LOG(INFO, "thread_exit");
//     os_thread_exit(nullptr);
// }

// int get_timeout(_mutex *pmutex, u32 timeout_ms) {
//     return !os_mutex_lock_timeout(*pmutex, timeout_ms);
// }

// void msleep_os(int ms) {
//     mdelay_os(ms);
// }

// } // osdep

extern "C" void rtl_wifi_log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogAttributes attr = {};
    log_message_v(LOG_LEVEL_INFO, "rtl", &attr, nullptr, fmt, args);
    va_end(args);
}

// #define RLTK_OSDEP_DUMMY(fieldname) (decltype(osdep_service.fieldname))+[](void) -> void { osdep::dummy_fail(#fieldname); }

// extern "C" const osdep_service_ops osdep_service = {
//     &osdep::malloc,                 // rtw_vmalloc
//     &osdep::zmalloc,                // rtw_zvmalloc
//     &osdep::free,                   // rtw_vmfree
//     &osdep::malloc,                 // rtw_malloc
//     &osdep::zmalloc,                // rtw_zmalloc
//     &osdep::free,                   // rtw_mfree
//     &osdep::memcpy,                 // rtw_memcpy
//     &osdep::memcmp,                 // rtw_memcmp
//     &osdep::memset,                 // rtw_memset
//     &osdep::init_sema,
//     &osdep::free_sema,
//     &osdep::up_sema,
//     &osdep::up_sema_from_isr,
//     &osdep::down_timeout_sema,
//     &osdep::mutex_init,
//     &osdep::mutex_free,
//     &osdep::mutex_get,
//     &osdep::get_timeout,
//     &osdep::mutex_put,
//     &osdep::enter_critical,
//     &osdep::exit_critical,
//     &osdep::enter_critical_from_isr,
//     &osdep::exit_critical_from_isr,
//     RLTK_OSDEP_DUMMY(rtw_enter_critical_bh),
//     RLTK_OSDEP_DUMMY(rtw_exit_critical_bh),
//     &osdep::enter_critical_mutex,
//     &osdep::exit_critical_mutex,
//     RLTK_OSDEP_DUMMY(rtw_cpu_lock),
//     RLTK_OSDEP_DUMMY(rtw_cpu_unlock),
//     &osdep::spinlock_init,
//     &osdep::spinlock_free,
//     &osdep::spin_lock,
//     &osdep::spin_unlock,
//     RLTK_OSDEP_DUMMY(rtw_spinlock_irqsave),
//     RLTK_OSDEP_DUMMY(rtw_spinunlock_irqsave),
//     RLTK_OSDEP_DUMMY(rtw_init_xqueue),
//     RLTK_OSDEP_DUMMY(rtw_push_to_xqueue),
//     RLTK_OSDEP_DUMMY(rtw_pop_from_xqueue),
//     RLTK_OSDEP_DUMMY(rtw_peek_from_xqueue),
//     RLTK_OSDEP_DUMMY(rtw_deinit_xqueue),
//     &osdep::get_current_time,
//     &osdep::systime_to_ms,
//     &osdep::systime_to_sec,
//     &osdep::ms_to_systime,
//     &osdep::sec_to_systime,
//     &osdep::msleep_os,
//     RLTK_OSDEP_DUMMY(rtw_usleep_os),
//     &osdep::mdelay_os,
//     &osdep::udelay_os,
//     RLTK_OSDEP_DUMMY(rtw_yield_os),
//     &osdep::ATOMIC_SET,
//     &osdep::ATOMIC_READ,
//     &osdep::ATOMIC_ADD,
//     &osdep::ATOMIC_SUB,
//     &osdep::ATOMIC_INC,
//     &osdep::ATOMIC_DEC,
//     &osdep::ATOMIC_ADD_RETURN,
//     &osdep::ATOMIC_SUB_RETURN,
//     &osdep::ATOMIC_INC_RETURN,
//     &osdep::ATOMIC_DEC_RETURN,
//     RLTK_OSDEP_DUMMY(rtw_modular64),
//     RLTK_OSDEP_DUMMY(rtw_get_random_bytes),
//     &osdep::getFreeHeapSize,    // rtw_getFreeHeapSize
//     &osdep::create_task,
//     RLTK_OSDEP_DUMMY(rtw_delete_task),
//     RLTK_OSDEP_DUMMY(rtw_wakeup_task),
//     RLTK_OSDEP_DUMMY(rtw_set_priority_task),
//     RLTK_OSDEP_DUMMY(rtw_get_priority_task),
//     RLTK_OSDEP_DUMMY(rtw_suspend_task),
//     RLTK_OSDEP_DUMMY(rtw_resume_task),
//     &osdep::thread_enter,
//     &osdep::thread_exit,
//     &osdep::timerCreate,
//     &osdep::timerDelete,
//     &osdep::timerIsTimerActive,
//     &osdep::timerStop,
//     &osdep::timerChangePeriod,
//     &osdep::timerGetID,
//     &osdep::timerStart,
//     &osdep::timerStartFromISR,
//     &osdep::timerStopFromISR,
//     &osdep::timerResetFromISR,
//     &osdep::timerChangePeriodFromISR,
//     &osdep::timerReset,
//     &osdep::acquire_wakelock,
//     &osdep::release_wakelock,
//     &osdep::wakelock_timeout,
//     RLTK_OSDEP_DUMMY(rtw_get_scheduler_state),
//     RLTK_OSDEP_DUMMY(rtw_create_secure_context),
//     RLTK_OSDEP_DUMMY(rtw_get_current_TaskHandle)
// };

#ifndef MBEDTLS_PLATFORM_MEMORY
extern "C" int mbedtls_platform_set_calloc_free(void*(*)(size_t, size_t), void (*)(void*)) {
    LOG(INFO, "mbedtls_platform_set_calloc_free");
    // mbedtls_calloc_func = calloc_func;
    // mbedtls_free_func = free_func;
    return 0;
}
#endif // MBEDTLS_PLATFORM_MEMORY

// void pmu_set_dev_wakeup_tick(u32 nDeviceId, u32 Ms) {
// }

// uint32_t pmu_set_sysactive_time(uint32_t timeout) {
//     return 0;
// }

// void pmu_register_sleep_callback(u32 nDeviceId, PSM_HOOK_FUN sleep_hook_fun, void* sleep_param_ptr, PSM_HOOK_FUN wakeup_hook_fun, void* wakeup_param_ptr) {
// }

// void pmu_unregister_sleep_callback(u32 nDeviceId) {
// }

// void pmu_set_roaming_awake(u8 enable, u8 threshhold, u8 winsize) {
// }
// void pmu_reset_awake(u8 type) {
// }
// void pmu_degrade_awake(u8 type) {
// }

// uint32_t pmu_get_max_sleep_time(void) {
//     return 0;
// }

// u8 pmu_get_roaming_awake(u8 *p) {
//     return 0;
// }

void ipc_send_message(u8 IPC_ChNum, u32 Message) {
    LOG(INFO, "ipc_send_message");
    return;
}
