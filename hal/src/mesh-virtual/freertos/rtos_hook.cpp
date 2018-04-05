#include "FreeRTOS.h"
#include "task.h"
#include "service_debug.h"

extern "C" {

#ifdef DEBUG_BUILD
void vApplicationStackOverflowHook(TaskHandle_t, char*) {
    PANIC(StackOverflow, "Stack overflow detected");
}
#endif

} // extern "C"
