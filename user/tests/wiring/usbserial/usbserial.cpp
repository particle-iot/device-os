#include "application.h"
#include "unit-test/unit-test.h"
#include "usb_settings.h"
#include "ringbuf_helper.h"

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
}

test(USBSERIAL_02_ReadWrite) {
    //The following code will test all the important USB Serial routines
    char test[] = "hello";
    char message[10];
    // when
    consume(Serial);
    Serial.print("Type the following message and press Enter: ");
    Serial.println(test);
    serialReadLine(&Serial, message, 9, 10000);//10 sec timeout
    Serial.println("");
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(USBSERIAL_03_isConnectedWorksCorrectly) {
    Serial.println("Please close the USB Serial port and open it again within 60 seconds");

    uint32_t mil = millis();
    // Wait for 60 seconds maximum for the host to close the USB Serial port
    while(Serial.isConnected()) {
        assertTrue((millis() - mil) < 60000);
    }
    // Wait for 60 seconds maximum for the host to open the USB Serial port again
    while(!Serial.isConnected()) {
        assertTrue((millis() - mil) < 60000);
    }

    delay(10);
    Serial.println("Glad too see you back!");

    char test[] = "hello";
    char message[10];
    // when
    consume(Serial);
    Serial.print("Type the following message and press Enter: ");
    Serial.println(test);
    serialReadLine(&Serial, message, 9, 10000);//10 sec timeout
    Serial.println("");
    // then
    assertTrue(strncmp(test, message, 5)==0);
}

test(USBSERIAL_04_RxBufferFillsCompletely) {
    Serial.println("We will now test USB Serial RX buffer");
    Serial.println("Please close the USB Serial port and open it again once the device reattaches");

    uint32_t mil = millis();
    // Wait for 60 seconds maximum for the host to close the USB Serial port
    while(Serial.isConnected()) {
        assertTrue((millis() - mil) < 60000);
    }

    Serial.end();
    assertEqual(Serial.isEnabled(), false);
    Serial.begin();
    assertEqual(Serial.isEnabled(), true);

    // Wait for 60 seconds maximum for the host to open the USB Serial port again
    while(!Serial.isConnected()) {
        assertTrue((millis() - mil) < 60000);
    }

    assertEqual(Serial.available(), 0);

    delay(10);
    Serial.println("Glad too see you back!");

    for (int attmpt = 0; attmpt < 3; attmpt++) {
        char randStr[USB_RX_BUFFER_SIZE + 2];
        char rStr[USB_RX_BUFFER_SIZE + 2];
        memset(randStr, 0, USB_RX_BUFFER_SIZE + 2);
        memset(rStr, 0, USB_RX_BUFFER_SIZE + 2);
        srand(millis());
        randomString(randStr, USB_RX_BUFFER_SIZE - 1);

        Serial.println("Please copy and paste the following string:");
        Serial.println(randStr);

        int32_t avail = 0;
        mil = millis();
        uint32_t milsame = 0;
        while(Serial.available() != (USB_RX_BUFFER_SIZE - 1)) {
            if (avail == Serial.available() && avail != 0) {
                if (milsame == 0) {
                    milsame = millis();
                } else {
                    if ((millis() - milsame) >= 5000) {
                        // Depending on the host driver, we might have received (USB_RX_BUFFER_SIZE - 64)
                        break;
                    }
                }
            } else {
                avail = Serial.available();
                milsame = 0;
            }
            assertTrue((millis() - mil) < 120000);
        }
        avail = Serial.available();
        Serial.printf("OK. Read back %d bytes\r\n", avail);

        assertTrue((avail == (USB_RX_BUFFER_SIZE - 1)) ||
                   (avail >= ((USB_RX_BUFFER_SIZE - 1) / 64 * 64)));
        for (int i = 0; i < avail; i++) {
            rStr[i] = Serial.read();
        }

        Serial.printf("Data: %s\r\n\r\n", rStr);

        assertTrue(!strncmp(randStr, rStr, avail));

        delay(500);
        while(Serial.available()) {
            (void)Serial.read();
        }

        if (!((avail == (USB_RX_BUFFER_SIZE - 1)) || (avail == ((USB_RX_BUFFER_SIZE - 1) / 64 * 64)))) {
            // Continue only if we received data in 64-byte blocks
            break;
        }
    }
}