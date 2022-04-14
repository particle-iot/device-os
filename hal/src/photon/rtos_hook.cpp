#include "FreeRTOS.h"
#include "task.h"
#include "service_debug.h"
#include <reent.h>

extern "C" {

void vApplicationStackOverflowHook(xTaskHandle handle, char* name) {
    PANIC(StackOverflow, "Stack overflow detected");
}

// Variant of the log_printf() function for use in WICED
void log_printf_wiced(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_printf_v(LOG_LEVEL_TRACE, "wiced", nullptr /* reserved */, fmt, args);
    va_end(args);
}

void reent_init_ptr(struct _reent* r, size_t size) {
    SPARK_ASSERT(size >= sizeof(struct _reent));
    _REENT_INIT_PTR(r);
}

} // extern "C"
