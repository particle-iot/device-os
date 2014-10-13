
#include "application.h"
#include "unit-test/unit-test.h"

test(SERIAL_ReadWriteSucceedsWithUserIntervention) {
    //The following code will test all the important USB Serial routines
    char test[] = "hello";
    char message[10];
    // when
    Serial.print("Type the following message and press Enter : ");
    Serial.println(test);
    serialReadLine(&Serial, message, 9, 10000);//10 sec timeout
    Serial.println("");
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(SERIAL1_ReadWriteSucceedsInLoopbackWithTxRxShorted) {
    //The following code will test all the important USART Serial routines
    char test[] = "hello";
    char message[10];
    // when
    Serial1.begin(9600);
    Serial1.println(test);
    serialReadLine(&Serial1, message, 9, 1000);//1 sec timeout
    // then
    assertTrue(strncmp(test, message, 5)==0);
}
