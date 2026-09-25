#include "application.h"
#include "unit-test/unit-test.h"
#include "../common/common.inc"

namespace {

enum class StallMode {
    OFF = 0,
    STATUS = 1
};

constexpr uint32_t STALL_TOTAL_BYTES = (STALL_TRANSFERS + 1) * STALL_TRANSFER_SIZE;

// The I2C_06 state, maintained by the wire callbacks. The slave interrupt may be
// delayed across the stop of one transaction and the start of the next, and the bytes
// of the two transactions then arrive in one report. The received bytes are checked
// against the expected stream, one by one.
struct StallState {
    volatile uint32_t lastRxMs;
    volatile bool stop;
    bool ready;
    uint32_t goodBytes;
    uint32_t badBytes;
    uint32_t onReceiveCount;
    uint32_t onRequestCount;
    uint8_t lastBadByte;
    uint32_t lastBadPos;
    StallMode mode;
} stall;

// The master sends STALL_TRANSFERS numbered frames, then the stop frame.
static uint8_t stallStreamByte(size_t pos) {
    const size_t frame = pos / STALL_TRANSFER_SIZE;
    const size_t i = pos % STALL_TRANSFER_SIZE;
    if (i == 0) {
        return STALL_FRAME_SEED;
    }
    const uint32_t seq = (frame < STALL_TRANSFERS) ? (frame + 1) : STALL_STOP_SEQ;
    if (i == 1) {
        return (uint8_t)(seq >> 8);
    }
    if (i == 2) {
        return (uint8_t)seq;
    }
    return STALL_FRAME_PADDING;
}

} // anonymous namespace

static uint8_t done = 0;
static uint32_t requestedLength = 0;
static bool requested = false;

static int random_range(int minVal, int maxVal)
{
    static unsigned int seed = HAL_RNG_GetRandomNumber();
    return rand_r(&seed) % (maxVal - minVal + 1) + minVal;
}

static void handleStallBytes(int byteCount) {
    stall.lastRxMs = millis();
    stall.mode = StallMode::STATUS;
    const int i = byteCount;
    if (!stall.ready) {
        while (USE_WIRE.available()) {
            USE_WIRE.read();
        }
        return;
    }
    if (i <= 0 || stall.goodBytes >= STALL_TOTAL_BYTES) {
        if (i > 0) {
            stall.lastBadByte = USE_WIRE.peek();
            stall.lastBadPos = stall.goodBytes;
        }
        stall.badBytes += i > 0 ? i : 0;
        return;
    }
    stall.onReceiveCount++;
    for (int j = 0; j < i; j++) {
        const uint8_t data = (uint8_t)USE_WIRE.read();
        if (data == stallStreamByte(stall.goodBytes)) {
            stall.goodBytes++;
        } else {
            stall.lastBadByte = data;
            stall.lastBadPos = stall.goodBytes;
            stall.badBytes++;
        }
    }
    if (stall.goodBytes == STALL_TOTAL_BYTES) {
        stall.stop = true;
    }
}

static void writeStallStatus() {
    char status[STALL_TRANSFER_SIZE + 1];
    int n = snprintf(status, sizeof(status), "%lu,%lu,%lu,%lu,%lu,0x%lx",
            (unsigned long)stall.goodBytes,
            (unsigned long)stall.badBytes,
            (unsigned long)stall.onReceiveCount,
            (unsigned long)stall.onRequestCount,
            (unsigned long)stall.lastBadPos,
            (unsigned long)stall.lastBadByte);
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
        stall.ready = true;
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
    if (stall.mode == StallMode::STATUS) {
        handleStallBytes(byteCount);
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
    stall.mode = StallMode::STATUS;
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