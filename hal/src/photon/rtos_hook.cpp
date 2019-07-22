#include "FreeRTOS.h"
#include "task.h"
#include "service_debug.h"

extern "C" {

void vApplicationStackOverflowHook(xTaskHandle handle, char* name) {
    PANIC(StackOverflow, "Stack overflow detected");
}

// Variant of the log_printf() function for use in WICED
void log_printf_wiced(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char fmttmp[255] = {};
    strncpy(fmttmp, fmt, sizeof(fmttmp) - 1);
    if (fmt != nullptr) {
        const size_t len = strlen(fmttmp);
        if (len > 0) {
            if (fmttmp[len - 1] == '\n') {
                if (fmttmp[len - 2] != '\r' && len < 254) {
                    fmttmp[len - 1] = '\r';
                    fmttmp[len] = '\n';
                }
            }
        }
    }
    log_printf_v(LOG_LEVEL_TRACE, "wiced", nullptr /* reserved */, fmttmp, args);
    va_end(args);
}

} // extern "C"
