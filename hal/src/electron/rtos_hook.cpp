#include "FreeRTOS.h"
#include "task.h"
#include "service_debug.h"

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
static StaticTask_t idle_thread_handle;
static StaticTask_t timer_thread_handle;
static StackType_t* idle_thread_stack = NULL;
static StackType_t* timer_thread_stack = NULL;
#endif

extern "C" {

#ifdef DEBUG_BUILD
void vApplicationStackOverflowHook(TaskHandle_t, char*) {
    PANIC(StackOverflow, "Stack overflow detected");
}
#endif

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

extern void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
extern void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &idle_thread_handle;
    if (!idle_thread_stack) {
        idle_thread_stack = (StackType_t*)pvPortMalloc(configMINIMAL_STACK_SIZE * sizeof(StackType_t));
    }
    *ppxIdleTaskStackBuffer = idle_thread_stack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
    *ppxTimerTaskTCBBuffer = &timer_thread_handle;
    if (!timer_thread_stack) {
        timer_thread_stack = (StackType_t*)pvPortMalloc(configTIMER_TASK_STACK_DEPTH * sizeof(StackType_t));
    }
    *ppxTimerTaskStackBuffer = timer_thread_stack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif

} // extern "C"
