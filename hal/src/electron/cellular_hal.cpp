
#include "cellular_hal.h"
#include "modem/mdm_hal.h"
#include "wlan_hal.h"


#define CHECK_SUCCESS(x) { if (!(x)) return -1; }

static CellularCredentials cellularCredentials;

cellular_result_t  cellular_on(void* reserved)
{
    //MDMParser::DevStatus devStatus = {};
    //CHECK_SUCCESS(electronMDM.init(NULL, &devStatus));
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
    //MDMParser::DevStatus netStatus = {};
    //CHECK_SUCCESS(electronMDM.registerNet(&netStatus, (system_tick_t) 300000);
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
    return 0;
}

cellular_result_t cellular_device_info(CellularDevice* device, void* reserved)
{
    const MDMParser::DevStatus* status = electronMDM.getDevStatus();
    strncpy(device->imei, status->imei, sizeof(device->imei));
    strncpy(device->iccid, status->ccid, sizeof(device->iccid));
    return 0;
}

cellular_result_t cellular_fetch_ipconfig(WLanConfig* config)
{
    memset(&config, 0, sizeof(config));
    return 0;
}

cellular_result_t cellular_credentials_set(const char* apn, const char* username, const char* password, void* reserved)
{
    cellularCredentials.apn = apn;
    cellularCredentials.username = username;
    cellularCredentials.password = password;
    return 0;
}

CellularCredentials* cellular_credentials_get(void* reserved)
{
    return &cellularCredentials;
}
