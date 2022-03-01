/*
 * MDM_HAL_UTILITIES is reserved for small pure C/C++ utilities that can be used from cellular_hal or mdm_hal
 */

#ifndef MDM_HAL_UTILITIES_H
#define MDM_HAL_UTILITIES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFINE_NET_PROVIDER_DATA \
    DEFINE_NET_PROVIDER( CELLULAR_NETPROV_TELEFONICA, "spark.telefonica.com", (23*60), (5684) ),  \
    DEFINE_NET_PROVIDER( CELLULAR_NETPROV_KORE_VODAFONE, "vfd1.korem2m.com", (23*60), (5684) ),  \
    DEFINE_NET_PROVIDER( CELLULAR_NETPROV_KORE_ATT, "10569.mcs", (23*60), (5684) ),  \
    DEFINE_NET_PROVIDER( CELLULAR_NETPROV_TWILIO, "super", (23*60), (5684) ),  \
    DEFINE_NET_PROVIDER( CELLULAR_NETPROV_MAX, "", (0), (0) )

#define DEFINE_NET_PROVIDER( idx, apn, keepalive, port )  idx
#ifdef __cplusplus
enum CellularNetProv { DEFINE_NET_PROVIDER_DATA };
#else
typedef enum CellularNetProv CellularNetProv;
#endif

#undef DEFINE_NET_PROVIDER
#define DEFINE_NET_PROVIDER( idx, apn, keepalive, port )  { apn, keepalive, port }

#ifdef __cplusplus
struct CellularNetProvData {
    const char* apn;
    int keepalive;
    uint16_t port;
};
#else
typedef struct CellularNetProvData CellularNetProvData;
#endif

/* detail functions defined for unit tests */
namespace particle { namespace detail {
/**
 * Function for setting the cellular network provider based on the ICCID and/or IMSI of the SIM card inserted, broken out for unit tests
 */
CellularNetProv cellular_sim_to_network_provider_impl(const char* imsi, const char* iccid);

}} // namespace particle detail

#ifdef __cplusplus
}
#endif

#endif  /* MDM_HAL_UTILITIES_H */
