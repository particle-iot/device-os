#ifndef HAL_CELLULAR_EXCLUDE

#include "logging.h"
#include "cellular_hal.h"
#include "cellular_internal.h"
#include <stdlib.h>

namespace detail {
    CellularNetProv _cellular_imsi_to_network_provider(const char* imsi) {
        if (imsi && strlen(imsi) > 0) {
            // convert to unsigned long long (imsi can be 15 digits)
            unsigned long long imsi64 = strtoull(imsi, NULL, 10);
            // LOG(INFO,"IMSI: %s %lu%lu", imsi, (uint32_t)(imsi64/100000000), (uint32_t)(imsi64-310260800000000));
            // set network provider based on IMSI range
            if (imsi64 >= 310260859000000 && imsi64 <= 310260859999999) {
                return CELLULAR_NETPROV_TWILIO;
            }
            else {
                return CELLULAR_NETPROV_TELEFONICA;
            }
        }
        return CELLULAR_NETPROV_TELEFONICA; // default to telefonica
    }
} // namespace detail

#endif // !defined(HAL_CELLULAR_EXCLUDE)
