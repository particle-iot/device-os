
#include "cellular_hal.h"
#include "modem/mdm_hal.h"

#include "parser.h"


#define CHECK_SUCCESS(x) { if (!(x)) return -1; }

cellular_result_t  cellular_init(void* reserved)
{
    CHECK_SUCCESS(electronMDM.init());
    return 0;
}

cellular_result_t  cellular_on(void* reserved)
{
    CHECK_SUCCESS(electronMDM.init());
    CHECK_SUCCESS(electronMDM.registerNet());
    return 0;
}

cellular_result_t  cellular_off(void* reserved)
{
    CHECK_SUCCESS(electronMDM.powerOff());
    return 0;
}

cellular_result_t  cellular_connect(void* reserved)
{
    CHECK_SUCCESS(electronMDM.join());
    return 0;
}

cellular_result_t  cellular_disconnect(void* reserved)
{
    CHECK_SUCCESS(electronMDM.disconnect());
    return 0;
}

cellular_result_t cellular_join(CellularConnect* connect, void* reserved)
{
    CHECK_SUCCESS(electronMDM.join(connect->apn, connect->username, connect->password));
    return 0;
}

cellular_result_t cellular_device_info(CellularDevice* device, void* reserved)
{
    const MDMParser::DevStatus* status = electronMDM.getDevStatus();
    strncpy(device->imei, status->imei, sizeof(device->imei));
    strncpy(device->iccid, status->ccid, sizeof(device->imei));

    return 0;
}
