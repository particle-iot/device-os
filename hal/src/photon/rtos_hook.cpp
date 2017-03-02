#include "FreeRTOS.h"
#include "task.h"
#include "service_debug.h"

extern "C" {

void vApplicationStackOverflowHook(xTaskHandle handle, char* name) {
    PANIC(StackOverflow, "Stack overflow detected");
}

} // extern "C"
