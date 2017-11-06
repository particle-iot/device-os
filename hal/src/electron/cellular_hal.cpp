#ifndef HAL_CELLULAR_EXCLUDE

#include "modem/mdm_hal.h"
#include "cellular_hal.h"
#include "cellular_internal.h"
#include "system_error.h"
#include <limits>
#include <cmath>
#include "net_hal.h"

#define CHECK_SUCCESS(x) { if (!(x)) return -1; }

static CellularCredentials cellularCredentials;

static CellularNetProv cellularNetProv = CELLULAR_NETPROV_TELEFONICA;

// CELLULAR_NET_PROVIDER_DATA[CELLULAR_NETPROV_MAX - 1] is the last provider record
const CellularNetProvData CELLULAR_NET_PROVIDER_DATA[] = { DEFINE_NET_PROVIDER_DATA };

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE

static HAL_NET_Callbacks netCallbacks = { 0 };

void HAL_NET_notify_connected()
{
    if (netCallbacks.notify_connected) {
        netCallbacks.notify_connected();
    }
}

void HAL_NET_notify_disconnected()
{
    if (netCallbacks.notify_disconnected) {
        netCallbacks.notify_disconnected();
    }
}

void HAL_NET_notify_dhcp(bool dhcp)
{
    if (netCallbacks.notify_dhcp) {
        netCallbacks.notify_dhcp(dhcp); // dhcp dhcp
    }
}

void HAL_NET_notify_can_shutdown()
{
    if (netCallbacks.notify_can_shutdown) {
        netCallbacks.notify_can_shutdown();
    }
}

void HAL_NET_SetCallbacks(const HAL_NET_Callbacks* callbacks, void* reserved)
{
    netCallbacks.notify_connected = callbacks->notify_connected;
    netCallbacks.notify_disconnected = callbacks->notify_disconnected;
    netCallbacks.notify_dhcp = callbacks->notify_dhcp;
    netCallbacks.notify_can_shutdown = callbacks->notify_can_shutdown;
}

#else

void HAL_NET_SetCallbacks(const HAL_NET_Callbacks* callbacks, void* reserved)
{
}

#endif /* defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE */

cellular_result_t  cellular_on(void* reserved)
{
    CHECK_SUCCESS(electronMDM.powerOn());
    return 0;
}

cellular_result_t  cellular_init(void* reserved)
{
    CHECK_SUCCESS(electronMDM.init());
    return 0;
}

cellular_result_t  cellular_off(void* reserved)
{
    CHECK_SUCCESS(electronMDM.powerOff());
    return 0;
}

cellular_result_t  cellular_register(void* reserved)
{
    CHECK_SUCCESS(electronMDM.registerNet());
    return 0;
}

cellular_result_t  cellular_pdp_activate(CellularCredentials* connect, void* reserved)
{
    if (strcmp(connect->apn,"") != 0) {
        CHECK_SUCCESS(electronMDM.pdp(connect->apn));
    }
    else {
        CHECK_SUCCESS(electronMDM.pdp());
    }
    return 0;
}

cellular_result_t  cellular_pdp_deactivate(void* reserved)
{
    CHECK_SUCCESS(electronMDM.disconnect());
    return 0;
}

cellular_result_t  cellular_gprs_attach(CellularCredentials* connect, void* reserved)
{
    if (strcmp(connect->apn,"") != 0 || strcmp(connect->username,"") != 0 || strcmp(connect->password,"") != 0 ) {
        CHECK_SUCCESS(electronMDM.join(connect->apn, connect->username, connect->password));
    }
    else {
        CHECK_SUCCESS(electronMDM.join(CELLULAR_NET_PROVIDER_DATA[cellularNetProv].apn, NULL, NULL));
    }
    return 0;
}

cellular_result_t  cellular_gprs_detach(void* reserved)
{
    CHECK_SUCCESS(electronMDM.detach());
    HAL_NET_notify_disconnected();
    return 0;
}

cellular_result_t cellular_device_info(CellularDevice* device, void* reserved)
{
    const DevStatus* status = electronMDM.getDevStatus();
    if (!*status->ccid)
        electronMDM.init(); // attempt to fetch the info again in case the SIM card has been inserted.
    // this would benefit from an unsolicited event to call electronMDM.init() automatically on sim card insert)
    strncpy(device->imei, status->imei, sizeof(device->imei));
    strncpy(device->iccid, status->ccid, sizeof(device->iccid));
    return 0;
}

