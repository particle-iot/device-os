#include "application.h"
#include "unit-test/unit-test.h"
#include "usb_settings.h"
#include "ringbuf_helper.h"

static constexpr size_t TX_STRESS_CHUNK_SIZE = USB_TX_BUFFER_SIZE * 16;
static constexpr unsigned TX_NONBLOCKING_STRESS_ATTEMPTS = 2048;
static constexpr unsigned TX_BLOCKING_STRESS_ITERATIONS = 256;
static constexpr unsigned TX_CHAR_NONBLOCKING_STRESS_ATTEMPTS = 4096;
static constexpr unsigned TX_CHAR_BLOCKING_STRESS_ITERATIONS = 64 * 1024;
static constexpr size_t RX_STRESS_CHUNK_SIZE = 64;
static constexpr unsigned RX_STRESS_ITERATIONS = 256;
static uint8_t bulkWriteBuffer[TX_STRESS_CHUNK_SIZE] = {};
static constexpr size_t RX_TEST_DATA_SIZE = USB_RX_BUFFER_SIZE - 1;

int randomString(char *buf, int len) {
    for (int i = 0; i < len; i++) {
        uint8_t d = random(0, 15);
        char c = d + 48;
        if (57 < c)
            c += 7;
        buf[i] = c;
    }

    return len;
}

void consume(Stream& serial)
{
    while (serial.available() > 0) {
        (void)serial.read();
    }
}

bool waitForEmptyTxBuffer() {
    return waitFor([] {
        return Serial.availableForWrite() == USB_TX_BUFFER_SIZE;
    }, 5000);
}

test(USBSERIAL_00_RingBufferHelperIsSane) {
    uint32_t size = 129;
    uint32_t head = 0;
    uint32_t tail = 0;

    head = 0;
    tail = 0;
    assertEqual(0, ring_data_avail(size, head, tail));
    assertEqual(128, ring_space_avail(size, head, tail));
    assertEqual(0, ring_data_contig(size, head, tail));
    assertEqual(128, ring_space_contig(size, head, tail));
    assertEqual(0, ring_space_wrapped(size, head, tail));

    head = 63;
    tail = 0;
    assertEqual(63, ring_data_avail(size, head, tail));
    assertEqual(65, ring_space_avail(size, head, tail));
    assertEqual(63, ring_data_contig(size, head, tail));
    assertEqual(65, ring_space_contig(size, head, tail));
    assertEqual(0, ring_space_wrapped(size, head, tail));

    head = 63;
    tail = 32;
    assertEqual(31, ring_data_avail(size, head, tail));
    assertEqual(97, ring_space_avail(size, head, tail));
    assertEqual(31, ring_data_contig(size, head, tail));
    assertEqual(66, ring_space_contig(size, head, tail));
    assertEqual(31, ring_space_wrapped(size, head, tail));

    head = 63;
    tail = 63;
    assertEqual(0, ring_data_avail(size, head, tail));
    assertEqual(128, ring_space_avail(size, head, tail));
    assertEqual(0, ring_data_contig(size, head, tail));
    assertEqual(66, ring_space_contig(size, head, tail));
    assertEqual(62, ring_space_wrapped(size, head, tail));

    head = 128;
    tail = 63;
    assertEqual(65, ring_data_avail(size, head, tail));
    assertEqual(63, ring_space_avail(size, head, tail));
    assertEqual(65, ring_data_contig(size, head, tail));
    assertEqual(1, ring_space_contig(size, head, tail));
    assertEqual(62, ring_space_wrapped(size, head, tail));

    head = 128;
    tail = 128;
    assertEqual(0, ring_data_avail(size, head, tail));
    assertEqual(128, ring_space_avail(size, head, tail));
    assertEqual(0, ring_data_contig(size, head, tail));
    assertEqual(1, ring_space_contig(size, head, tail));
    assertEqual(127, ring_space_wrapped(size, head, tail));

    head = 0;
    tail = 63;
    assertEqual(66, ring_data_avail(size, head, tail));
    assertEqual(62, ring_space_avail(size, head, tail));
    assertEqual(66, ring_data_contig(size, head, tail));
    assertEqual(62, ring_space_contig(size, head, tail));
    assertEqual(0, ring_space_wrapped(size, head, tail));

    head = 32;
    tail = 63;
    assertEqual(98, ring_data_avail(size, head, tail));
    assertEqual(30, ring_space_avail(size, head, tail));
    assertEqual(66, ring_data_contig(size, head, tail));
    assertEqual(30, ring_space_contig(size, head, tail));
    assertEqual(0, ring_space_wrapped(size, head, tail));

    head = 0;
    tail = 128;
    assertEqual(1, ring_data_avail(size, head, tail));
    assertEqual(127, ring_space_avail(size, head, tail));
    assertEqual(1, ring_data_contig(size, head, tail));
    assertEqual(127, ring_space_contig(size, head, tail));
    assertEqual(0, ring_space_wrapped(size, head, tail));

    head = 128;
    tail = 0;
    assertEqual(128, ring_data_avail(size, head, tail));
    assertEqual(0, ring_space_avail(size, head, tail));
    assertEqual(128, ring_data_contig(size, head, tail));
    assertEqual(0, ring_space_contig(size, head, tail));
    assertEqual(0, ring_space_wrapped(size, head, tail));
}

