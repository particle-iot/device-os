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

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

FactoryTester tester;

Serial1LogHandler logHandler(115200, LOG_LEVEL_TRACE);

void setup()
{
    tester.setup();
}

void loop()
{
    // // TODO: Resolve USART issue.
    // if(Serial1.available()){
    //     char received = Serial1.read();
    //     switch(received){
    //         case 'i':
    //             tester.testCommand();
    //             tester.getDeviceId();
    //             Log.info("getDeviceId: %s", tester.get_command_response());
    //             break;
    //         default:
    //             break;
    //     }
    // }
}

void ctrl_request_custom_handler(ctrl_request* req) {
    const int r = tester.processUSBRequest(req);
    // TODO: Test error condition
    if (r < 0) {
        system_ctrl_set_result(req, r, nullptr, nullptr, nullptr);
    }
}
