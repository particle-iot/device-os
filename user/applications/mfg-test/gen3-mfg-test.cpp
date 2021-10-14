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
    tester.loop();
}