test(USBSERIAL_01_SerialDoesNotDeadlockWhenInterruptsAreMasked) {
    int32_t state = HAL_disable_irq();
    // Write 2048 + \r\n bytes into Serial TX buffer
    for (int i = 0; i < 2048; i++) {
        char tmp;
        randomString(&tmp, 1);
        Serial.write(tmp);
    }
    HAL_enable_irq(state);
    Serial.println();
    Serial.flush();
}

test(USBSERIAL_02_ReadWrite) {
    consume(Serial);
    constexpr char data[] = "device-to-host";
    assertEqual(Serial.write((const uint8_t*)data, sizeof(data) - 1), sizeof(data) - 1);
    Serial.flush();
}

test(USBSERIAL_03_ReadWriteVerifiesHostData) {
    constexpr char expected[] = "host-to-device";
    char data[sizeof(expected) - 1] = {};
    assertTrue(waitFor([] {
        return Serial.available() >= (int)(sizeof(expected) - 1);
    }, 5000));
    assertEqual(Serial.readBytes(data, sizeof(data)), sizeof(data));
    assertTrue(!memcmp(data, expected, sizeof(data)));
}

test(USBSERIAL_04_isConnectedInitially) {
    assertTrue(Serial.isConnected());
}

test(USBSERIAL_05_ClosedPortWritesFailWithoutBlocking) {
    assertTrue(waitForNot(Serial.isConnected, 5000));

    uint8_t data[16] = {};
    assertEqual(Serial.availableForWrite(), 0);
    assertEqual(Serial.available(), 0);
    assertEqual(Serial.peek(), -1);
    assertEqual(Serial.read(), -1);

    const auto start = millis();
    Serial.blockOnOverrun(false);
    assertEqual(Serial.write((uint8_t)'a'), 0);
    assertEqual(Serial.write(data, sizeof(data)), 0);
    Serial.blockOnOverrun(true);
    assertEqual(Serial.write((uint8_t)'b'), 0);
    assertEqual(Serial.write(data, sizeof(data)), 0);
    Serial.flush();
    assertTrue(millis() - start < 1000);
}

test(USBSERIAL_06_isConnectedDetectsReopenedPort) {
    assertTrue(waitFor(Serial.isConnected, 5000));
}

test(USBSERIAL_07_EndBeginWhilePortIsClosed) {
    assertTrue(waitForNot(Serial.isConnected, 5000));

    // Serial.end() re-enumerates USB, which temporarily removes the control
    // interface used by the test runner. RESET_PENDING is also used by sleep
    // tests as the generic notification for an expected USB detach.
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    Serial.end();
    assertEqual(Serial.isEnabled(), false);
    Serial.begin();
    assertEqual(Serial.isEnabled(), true);
}

test(USBSERIAL_08_RxBufferSetup) {
    assertTrue(waitFor(Serial.isConnected, 30000));
    consume(Serial);
    assertEqual(Serial.available(), 0);

    char size[16] = {};
    snprintf(size, sizeof(size), "%u", (unsigned)RX_TEST_DATA_SIZE);
    assertEqual(0, pushMailboxMsg(size, 10000));
}

void verifyRxBuffer(unsigned iteration) {
    assertTrue(waitFor([] {
        return Serial.available() >= (int)RX_TEST_DATA_SIZE;
    }, 10000));

    uint8_t data[RX_TEST_DATA_SIZE] = {};
    assertEqual(Serial.readBytes((char*)data, sizeof(data)), sizeof(data));
    for (size_t i = 0; i < sizeof(data); ++i) {
        const uint8_t expected = "0123456789ABCDEF"[(i + iteration) % 16];
        assertEqual(data[i], expected);
    }
    assertEqual(Serial.available(), 0);
}

test(USBSERIAL_09_RxBufferFillsCompletelyFirst) {
    verifyRxBuffer(0);
}

test(USBSERIAL_10_RxBufferFillsCompletelySecond) {
    verifyRxBuffer(1);
}

test(USBSERIAL_11_RxBufferFillsCompletelyThird) {
    verifyRxBuffer(2);
}

test(USBSERIAL_12_NonBlockingWriteStressSetup) {
    assertTrue(waitFor(Serial.isConnected, 30000));
    assertTrue(waitForEmptyTxBuffer());
}

