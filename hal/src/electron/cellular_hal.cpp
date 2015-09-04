
#include "cellular_hal.h"
#include "modem/mdm_hal.h"
#include "wlan_hal.h"


#define CHECK_SUCCESS(x) { if (!(x)) return -1; }

cellular_result_t  cellular_on(void* reserved)
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

cellular_result_t  cellular_pdp_activate(CellularConnect* connect, void* reserved)
{
    CHECK_SUCCESS(electronMDM.pdp(connect->apn));
    return 0;
}

cellular_result_t  cellular_pdp_deactivate(void* reserved)
{
    CHECK_SUCCESS(electronMDM.disconnect());
    return 0;
}

cellular_result_t  cellular_gprs_attach(CellularConnect* connect, void* reserved)
{
    CHECK_SUCCESS(electronMDM.join(connect->apn, connect->username, connect->password));
    return 0;
}

cellular_result_t  cellular_gprs_detach(void* reserved)
{
    CHECK_SUCCESS(electronMDM.detach());
    return 0;
}

cellular_result_t cellular_device_info(CellularDevice* device, void* reserved)
{
    const MDMParser::DevStatus* status = electronMDM.getDevStatus();
    strncpy(device->imei, status->imei, sizeof(device->imei));
    strncpy(device->iccid, status->ccid, sizeof(device->imei));
    return 0;
}

cellular_result_t cellular_fetch_ipconfig(WLanConfig* config)
{
    memset(&config, 0, sizeof(config));
    return 0;
}