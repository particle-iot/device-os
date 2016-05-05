
#include "cellular_hal.h"
#include "cellular_internal.h"
#include "modem/mdm_hal.h"

#define CHECK_SUCCESS(x) { if (!(x)) return -1; }

static CellularCredentials cellularCredentials;

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
        CHECK_SUCCESS(electronMDM.join());
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

cellular_result_t cellular_signal(CellularSignalHal &signal, void* reserved)
{
    NetStatus status;
    CHECK_SUCCESS(electronMDM.getSignalStrength(status));
    signal.rssi = status.rssi;
    signal.qual = status.qual;
    return 0;
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