test(USBSERIAL_13_NonBlockingWriteStressHandlesBackpressure) {
    assertTrue(Serial.isConnected());
    assertTrue(waitForEmptyTxBuffer());

    Serial.blockOnOverrun(false);
    size_t totalWritten = 0;
    unsigned partialWrites = 0;
    unsigned zeroWrites = 0;
    bool invalidWriteCount = false;
    for (unsigned i = 0; i < TX_NONBLOCKING_STRESS_ATTEMPTS; ++i) {
        const size_t written = Serial.write(bulkWriteBuffer, sizeof(bulkWriteBuffer));
        if (written > sizeof(bulkWriteBuffer)) {
            invalidWriteCount = true;
            break;
        }
        totalWritten += written;
        if (written < sizeof(bulkWriteBuffer)) {
            ++partialWrites;
        }
        if (written == 0) {
            ++zeroWrites;
        }
    }
    Serial.blockOnOverrun(true);

    assertFalse(invalidWriteCount);
    assertTrue(totalWritten > 0);
    assertTrue(partialWrites > 0);
    assertTrue(zeroWrites > 0);
}

test(USBSERIAL_14_BlockingWriteStressWritesEveryBuffer) {
    assertTrue(waitFor(Serial.isConnected, 30000));
    assertTrue(waitForEmptyTxBuffer());

    Serial.blockOnOverrun(true);
    for (unsigned i = 0; i < TX_BLOCKING_STRESS_ITERATIONS; ++i) {
        const size_t written = Serial.write(bulkWriteBuffer, sizeof(bulkWriteBuffer));
        assertEqual(written, sizeof(bulkWriteBuffer));
    }
    Serial.flush();
}

test(USBSERIAL_15_NonBlockingCharacterWriteStressSetup) {
    assertTrue(waitFor(Serial.isConnected, 30000));
    assertTrue(waitForEmptyTxBuffer());
}

test(USBSERIAL_16_NonBlockingCharacterWriteStressHandlesBackpressure) {
    assertTrue(Serial.isConnected());
    assertTrue(waitForEmptyTxBuffer());

    Serial.blockOnOverrun(false);
    bool backpressureReached = false;
    for (unsigned i = 0; i < TX_NONBLOCKING_STRESS_ATTEMPTS; ++i) {
        if (Serial.write(bulkWriteBuffer, sizeof(bulkWriteBuffer)) == 0) {
            backpressureReached = true;
            break;
        }
    }

    unsigned zeroWrites = 0;
    bool invalidWriteCount = false;
    for (unsigned i = 0; i < TX_CHAR_NONBLOCKING_STRESS_ATTEMPTS; ++i) {
        const size_t written = Serial.write((uint8_t)i);
        if (written > 1) {
            invalidWriteCount = true;
            break;
        }
        if (written == 0) {
            ++zeroWrites;
        }
    }
    Serial.blockOnOverrun(true);

    assertTrue(backpressureReached);
    assertFalse(invalidWriteCount);
    assertTrue(zeroWrites > 0);
}

test(USBSERIAL_17_BlockingCharacterWriteStressWritesEveryByte) {
    assertTrue(waitFor(Serial.isConnected, 30000));
    assertTrue(waitForEmptyTxBuffer());

    Serial.blockOnOverrun(true);
    for (unsigned i = 0; i < TX_CHAR_BLOCKING_STRESS_ITERATIONS; ++i) {
        assertEqual(Serial.write((uint8_t)i), 1);
    }
    Serial.flush();
}

test(USBSERIAL_18_DeviceReceiveStressSetup) {
    assertTrue(waitFor(Serial.isConnected, 30000));
    consume(Serial);
}

test(USBSERIAL_19_DeviceReceiveStress) {
    assertTrue(Serial.isConnected());

    uint8_t data[RX_STRESS_CHUNK_SIZE] = {};
    for (unsigned iteration = 0; iteration < RX_STRESS_ITERATIONS; ++iteration) {
        assertTrue(waitFor([] {
            return Serial.available() >= (int)RX_STRESS_CHUNK_SIZE;
        }, 10000));

        if ((iteration % 2) == 0) {
            // Exercise the legacy single-byte receive and peek paths.
            for (size_t i = 0; i < RX_STRESS_CHUNK_SIZE; ++i) {
                const int expected = "0123456789ABCDEF"[(i + iteration) % 16];
                assertEqual(Serial.peek(), expected);
                assertEqual(Serial.read(), expected);
            }
        } else {
            // Exercise the buffer receive and buffer peek paths.
            const int peeked = Serial.peek((char*)data, sizeof(data));
            assertTrue(peeked > 0);
            assertTrue(peeked <= (int)sizeof(data));
            for (int i = 0; i < peeked; ++i) {
                const uint8_t expected = "0123456789ABCDEF"[(i + iteration) % 16];
                assertEqual(data[i], expected);
            }
            memset(data, 0, sizeof(data));
            assertEqual(Serial.readBytes((char*)data, sizeof(data)), sizeof(data));
            for (size_t i = 0; i < sizeof(data); ++i) {
                const uint8_t expected = "0123456789ABCDEF"[(i + iteration) % 16];
                assertEqual(data[i], expected);
            }
        }
    }

    assertEqual(Serial.available(), 0);
    assertEqual(Serial.peek(), -1);
    assertEqual(Serial.read(), -1);
}