cellular_result_t cellular_fetch_ipconfig(CellularConfig* config, void* reserved)
{
    uint32_t ip_addr = electronMDM.getIpAddress(); // Local IP
    if (ip_addr > 0) {
        memcpy(&config->nw.aucIP, &ip_addr, 4);
        return 0;
    }
    return -1;
}

cellular_result_t cellular_credentials_set(const char* apn, const char* username, const char* password, void* reserved)
{
    cellularCredentials.apn = apn;
    cellularCredentials.username = username;
    cellularCredentials.password = password;
    return 0;
}

// todo - better to have the caller pass CellularCredentials and copy the details out according to the size of the struct given.
CellularCredentials* cellular_credentials_get(void* reserved)
{
    return &cellularCredentials;
}

bool cellular_sim_ready(void* reserved)
{
    const DevStatus* status = electronMDM.getDevStatus();
    return status->sim == SIM_READY;
}

// Todo rename me, and allow the different connect, disconnect etc. timeouts be set by the HAL
uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInuS)
{
    return 0;
}

void cellular_cancel(bool cancel, bool calledFromISR, void*)
{
    if (cancel) {
        electronMDM.cancel();
    } else {
        electronMDM.resume();
    }
}

cellular_result_t cellular_signal(CellularSignalHal* signal, cellular_signal_t* signalext)
{
    // % * 100, see 3GPP TS 45.008 8.2.4
    // 0.14%, 0.28%, 0.57%, 1.13%, 2.26%, 4.53%, 9.05%, 18.10%
    static const uint16_t berMapping[] = {14, 28, 57, 113, 226, 453, 905, 1810};

    cellular_result_t res = SYSTEM_ERROR_NONE;
    if (signal == nullptr && signalext == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    NetStatus status;
    CHECK_SUCCESS(electronMDM.getSignalStrength(status));
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
        default:
            signalext->rat = NET_ACCESS_TECHNOLOGY_NONE;
            break;
        }
        switch (status.act) {
        case ACT_GSM:
        case ACT_EDGE:
            // Convert to dBm [-111, -48], see 3GPP TS 45.008 8.1.4
            // Reported multiplied by 100
            signalext->rssi = ((status.rxlev != 99) ? status.rxlev - 111 : -111) * 100;

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
                signalext->bep = bepLevel >= 0 ? (-(bepLevel - 31) * 10 - 3.7) : bepLevel;
            }

            // RSSI in % [0, 100] based on [-111, -48] range mapped to [0, 65535] integer range
            signalext->strength = (status.rxlev != 99) ? status.rxlev * 65535 / 63 : std::numeric_limits<int32_t>::min();
            // Quality based on RXQUAL in % [0, 100] mapped to [0, 65535] integer range
            signalext->quality = (status.rxqual != 99) ? (7 - status.rxqual) * 65535 / 7 : std::numeric_limits<int32_t>::min();
            break;
        case ACT_UTRAN:
            // Convert to dBm [-121, -25], see 3GPP TS 25.133 9.1.1.3
            // Reported multiplied by 100
            signalext->rscp = ((status.rscp != 255) ? status.rscp - 116 : -121) * 100;
            // Convert to Ec/Io (dB) [-24.5, 0], see 3GPP TS 25.133 9.1.2.3
            // Report multiplied by 100
            signalext->ecno = (status.ecno != 255) ? status.ecno * 50 - 2450 : -2450;

            // RSCP in % [0, 100] based on [-121, -25] range mapped to [0, 65535] integer range
            signalext->strength = (status.rscp != 255) ? (status.rscp + 5) * 65535 / 96 : std::numeric_limits<int32_t>::min();
            // Quality based on Ec/Io in % [0, 100] mapped to [0,65535] integer range
            signalext->quality = (status.ecno != 255) ? status.ecno * 65535 / 49 : std::numeric_limits<int32_t>::min();
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

cellular_result_t cellular_command(_CALLBACKPTR_MDM cb, void* param,
                          system_tick_t timeout_ms, const char* format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    electronMDM.sendFormated(buf);

    return electronMDM.waitFinalResp((MDMParser::_CALLBACKPTR)cb, (void*)param, timeout_ms);
}

cellular_result_t _cellular_data_usage_set(CellularDataHal &data, const MDM_DataUsage &data_usage, bool ret)
{
    if (!ret) {
        data.cid = -1; // let the caller know this object was not updated
        return data.cid;
    }
    // Now update the data usage values in the provided CellularDataHal object
    data.cid = data_usage.cid;
    // Offset = Requested Set Count - Actual Modem Current Count
    // Current Count is the Requested Set Count
    data.tx_session_offset = data.tx_session - data_usage.tx_session;
    data.rx_session_offset = data.rx_session - data_usage.rx_session;
    data.tx_total_offset = data.tx_total - data_usage.tx_total;
    data.rx_total_offset = data.rx_total - data_usage.rx_total;
    return 0;
}

cellular_result_t cellular_data_usage_set(CellularDataHal* data, void* reserved)
{
    // First get a copy of the current data usage values
    MDM_DataUsage data_usage;
    bool rv = electronMDM.getDataUsage(data_usage);
    // Now process the Set request
    return _cellular_data_usage_set(*data, data_usage, rv);
}

cellular_result_t _cellular_data_usage_get(CellularDataHal& data, const MDM_DataUsage &data_usage, bool ret)
{
    if (!ret) {
        data.cid = -1; // let the caller know this object was not updated
        return data.cid;
    }
    // Now update the data usage values in the provided CellularDataHal object
    data.cid = data_usage.cid; // set the CID
    // If any counts are decreasing, suspect modem counters have been set/reset
    // to a lower value. Rebase count at current count by updating offsets.
    if ( (data.tx_session > (data_usage.tx_session + data.tx_session_offset))
         || (data.rx_session > (data_usage.rx_session + data.rx_session_offset))
         || (data.tx_total > (data_usage.tx_total + data.tx_total_offset))
         || (data.rx_total > (data_usage.rx_total + data.rx_total_offset)) )
    {
        // Offset = Current Count - Actual Modem Current Count
        // Current Count is the old Current Count (remains unchanged)
        data.tx_session_offset = data.tx_session - data_usage.tx_session;
        data.rx_session_offset = data.rx_session - data_usage.rx_session;
        data.tx_total_offset = data.tx_total - data_usage.tx_total;
        data.rx_total_offset = data.rx_total - data_usage.rx_total;
    }
    else
    {
        // Current Count = Actual Modem Current Count + Offset
        data.tx_session = data_usage.tx_session + data.tx_session_offset;
        data.rx_session = data_usage.rx_session + data.rx_session_offset;
        data.tx_total = data_usage.tx_total + data.tx_total_offset;
        data.rx_total = data_usage.rx_total + data.rx_total_offset;
    }
    return 0;
}

cellular_result_t cellular_data_usage_get(CellularDataHal* data, void* reserved)
{
    // First get a copy of the current data usage values
    MDM_DataUsage data_usage;
    bool rv = electronMDM.getDataUsage(data_usage);
    // Now process the Get request
    return _cellular_data_usage_get(*data, data_usage, rv);
}

cellular_result_t cellular_band_select_set(MDM_BandSelect* bands, void* reserved)
{
    CHECK_SUCCESS(electronMDM.setBandSelect(*bands));
    return 0;
}

cellular_result_t cellular_band_select_get(MDM_BandSelect* bands, void* reserved)
{
    CHECK_SUCCESS(electronMDM.getBandSelect(*bands));
    return 0;
}

cellular_result_t cellular_band_available_get(MDM_BandSelect* bands, void* reserved)
{
    CHECK_SUCCESS(electronMDM.getBandAvailable(*bands));
    return 0;
}

cellular_result_t cellular_sms_received_handler_set(_CELLULAR_SMS_CB_MDM cb, void* data, void* reserved)
{
    if (cb) {
        electronMDM.setSMSreceivedHandler((MDMParser::_CELLULAR_SMS_CB)cb, (void*)data);
        return 0;
    }
    return -1;
}

cellular_result_t cellular_pause(void* reserved)
{
    electronMDM.pause();
    return 0;
}

cellular_result_t cellular_resume(void* reserved)
{
    electronMDM.resume();
    return 0;
}

cellular_result_t cellular_imsi_to_network_provider(void* reserved)
{
    const DevStatus* status = electronMDM.getDevStatus();
    cellularNetProv = detail::_cellular_imsi_to_network_provider(status->imsi);
    return 0;
}

const CellularNetProvData cellular_network_provider_data_get(void* reserved)
{
    return CELLULAR_NET_PROVIDER_DATA[cellularNetProv];
}

#endif // !defined(HAL_CELLULAR_EXCLUDE)
