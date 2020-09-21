#include "application.h"
#include "unit-test/unit-test.h"
#include "../common/common.inc"

static void I2C_Master_Configure()
{
    USE_WIRE.setSpeed(400000);
    USE_WIRE.begin();
}

test(I2C_01_Master_Slave_Master_Variable_Length_Transfer)
{
    Serial.println("This is Master");
    Serial.printlnf("Master message: %s", MASTER_TEST_MESSAGE);
    Serial.printlnf("Slave message: %s", SLAVE_TEST_MESSAGE);
    uint32_t requestedLength = TEST_I2C_BUFFER_SIZE;
    I2C_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0)
        {
            /* Zero-length request instructs Slave to finish running its own test. */
            break;
        }
        memset(I2C_Test_Tx_Buffer, 0, sizeof(I2C_Test_Tx_Buffer));
        memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));

        // delay(I2C_DELAY);

        memcpy(I2C_Test_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(I2C_Test_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        USE_WIRE.beginTransmission(I2C_ADDRESS);
        USE_WIRE.write(I2C_Test_Tx_Buffer, TRANSFER_LENGTH_1);
        
        // Sleep API should keep the buffer state as-is
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, NULL), (int)SYSTEM_ERROR_NONE);
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, NULL), (int)SYSTEM_ERROR_NONE);

        // End with STOP
        assertEqual(USE_WIRE.endTransmission(true), 0);
        // delay(I2C_DELAY);

        // Serial.print("> ");
        // Serial.println((const char *)I2C_Test_Tx_Buffer);

        if (requestedLength == 0)
            break;

        // Now read out requestedLength bytes
        memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));
        USE_WIRE.requestFrom(I2C_ADDRESS, requestedLength);
        assertEqual(requestedLength, USE_WIRE.available());

        // Sleep API should keep the buffer state as-is
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, NULL), (int)SYSTEM_ERROR_NONE);
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, NULL), (int)SYSTEM_ERROR_NONE);

        uint32_t count = 0;
        while(USE_WIRE.available()) {
            I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
        }
        // Serial.print("< ");
        // Serial.println((const char *)I2C_Test_Rx_Buffer);
        assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, SLAVE_TEST_MESSAGE, requestedLength) == 0);

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, NULL), (int)SYSTEM_ERROR_NONE);
        
        // Exit sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, NULL), (int)SYSTEM_ERROR_NONE);
    }

    USE_WIRE.end();
}

test(I2C_02_Master_Slave_Master_Variable_Length_Transfer_Slave_Tx_Buffer_Underflow)
{
    /* This test requests the slave to prepare N bytes for transmission, but the actual
     * number of bytes clocked by the Master is N + 1. This will cause tx buffer underrun on the Slave.
     * When clock stretching is enabled, the I2C peripheral in Slave mode by default will continue pulling SCL low,
     * until the data is loaded into the DR register. This might cause the bus to enter an unrecoverable state.
     */
    uint32_t requestedLength = TEST_I2C_BUFFER_SIZE - 1;
    I2C_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0)
        {
            /* Zero-length request instructs Slave to finish running its own test. */
            break;
        }
        memset(I2C_Test_Tx_Buffer, 0, sizeof(I2C_Test_Tx_Buffer));
        memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));

        // delay(I2C_DELAY);

        memcpy(I2C_Test_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(I2C_Test_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        USE_WIRE.beginTransmission(I2C_ADDRESS);
        USE_WIRE.write(I2C_Test_Tx_Buffer, TRANSFER_LENGTH_1);
        
        // End with STOP
        assertEqual(USE_WIRE.endTransmission(true), 0);
        // delay(I2C_DELAY);

        if (requestedLength == 0)
            break;

        // Now read out requestedLength bytes a couple of times
        int count = random(4, 20);
        while (count--) {
            memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));
            USE_WIRE.requestFrom(I2C_ADDRESS, requestedLength + (requestedLength & 0x01));
            if (USE_WIRE.available() != 0) {
                assertEqual(requestedLength + (requestedLength & 0x01), USE_WIRE.available());

                uint32_t count = 0;
                while(USE_WIRE.available()) {
                    I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
                }
                // Serial.print("< ");
                // Serial.println((const char *)I2C_Test_Rx_Buffer);
                assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, SLAVE_TEST_MESSAGE, requestedLength) == 0);
            } else if (requestedLength & 0x01) {
                Serial.println("Error reading from Slave, checking if we can recover");
            } else {
                Serial.println("Failed to recover");
                assertTrue(false);
            }
        }

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, NULL), (int)SYSTEM_ERROR_NONE);

        // Exit sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, NULL), (int)SYSTEM_ERROR_NONE);
    }

    USE_WIRE.end();
}

test(I2C_03_Master_Slave_Master_WireTransmission_And_Short_Timeout)
{
    uint32_t requestedLength = TEST_I2C_BUFFER_SIZE;
    I2C_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0)
        {
            /* Zero-length request instructs Slave to finish running its own test. */
            break;
        }
        memset(I2C_Test_Tx_Buffer, 0, sizeof(I2C_Test_Tx_Buffer));
        memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));

        memcpy(I2C_Test_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(I2C_Test_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        // NOTE: wrong address
        USE_WIRE.beginTransmission(WireTransmission(I2C_ADDRESS + 1).timeout(1ms));
        USE_WIRE.write(I2C_Test_Tx_Buffer, TRANSFER_LENGTH_1);

        // End with STOP (set in beginTransmission())
        auto t1 = millis();
        assertNotEqual(USE_WIRE.endTransmission(), 0);
        auto t2 = millis();
        assertLessOrEqual(t2 - t1, 55);

        if (requestedLength == 0)
            break;

        // Now read out requestedLength bytes
        memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));
        t1 = millis();
        // NOTE: wrong address
        USE_WIRE.requestFrom(WireTransmission(I2C_ADDRESS + 1).quantity(requestedLength).timeout(1ms));
        t2 = millis();
        assertLessOrEqual(t2 - t1, 55);
        assertNotEqual(requestedLength, USE_WIRE.available());

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, NULL), (int)SYSTEM_ERROR_NONE);

        // Exit sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, NULL), (int)SYSTEM_ERROR_NONE);
    }

    USE_WIRE.end();
}


