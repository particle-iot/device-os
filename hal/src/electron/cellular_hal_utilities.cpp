/*
 * MDM_HAL_UTILITIES is reserved for small pure C/C++ utilities that can be used from cellular_hal or mdm_hal
 */

#ifndef HAL_CELLULAR_EXCLUDE

#include "string.h"
#include "cellular_hal_utilities.h"

namespace particle { namespace detail {

const char TWILIO_ICCID_1[] = "8988323";
const char TWILIO_ICCID_2[] = "8988307";
const char TELEFONICA_MCC_MNC[] = "21407";
const char KORE_ATT_MCC_MNC[] = "310410";
const char KORE_ATT2_MCC_MNC[] = "310030";
const char KORE_VODAFONE_MCC_MNC[] = "20404";

const int MCC_MNC_MIN_SIZE = 5;
const int ICCID_MIN_SIZE = 19;

CellularNetProv cellular_sim_to_network_provider_impl(const char* imsi, const char* iccid) {
    if (iccid && strlen(iccid) >= ICCID_MIN_SIZE) {
        if ((strncmp(iccid, TWILIO_ICCID_1, strlen(TWILIO_ICCID_1)) == 0) || (strncmp(iccid, TWILIO_ICCID_2, strlen(TWILIO_ICCID_2)) == 0)) {
            return CELLULAR_NETPROV_TWILIO;
        }
    }

    if (imsi && strlen(imsi) >= MCC_MNC_MIN_SIZE) {
        if (strncmp(imsi, TELEFONICA_MCC_MNC, strlen(TELEFONICA_MCC_MNC)) == 0) {
            // LOG(INFO, "CELLULAR_NETPROV_TELEFONICA");
            return CELLULAR_NETPROV_TELEFONICA;
        } else if (strncmp(imsi, KORE_ATT_MCC_MNC, strlen(KORE_ATT_MCC_MNC)) == 0) {
            // LOG(INFO, "CELLULAR_NETPROV_KORE_ATT");
            return CELLULAR_NETPROV_KORE_ATT;
        } else if (strncmp(imsi, KORE_ATT2_MCC_MNC, strlen(KORE_ATT2_MCC_MNC)) == 0) {
            // LOG(INFO, "CELLULAR_NETPROV_KORE_ATT");
            return CELLULAR_NETPROV_KORE_ATT;
        } else if (strncmp(imsi, KORE_VODAFONE_MCC_MNC, strlen(KORE_VODAFONE_MCC_MNC)) == 0) {
            // LOG(INFO, "CELLULAR_NETPROV_KORE_VODAFONE");
            return CELLULAR_NETPROV_KORE_VODAFONE;
        }
    }
    // LOG(INFO, "DEFAULT CELLULAR_NETPROV_TELEFONICA");
    return CELLULAR_NETPROV_TELEFONICA; // default to telefonica
}

}} // namespace particle detail

#endif // !defined(HAL_CELLULAR_EXCLUDE)