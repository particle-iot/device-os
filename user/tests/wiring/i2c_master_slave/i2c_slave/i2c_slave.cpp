#include "application.h"
#include "unit-test/unit-test.h"
#include "../common/common.inc"

static uint8_t done = 0;
static uint32_t requestedLength = 0;
static bool requested = false;

static int random_range(int minVal, int maxVal)
{
    static unsigned int seed = HAL_RNG_GetRandomNumber();
    return rand_r(&seed) % (maxVal - minVal + 1) + minVal;
}

void I2C_Slave_On_Request_Callback(void) {
    // Random delay.
    // Just to be on a safe side delay between 0 and 50ms (I2C EVENT_TIMEOUT / 2)
    delayMicroseconds(random_range(0, 50000));
    memset(I2C_Test_Tx_Buffer, 0, sizeof(I2C_Test_Tx_Buffer));
    memcpy(I2C_Test_Tx_Buffer, SLAVE_TEST_MESSAGE, requestedLength);

    // Serial.print("> ");
    // Serial.println((const char *)I2C_Test_Tx_Buffer);

    USE_WIRE.write((const uint8_t*)I2C_Test_Tx_Buffer, requestedLength);
    requested = false;
}

void I2C_Slave_On_Receive_Callback(int) {
    if (!requested) {
        requested = true;
        requestedLength = 0;
    }
    assertEqual(USE_WIRE.available(), TRANSFER_LENGTH_1);
    int count = 0;
    while(USE_WIRE.available()) {
        I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
    }
    I2C_Test_Rx_Buffer[count] = 0x00;

    // Serial.print("< ");
    // Serial.println((const char *)I2C_Test_Rx_Buffer);

    assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE)) == 0);

    requestedLength += *((uint32_t *)(I2C_Test_Rx_Buffer + sizeof(MASTER_TEST_MESSAGE)));
    assertTrue((requestedLength >= 0 && requestedLength <= TRANSFER_LENGTH_2));

    if (requestedLength == 0)
        done = 1;
}

test(I2C_01_Master_Slave_Slave_Transfer)
{
    Serial.println("This is Slave");
    Serial.printlnf("Master message: %s", MASTER_TEST_MESSAGE);
    Serial.printlnf("Slave message: %s", SLAVE_TEST_MESSAGE);
    Serial.blockOnOverrun(false);
    USE_WIRE.begin(I2C_ADDRESS);
    USE_WIRE.stretchClock(true);
    USE_WIRE.onRequest(I2C_Slave_On_Request_Callback);
    USE_WIRE.onReceive(I2C_Slave_On_Receive_Callback);

    // Dummy call to initialize the seed
    (void)random_range(0, 1);

    while(done == 0) {
        delay(100);
    }

    USE_WIRE.end();
}
