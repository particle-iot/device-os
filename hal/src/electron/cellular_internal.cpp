#ifndef HAL_CELLULAR_EXCLUDE

#include "logging.h"
#include "cellular_hal.h"
#include "cellular_internal.h"
#include <stdlib.h>
#include "system_error.h"
#include <limits>
#include <cmath>
#include "net_hal.h"

namespace detail {

const int MCC_MNC_MIN_SIZE = 5;

CellularNetProv _cellular_imsi_to_network_provider(const char* imsi) {
    if (imsi && strlen(imsi) >= MCC_MNC_MIN_SIZE) {
        if (strncmp(imsi, "21407", 5) == 0) {
            // LOG(INFO, "CELLULAR_NETPROV_TELEFONICA");
            return CELLULAR_NETPROV_TELEFONICA;
        } else if (strncmp(imsi, "310410", 6) == 0) {
            // LOG(INFO, "CELLULAR_NETPROV_KORE_ATT");
            return CELLULAR_NETPROV_KORE_ATT;
        } else if (strncmp(imsi, "20404", 5) == 0) {
            // LOG(INFO, "CELLULAR_NETPROV_KORE_VODAFONE");
            return CELLULAR_NETPROV_KORE_VODAFONE;
        }
    }
    // LOG(INFO, "DEFAULT CELLULAR_NETPROV_TELEFONICA");
    return CELLULAR_NETPROV_TELEFONICA; // default to telefonica
}

cellular_result_t cellular_signal_impl(CellularSignalHal* signal, cellular_signal_t* signalext, bool strengthResult, const NetStatus& status) {
    // % * 100, see 3GPP TS 45.008 8.2.4
    // 0.14%, 0.28%, 0.57%, 1.13%, 2.26%, 4.53%, 9.05%, 18.10%
    static const uint16_t berMapping[] = {14, 28, 57, 113, 226, 453, 905, 1810};

    cellular_result_t res = SYSTEM_ERROR_NONE;

    if (strengthResult != true) {
        return SYSTEM_ERROR_UNKNOWN;
    }

    if (signal != nullptr) {
        signal->rssi = status.rssi;
        signal->qual = status.qual;
    }

    if (signalext != nullptr) {
        switch (status.act) {
        case ACT_GSM:
            signalext->rat = NET_ACCESS_TECHNOLOGY_GSM;
            break;
        case ACT_EDGE:
            signalext->rat = NET_ACCESS_TECHNOLOGY_EDGE;
            break;
        case ACT_UTRAN:
            signalext->rat = NET_ACCESS_TECHNOLOGY_UTRAN;
            break;
        case ACT_LTE:
            signalext->rat = NET_ACCESS_TECHNOLOGY_LTE;
            break;
        case ACT_LTE_CAT_M1:
            signalext->rat = NET_ACCESS_TECHNOLOGY_LTE_CAT_M1;
            break;
        case ACT_LTE_CAT_NB1:
            signalext->rat = NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1;
            break;
        default:
            signalext->rat = NET_ACCESS_TECHNOLOGY_NONE;
            break;
        }
        switch (status.act) {
        case ACT_GSM:
        case ACT_EDGE:
            // Convert to dBm [-111, -48], see 3GPP TS 45.008 8.1.4
            // Reported multiplied by 100
            signalext->rssi = (status.rxlev != 99) ? (status.rxlev - 111) * 100 : std::numeric_limits<int32_t>::min();

            // NOTE: From u-blox AT Command Reference manual:
            // SARA-U260-00S / SARA-U270-00S / SARA-U270-00X / SARA-U280-00S / LISA-U200-00S /
            // LISA-U200-01S / LISA-U200-02S / LISA-U200-52S / LISA-U200-62S / LISA-U230 / LISA-U260 /
            // LISA-U270 / LISA-U1
            // The <qual> parameter is not updated in GPRS and EGPRS packet transfer mode
            if (status.act == ACT_GSM) {
                // Convert to BER (% * 100), see 3GPP TS 45.008 8.2.4
                signalext->ber = (status.rxqual != 99) ? berMapping[status.rxqual] : std::numeric_limits<int32_t>::min();
            } else /* status.act == ACT_EDGE */ {
                // Convert to MEAN_BEP level first
                // See u-blox AT Reference Manual:
                // In 2G RAT EGPRS packet transfer mode indicates the Mean Bit Error Probability (BEP) of a radio
                // block. 3GPP TS 45.008 [148] specifies the range 0-31 for the Mean BEP which is mapped to
                // the range 0-7 of <qual>
                int bepLevel = (status.rxqual != 99) ? (7 - status.rxqual) * 31 / 7 : std::numeric_limits<int32_t>::min();
                // Convert to log10(MEAN_BEP) multiplied by 100, see 3GPP TS 45.008 10.2.3.3
                // Uses QPSK table
                signalext->bep = bepLevel >= 0 ? (-(bepLevel - 31) * 10 - 370) : bepLevel;
            }

            // RSSI in % [0, 100] based on [-111, -48] range mapped to [0, 65535] integer range
            signalext->strength = (status.rxlev != 99) ? status.rxlev * 65535 / 63 : std::numeric_limits<int32_t>::min();
            // Quality based on RXQUAL in % [0, 100] mapped to [0, 65535] integer range
            signalext->quality = (status.rxqual != 99) ? (7 - status.rxqual) * 65535 / 7 : std::numeric_limits<int32_t>::min();
            break;
        case ACT_UTRAN:
            // Convert to dBm [-121, -25], see 3GPP TS 25.133 9.1.1.3
            // Reported multiplied by 100
            signalext->rscp = (status.rscp != 255) ? (status.rscp - 116) * 100 : std::numeric_limits<int32_t>::min();
            // Convert to Ec/Io (dB) [-24.5, 0], see 3GPP TS 25.133 9.1.2.3
            // Report multiplied by 100
            signalext->ecno = (status.ecno != 255) ? status.ecno * 50 - 2450 : std::numeric_limits<int32_t>::min();

            // RSCP in % [0, 100] based on [-121, -25] range mapped to [0, 65535] integer range
            signalext->strength = (status.rscp != 255) ? (status.rscp + 5) * 65535 / 96 : std::numeric_limits<int32_t>::min();
            // Quality based on Ec/Io in % [0, 100] mapped to [0,65535] integer range
            signalext->quality = (status.ecno != 255) ? status.ecno * 65535 / 49 : std::numeric_limits<int32_t>::min();
            break;
        case ACT_LTE:
        case ACT_LTE_CAT_M1:
        case ACT_LTE_CAT_NB1:
            // Convert to dBm [-140, -44], see 3GPP TS 36.133 subclause 9.1.4
            // Reported multiplied by 100
            signalext->rsrp = (status.rsrp != 255) ? (status.rsrp - 141) * 100 : std::numeric_limits<int32_t>::min();
            // Convert to dB [-19.5, -3], see 3GPP TS 36.133 subclause 9.1.7
            // Report multiplied by 100
            signalext->rsrq = (status.rsrq != 255) ? status.rsrq * 50 - 2000 : std::numeric_limits<int32_t>::min();

            // RSRP in % [0, 100] based on [-140, -44] range mapped to [0, 65535] integer range
            signalext->strength = (status.rsrp != 255) ? status.rsrp * 65535 / 97 : std::numeric_limits<int32_t>::min();
            // Quality based on Ec/Io in % [0, 100] mapped to [0,65535] integer range
            signalext->quality = (status.rsrq != 255) ? status.rsrq * 65535 / 34 : std::numeric_limits<int32_t>::min();
            break;
        default:
            res = SYSTEM_ERROR_UNKNOWN;
            signalext->rssi = std::numeric_limits<int32_t>::min();
            signalext->qual = std::numeric_limits<int32_t>::min();
            signalext->strength = 0;
            signalext->quality = 0;
            break;
        }
    }

    return res;
}

} // namespace detail

#endif // !defined(HAL_CELLULAR_EXCLUDE)
