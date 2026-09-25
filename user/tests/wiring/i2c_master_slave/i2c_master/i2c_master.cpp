#include "application.h"
#include "unit-test/unit-test.h"
#include "../common/common.inc"

static void I2C_Master_Configure()
{
    USE_WIRE.setSpeed(400000);
    USE_WIRE.begin();
}

test(I2C_000_Prepare)
{

}

test(I2C_01_Master_Slave_Master_Variable_Length_Transfer)
{
    // Serial.println("This is Master");
    // Serial.printlnf("Master message: %s", MASTER_TEST_MESSAGE);
    // Serial.printlnf("Slave message: %s", SLAVE_TEST_MESSAGE);
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
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), true, NULL), (int)SYSTEM_ERROR_NONE);
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), false, NULL), (int)SYSTEM_ERROR_NONE);

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
        uint32_t i2cAvailable = USE_WIRE.available();
        assertEqual(requestedLength, i2cAvailable);

        // Sleep API should keep the buffer state as-is
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), true, NULL), (int)SYSTEM_ERROR_NONE);
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), false, NULL), (int)SYSTEM_ERROR_NONE);

        uint32_t count = 0;
        while(USE_WIRE.available()) {
            I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
        }
        // Serial.print("< ");
        // Serial.println((const char *)I2C_Test_Rx_Buffer);
        assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, SLAVE_TEST_MESSAGE, requestedLength) == 0);

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), true, NULL), (int)SYSTEM_ERROR_NONE);

        // Exit sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), false, NULL), (int)SYSTEM_ERROR_NONE);
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
            uint32_t i2cAvailable = USE_WIRE.available();
            if (i2cAvailable != 0) {
                assertEqual(requestedLength + (requestedLength & 0x01), i2cAvailable);

                uint32_t count = 0;
                while(USE_WIRE.available()) {
                    I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
                }
                // Serial.print("< ");
                // Serial.println((const char *)I2C_Test_Rx_Buffer);
                assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, SLAVE_TEST_MESSAGE, requestedLength) == 0);
            } else if (requestedLength & 0x01) {
                // Serial.println("Error reading from Slave, checking if we can recover");
            } else {
                // Serial.println("Failed to recover");
                assertTrue(false);
            }
        }

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), true, NULL), (int)SYSTEM_ERROR_NONE);

        // Exit sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), false, NULL), (int)SYSTEM_ERROR_NONE);
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
        uint32_t i2cAvailable = USE_WIRE.available();
        assertNotEqual(requestedLength, i2cAvailable);

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), true, NULL), (int)SYSTEM_ERROR_NONE);

        // Exit sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), false, NULL), (int)SYSTEM_ERROR_NONE);
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
        uint32_t i2cAvailable = USE_WIRE.available();
        assertEqual(requestedLength, i2cAvailable);

        uint32_t count = 0;
        while(USE_WIRE.available()) {
            I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
        }
        // Serial.print("< ");
        // Serial.println((const char *)I2C_Test_Rx_Buffer);
        assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, SLAVE_TEST_MESSAGE, requestedLength) == 0);

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), true, NULL), (int)SYSTEM_ERROR_NONE);

        // Exit sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), false, NULL), (int)SYSTEM_ERROR_NONE);
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
        uint32_t i2cAvailable = USE_WIRE.available();
        assertEqual(requestedLength, i2cAvailable);

        uint32_t count = 0;
        while(USE_WIRE.available()) {
            I2C_Test_Rx_Buffer[count++] = USE_WIRE.read();
        }
        // Serial.print("< ");
        // Serial.println((const char *)I2C_Test_Rx_Buffer);
        assertTrue(strncmp((const char *)I2C_Test_Rx_Buffer, SLAVE_TEST_MESSAGE, requestedLength) == 0);

        requestedLength--;

        // Enter sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), true, NULL), (int)SYSTEM_ERROR_NONE);

        // Exit sleep
        assertEqual(hal_i2c_sleep(USE_WIRE.interface(), false, NULL), (int)SYSTEM_ERROR_NONE);
    }

    USE_WIRE.end();
}

