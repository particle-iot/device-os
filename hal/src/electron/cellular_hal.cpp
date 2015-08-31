
#include "cellular_hal.h"
#include "modem/mdm_hal.h"

#include "parser.h"

int cellular_device_info(CellularDevice* device, void* reserved)
{
    const MDMParser::DevStatus* status = electronMDM.getDevStatus();
    strncpy(device->imei, status->imei, sizeof(device->imei));
    strncpy(device->iccid, status->ccid, sizeof(device->imei));

    return 0;
}
