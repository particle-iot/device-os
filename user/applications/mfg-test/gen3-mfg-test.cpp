#include "application.h"
/*
 * Project gen3-mfg-test
 * Description: Example app for Tron secure provisioning
 * Author:
 * Date:
 */


#include "TesterCommandTypes.h"
#include "FactoryTester.h"

void setup();
void loop();

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

FactoryTester tester;
Serial1LogHandler logHandler(115200); // Baud rate

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);
    
    tester.setup();
}

void loop()
{

}

void ctrl_request_custom_handler(ctrl_request* req) {
    const int r = tester.processUSBRequest(req);
    // TODO: Test error condition
    if (r < 0) {
        system_ctrl_set_result(req, r, nullptr, nullptr, nullptr);
    }
}