test(I2C_04_Master_Slave_Master_Variable_Length_Transfer_With_WireTransmission_And_Standard_Timeout)
{
    uint32_t requestedLength = TEST_I2C_BUFFER_SIZE;
    I2C_Master_Configure();

    while (requestedLength >= 0)
    {
        if (requestedLength == 0)
        {
            /* Zero-length request instructs Slave to finish running its own test. */
            break;
        }
        memset(I2C_Test_Tx_Buffer, 0, sizeof(I2C_Test_Tx_Buffer));
        memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));

        // delay(I2C_DELAY);

        memcpy(I2C_Test_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(I2C_Test_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength, sizeof(uint32_t));

        USE_WIRE.beginTransmission(WireTransmission(I2C_ADDRESS).timeout(100ms));
        USE_WIRE.write(I2C_Test_Tx_Buffer, TRANSFER_LENGTH_1);

        // End with STOP (set in beginTransmission())
        assertEqual(USE_WIRE.endTransmission(), 0);
        // delay(I2C_DELAY);

        // Serial.print("> ");
        // Serial.println((const char *)I2C_Test_Tx_Buffer);

        if (requestedLength == 0)
            break;

        // Now read out requestedLength bytes
        memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));
        USE_WIRE.requestFrom(WireTransmission(I2C_ADDRESS).quantity(requestedLength).timeout(100ms));
        assertEqual(requestedLength, USE_WIRE.available());

        uint32_t count = 0;
        while(USE_WIRE.available()) {
            I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
        }
        // Serial.print("< ");
        // Serial.println((const char *)I2C_Test_Rx_Buffer);
        assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, SLAVE_TEST_MESSAGE, requestedLength) == 0);

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, NULL), (int)SYSTEM_ERROR_NONE);

        // Exit sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, NULL), (int)SYSTEM_ERROR_NONE);
    }

    USE_WIRE.end();
}

test(I2C_05_Master_Slave_Master_Variable_Length_Restarted_Transfer)
{
    uint32_t requestedLength = TEST_I2C_BUFFER_SIZE;
    I2C_Master_Configure();

    while (requestedLength >= 0)
    {
        // if (requestedLength == 0)
        // {
        //     /* Zero-length request instructs Slave to finish running its own test. */
        //     break;
        // }

        // delay(I2C_DELAY);
        /* The first requested length cannot be 0, otherwise, it instructs the Slave to finish running its own test. */
        memset(I2C_Test_Tx_Buffer, 0, sizeof(I2C_Test_Tx_Buffer));
        uint32_t requestedLength1 = (requestedLength / 10 > 0) ? (requestedLength - requestedLength % 10) : (requestedLength % 10);
        memcpy(I2C_Test_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(I2C_Test_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength1, sizeof(uint32_t));

        USE_WIRE.beginTransmission(I2C_ADDRESS);
        USE_WIRE.write(I2C_Test_Tx_Buffer, TRANSFER_LENGTH_1);

        // End without STOP
        assertEqual(USE_WIRE.endTransmission(false), 0);
        // delay(I2C_DELAY);

        // Serial.print("> ");
        // Serial.println((const char *)I2C_Test_Tx_Buffer);

        memset(I2C_Test_Tx_Buffer, 0, sizeof(I2C_Test_Tx_Buffer));
        uint32_t requestedLength2 = requestedLength - requestedLength1;
        memcpy(I2C_Test_Tx_Buffer, MASTER_TEST_MESSAGE, sizeof(MASTER_TEST_MESSAGE));
        memcpy(I2C_Test_Tx_Buffer + sizeof(MASTER_TEST_MESSAGE), (void*)&requestedLength2, sizeof(uint32_t));

        USE_WIRE.beginTransmission(I2C_ADDRESS);
        USE_WIRE.write(I2C_Test_Tx_Buffer, TRANSFER_LENGTH_1);

        // End without STOP
        assertEqual(USE_WIRE.endTransmission(false), 0);
        // delay(I2C_DELAY);

        // Serial.print("> ");
        // Serial.println((const char *)I2C_Test_Tx_Buffer);

        if (requestedLength == 0)
            break;

        // Now read out requestedLength bytes
        memset(I2C_Test_Rx_Buffer, 0, sizeof(I2C_Test_Rx_Buffer));
        USE_WIRE.requestFrom(I2C_ADDRESS, requestedLength);
        assertEqual(requestedLength, USE_WIRE.available());

        uint32_t count = 0;
        while(USE_WIRE.available()) {
            I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
        }
        // Serial.print("< ");
        // Serial.println((const char *)I2C_Test_Rx_Buffer);
        assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, SLAVE_TEST_MESSAGE, requestedLength) == 0);

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, true, NULL), (int)SYSTEM_ERROR_NONE);
        
        // Exit sleep
        assertEqual(hal_i2c_sleep(HAL_I2C_INTERFACE1, false, NULL), (int)SYSTEM_ERROR_NONE);
    }

    USE_WIRE.end();
}
