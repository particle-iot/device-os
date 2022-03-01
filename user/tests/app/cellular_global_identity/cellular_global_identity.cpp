/*
 * Project: particle_publish_vitals
 * Description: Confirm the functionality of the `cellular_global_identity` HAL API and confirm diagnostics propogate to cloud.
 * Author: Zachary J. Fields
 * Date: 13th March, 2019
 */

#include "Particle.h"
#include "cellular_hal.h"

// Log handler processing all messages
SerialLogHandler logHandler(LOG_LEVEL_ALL);

CellularGlobalIdentity cgi{.size = sizeof(CellularGlobalIdentity), .version = CGI_VERSION_LATEST};

// setup() runs once, when the device is first turned on.
void setup()
{
  // Put initialization like pinMode and begin functions here.
  Particle.publishVitals(30);
}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
  cellular_result_t result = ::cellular_global_identity(&cgi, nullptr);
  switch (result)
  {
  case SYSTEM_ERROR_NONE:
    Log("/%%%%%%%%%%%%%%%%%%%%%% Cellular Global Identity %%%%%%%%%%%%%%%%%%%%%%/");
    Log("\tMobile Country Code: %d", cgi.mobile_country_code);
    if (CGI_FLAG_TWO_DIGIT_MNC & cgi.cgi_flags) {
      Log("\tMobile Network Code: %02d", cgi.mobile_network_code);
    } else {
      Log("\tMobile Network Code: %03d", cgi.mobile_network_code);
    }
    Log("\tLocation Area Code: 0x%x", cgi.location_area_code);
    Log("\tCell Identification: 0x%lx", cgi.cell_id);
    Log("/%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%/");
    break;
  default:
    Log("ERROR <%d>: Failed to acquire cellular global identity!", result);
  }
  delay(30000);
}
