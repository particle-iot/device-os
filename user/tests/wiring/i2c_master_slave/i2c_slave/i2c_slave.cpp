#include "application.h"
#include "unit-test/unit-test.h"
#include "../common/common.inc"

namespace {

enum class StallMode {
    OFF = 0,
    STATUS = 1
};

// The I2C_06 state, maintained by the wire callbacks
struct StallState {
    volatile uint32_t lastRxMs;
    volatile bool stop;
    uint32_t onReceiveCount;
    uint32_t lastGoodSeq;
    uint32_t corruptCount;
    uint32_t shortCount;
    uint32_t onRequestCount;
    StallMode mode;
} stall;

} // anonymous namespace

static uint8_t done = 0;
static uint32_t requestedLength = 0;
static bool requested = false;

static int random_range(int minVal, int maxVal)
{
    static unsigned int seed = HAL_RNG_GetRandomNumber();
    return rand_r(&seed) % (maxVal - minVal + 1) + minVal;
}

static void handleStallFrame(int byteCount) {
    static uint8_t buf[STALL_TRANSFER_SIZE + 1];
    size_t i = 0;
    while (USE_WIRE.available()) {
        buf[i++] = (uint8_t)USE_WIRE.read();
    }
    stall.lastRxMs = millis();
    stall.mode = StallMode::STATUS;
    if (i < STALL_HEADER_SIZE || (int)i != byteCount) {
        stall.shortCount++;
        return;
    }
    const uint16_t seq = ((uint16_t)buf[1] << 8) | buf[2];
    if (seq == STALL_STOP_SEQ) {
        stall.stop = true;
        return;
    }
    if (buf[0] != STALL_FRAME_SEED || i != STALL_TRANSFER_SIZE ||
            seq != (uint16_t)(stall.lastGoodSeq + 1)) {
        stall.corruptCount++;
        return;
    }
    for (size_t j = STALL_HEADER_SIZE; j < i; j++) {
        if (buf[j] != STALL_FRAME_PADDING) {
            stall.corruptCount++;
            return;
        }
    }
    stall.lastGoodSeq = seq;
    stall.onReceiveCount++;
}

static void writeStallStatus() {
    char status[STALL_TRANSFER_SIZE + 1];
    int n = snprintf(status, sizeof(status), "%lu,%lu,%lu,%lu,%lu",
            (unsigned long)stall.onReceiveCount,
            (unsigned long)stall.lastGoodSeq,
            (unsigned long)stall.corruptCount,
            (unsigned long)stall.shortCount,
            (unsigned long)stall.onRequestCount);
    if (n < 0) {
        n = 0;
    }
    if (n > STALL_TRANSFER_SIZE) {
        n = STALL_TRANSFER_SIZE;
    }
    while (n < STALL_TRANSFER_SIZE) {
        status[n++] = ' ';
    }
    USE_WIRE.write((const uint8_t*)status, STALL_TRANSFER_SIZE);
}

void I2C_Slave_On_Request_Callback(void) {
    if (stall.mode == StallMode::STATUS) {
        stall.onRequestCount++;
        stall.lastRxMs = millis();
        writeStallStatus();
        return;
    }
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

void I2C_Slave_On_Receive_Callback(int byteCount) {
    if (byteCount > 0 && USE_WIRE.peek() == STALL_FRAME_SEED) {
        handleStallFrame(byteCount);
        return;
    }
    if (!requested) {
        requested = true;
        requestedLength = 0;
    }
    uint32_t i2cAvailable = USE_WIRE.available();
    assertEqual(i2cAvailable, TRANSFER_LENGTH_1);
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
    stall.mode = StallMode::OFF;
}

test(I2C_000_Prepare)
{
    // Serial.println("This is Slave");
    // Serial.printlnf("Master message: %s", MASTER_TEST_MESSAGE);
    // Serial.printlnf("Slave message: %s", SLAVE_TEST_MESSAGE);
    // Serial.blockOnOverrun(false);
    USE_WIRE.begin(I2C_ADDRESS);
    USE_WIRE.stretchClock(true);
    USE_WIRE.onRequest(I2C_Slave_On_Request_Callback);
    USE_WIRE.onReceive(I2C_Slave_On_Receive_Callback);

    // Dummy call to initialize the seed
    (void)random_range(0, 1);

    // while(done == 0) {
    //     delay(100);
    // }
}

test(I2C_01_Master_Slave_Master_Variable_Length_Transfer)
{

}

test(I2C_02_Master_Slave_Master_Variable_Length_Transfer_Slave_Tx_Buffer_Underflow)
{

}

test(I2C_03_Master_Slave_Master_WireTransmission_And_Short_Timeout)
{

}

test(I2C_04_Master_Slave_Master_Variable_Length_Transfer_With_WireTransmission_And_Standard_Timeout)
{

}

test(I2C_05_Master_Slave_Master_Variable_Length_Restarted_Transfer)
{

}

test(I2C_06_Master_Slave_Slave_Survives_Masked_Interrupt_Storm)
{
    uint32_t startMs = millis();
    stall.lastRxMs = startMs;
    while (!stall.stop) {
        if ((millis() - stall.lastRxMs) > STALL_TRAFFIC_LIMIT_MS) {
            break;
        }
        if ((millis() - startMs) > STALL_LOOP_LIMIT_MS) {
            break;
        }
        ATOMIC_BLOCK() {
            stallDelayUs(STALL_MASK_US);
        }
    }
}

test(I2C_ZZZ_Cleanup)
{
    USE_WIRE.end();
}