test(I2C_06_Master_Slave_Slave_Survives_Masked_Interrupt_Storm)
{
    uint8_t transferBuf[STALL_TRANSFER_SIZE];
    uint32_t timeoutCount = 0;
    uint32_t busClearFailCount = 0;
    uint32_t seq = 0;

    I2C_Master_Configure();

    auto runWrite = [&](uint32_t s) -> bool {
        transferBuf[0] = STALL_FRAME_SEED;
        transferBuf[1] = (uint8_t)((s >> 8) & 0xff);
        transferBuf[2] = (uint8_t)(s & 0xff);
        memset(transferBuf + STALL_HEADER_SIZE, STALL_FRAME_PADDING, STALL_TRANSFER_SIZE - STALL_HEADER_SIZE);
        USE_WIRE.beginTransmission(WireTransmission(I2C_ADDRESS).timeout(STALL_TIMEOUT_MS));
        USE_WIRE.write(transferBuf, STALL_TRANSFER_SIZE);
        return USE_WIRE.endTransmission() == 0;
    };

    auto readStatus = [&](uint32_t* onReceiveCount, uint32_t* lastGoodSeq,
            uint32_t* corruptCount, uint32_t* shortCount, uint32_t* onRequestCount) -> bool {
        uint8_t buf[STALL_TRANSFER_SIZE + 1] = {};
        (void)USE_WIRE.requestFrom(WireTransmission(I2C_ADDRESS).quantity(STALL_TRANSFER_SIZE).timeout(STALL_TIMEOUT_MS));
        if (USE_WIRE.available() != STALL_TRANSFER_SIZE) {
            while (USE_WIRE.available()) {
                USE_WIRE.read();
            }
            return false;
        }
        size_t i = 0;
        while (USE_WIRE.available()) {
            buf[i++] = (uint8_t)USE_WIRE.read();
        }
        unsigned long v[5] = {};
        if (sscanf((const char*)buf, "%lu,%lu,%lu,%lu,%lu", &v[0], &v[1], &v[2], &v[3], &v[4]) != 5) {
            return false;
        }
        *onReceiveCount = v[0];
        *lastGoodSeq = v[1];
        *corruptCount = v[2];
        *shortCount = v[3];
        *onRequestCount = v[4];
        return true;
    };

    uint32_t gapUs = 0;
    for (uint32_t n = 0; n < STALL_TRANSFERS; n++) {
        seq++;
        if (!runWrite(seq)) {
            timeoutCount++;
            (void)hal_i2c_reset(USE_WIRE.interface(), 0, nullptr);
            if (!runWrite(seq)) {
                busClearFailCount++;
            }
            continue;
        }
        stallDelayUs(gapUs);
        gapUs = (gapUs + 1) % STALL_MAX_GAP_US;
    }

    assertEqual(timeoutCount, (uint32_t)0);
    assertEqual(busClearFailCount, (uint32_t)0);

    uint32_t onReceiveCount = 0, lastGoodSeq = 0, corruptCount = 0, shortCount = 0, onRequestCount = 0;
    for (uint32_t n = 0; n < STALL_READ_TRANSFERS; n++) {
        assertTrue(readStatus(&onReceiveCount, &lastGoodSeq, &corruptCount, &shortCount, &onRequestCount));
        assertEqual(corruptCount, (uint32_t)0);
        assertEqual(shortCount, (uint32_t)0);
        stallDelayUs(gapUs);
        gapUs = (gapUs + 1) % STALL_MAX_GAP_US;
    }

    seq = STALL_STOP_SEQ;
    assertTrue(runWrite(seq));
    assertTrue(readStatus(&onReceiveCount, &lastGoodSeq, &corruptCount, &shortCount, &onRequestCount));

    assertEqual(lastGoodSeq, (uint32_t)(STALL_TRANSFERS & 0xffff));
    assertEqual(onReceiveCount, STALL_TRANSFERS);
    assertEqual(corruptCount, (uint32_t)0);
    assertEqual(shortCount, (uint32_t)0);
    assertEqual(onRequestCount, (uint32_t)(STALL_READ_TRANSFERS + 1));

    USE_WIRE.end();
}

test(I2C_ZZZ_Cleanup)
{

}
