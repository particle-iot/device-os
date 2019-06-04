#ifndef HAL_CELLULAR_EXCLUDE

#include "check.h"
#include "modem/mdm_hal.h"
#include "cellular_hal.h"
#include "cellular_internal.h"
#include "system_error.h"

#define CHECK_SUCCESS(x) { if (!(x)) return -1; }

static CellularCredentials cellularCredentials;

static CellularNetProv cellularNetProv = CELLULAR_NETPROV_TELEFONICA;

// CELLULAR_NET_PROVIDER_DATA[CELLULAR_NETPROV_MAX - 1] is the last provider record
const CellularNetProvData CELLULAR_NET_PROVIDER_DATA[] = { DEFINE_NET_PROVIDER_DATA };

namespace
{

const char* defaultOrUserApn(const CellularCredentials& cred)
{
    if ((cred.apn && *cred.apn) || (cred.username && *cred.username) || (cred.password && *cred.password)) {
        // Use user-provided APN
        return cred.apn;
    } else {
        // Determine APN based on IMSI
        auto ret = cellular_imsi_to_network_provider(nullptr);
        if (ret != 0) {
            // Should never get here since we always return 0 from above
            return cred.apn;
        }
        // If we are potentially still defaulting to Telefonica, check for R410 modem type
        if (cellularNetProv == CELLULAR_NETPROV_TELEFONICA) {
            // Determine APN based on Modem Type
            const DevStatus* const status = electronMDM.getDevStatus();
            if (status->dev == DEV_SARA_R410) {
                cellularNetProv = CELLULAR_NETPROV_KORE_ATT;
            }
        }
        return CELLULAR_NET_PROVIDER_DATA[cellularNetProv].apn;
    }
}

} // namespace

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
    CHECK_SUCCESS(electronMDM.deactivate());
    return 0;
}

cellular_result_t  cellular_gprs_attach(CellularCredentials* connect, void* reserved)
{
    const char* const apn = defaultOrUserApn(*connect);
    CHECK_SUCCESS(electronMDM.join(apn, connect->username, connect->password));
    // These callbacks have been invoked in the system code previously, moving them to HAL for consistency
    // and backward compatibility
    HAL_NET_notify_connected();
    HAL_NET_notify_dhcp(true);
    return 0;
}

cellular_result_t  cellular_gprs_detach(void* reserved)
{
    CHECK_SUCCESS(electronMDM.detach());
    HAL_NET_notify_disconnected();
    return 0;
}

cellular_result_t cellular_connect(void* reserved)
{
    const CellularCredentials& cred = cellularCredentials;
    const char* apn = cred.apn;
    apn = defaultOrUserApn(cred);
    CHECK_SUCCESS(electronMDM.connect(apn, cred.username, cred.password));
    return 0;
}

cellular_result_t cellular_disconnect(void* reserved)
{
    CHECK_SUCCESS(electronMDM.disconnect());
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
    if (device->size >= offsetof(CellularDevice, dev) + sizeof(CellularDevice::dev)) {
        device->dev = status->dev;
    }
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
    // TODO: Store the credentials persistently in the module's NVM or DCT similarly to WLAN HAL
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
    if (signal == nullptr && signalext == nullptr) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    NetStatus status;
    bool r = electronMDM.getSignalStrength(status);

    return detail::cellular_signal_impl(signal, signalext, r, status);
}

cellular_result_t cellular_command(_CALLBACKPTR_MDM cb, void* param,
                          system_tick_t timeout_ms, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    const int ret = electronMDM.sendCommandWithArgs(format, args, cb, param, timeout_ms);
    va_end(args);
    return ret;
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
    const DevStatus* const status = electronMDM.getDevStatus();
    if (status->dev == DEV_SARA_R410) {
        return -1;
    }

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
    const DevStatus* const status = electronMDM.getDevStatus();
    if (status->dev == DEV_SARA_R410) {
        return -1;
    }

    // First get a copy of the current data usage values
    MDM_DataUsage data_usage;
    bool rv = electronMDM.getDataUsage(data_usage);
    // Now process the Get request
    return _cellular_data_usage_get(*data, data_usage, rv);
}

cellular_result_t cellular_band_select_set(MDM_BandSelect* bands, void* reserved)
{
    const DevStatus* const status = electronMDM.getDevStatus();
    if (status->dev == DEV_SARA_R410) {
        return -1;
    }

    CHECK_SUCCESS(electronMDM.setBandSelect(*bands));
    return 0;
}

cellular_result_t cellular_band_select_get(MDM_BandSelect* bands, void* reserved)
{
    const DevStatus* const status = electronMDM.getDevStatus();
    if (status->dev == DEV_SARA_R410) {
        return -1;
    }

    CHECK_SUCCESS(electronMDM.getBandSelect(*bands));
    return 0;
}

cellular_result_t cellular_band_available_get(MDM_BandSelect* bands, void* reserved)
{
    const DevStatus* const status = electronMDM.getDevStatus();
    if (status->dev == DEV_SARA_R410) {
        return -1;
    }

    CHECK_SUCCESS(electronMDM.getBandAvailable(*bands));
    return 0;
}

cellular_result_t cellular_global_identity(CellularGlobalIdentity* cgi_, void* reserved_)
{
    CellularGlobalIdentity cgi;  // Intentionally left uninitialized

    // Validate Argument(s)
    (void)reserved_;
    CHECK_TRUE((nullptr != cgi_), SYSTEM_ERROR_INVALID_ARGUMENT);

    // Load cached data into result struct
    CHECK_TRUE(electronMDM.getCellularGlobalIdentity(cgi), SYSTEM_ERROR_AT_NOT_OK);

    // Validate cache
    CHECK_TRUE(0 != cgi.mobile_country_code, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(0 != cgi.mobile_network_code, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(0xFFFF != cgi.location_area_code, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(0xFFFFFFFF != cgi.cell_id, SYSTEM_ERROR_BAD_DATA);

    // Update result
    *cgi_ = cgi;

    return SYSTEM_ERROR_NONE;
}

cellular_result_t cellular_sms_received_handler_set(_CELLULAR_SMS_CB_MDM cb, void* data,
                                                    void* reserved)
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
    electronMDM.resumeRecv();
    return 0;
}

cellular_result_t cellular_imsi_to_network_provider(void* reserved)
{
    const DevStatus* status = electronMDM.getDevStatus();
    cellularNetProv = detail::_cellular_imsi_to_network_provider(status->imsi);
    return 0;
}

CellularNetProvData cellular_network_provider_data_get(void* reserved)
{
    return CELLULAR_NET_PROVIDER_DATA[cellularNetProv];
}

int cellular_lock(void* reserved)
{
    electronMDM.lock();
    return 0;
}

void cellular_unlock(void* reserved)
{
    electronMDM.unlock();
}

void cellular_set_power_mode(int mode, void* reserved)
{
    if (mode >= 0 && mode <= 3) {
        electronMDM.setPowerMode(mode);
    }
}

#endif // !defined(HAL_CELLULAR_EXCLUDE)
