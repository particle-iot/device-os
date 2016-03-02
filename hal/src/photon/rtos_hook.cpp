#include "FreeRTOS.h"
#include "task.h"
#include "service_debug.h"

extern "C" {

void vApplicationStackOverflowHook(xTaskHandle, char*) {
    PANIC(StackOverflow, "Stack overflow detected");
}

} // extern "C"
