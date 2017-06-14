#include "mbedtls_util.h"
#include "timer_hal.h"

struct MbedTlsCallbackInitializer {
    MbedTlsCallbackInitializer() {
        mbedtls_callbacks_t cb = {0};
        cb.version = 0;
        cb.size = sizeof(cb);
        // cb.mbedtls_md_list = mbedtls_md_list;
        // cb.mbedtls_md_info_from_string = mbedtls_md_info_from_string;
        // cb.mbedtls_md_info_from_type = mbedtls_md_info_from_type;
        cb.millis = HAL_Timer_Get_Milli_Seconds;
        mbedtls_set_callbacks(&cb, NULL);
    }
};

MbedTlsCallbackInitializer s_mbedtls_callback_initializer;
