#include "application.h"
#include "ota_flash_hal.h"
#include "dsakeygen.h"
#include "dct.h"

static int s_loops = 1000;

static uint8_t privkey[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];
static uint8_t pubkey[DCT_DEVICE_PUBLIC_KEY_SIZE];

static const char c_hexmap[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

void printHex(const uint8_t* buf, size_t size) {
    for (int i = 0; i < size; i++) {
        Serial.write(c_hexmap[(buf[i] & 0xF0) >> 4]);
        Serial.write(c_hexmap[buf[i] & 0x0F]);
    }
}

int key_gen_random(void* p) {
    return (int)HAL_RNG_GetRandomNumber();
}

/* executes once at startup */
void setup() {
    Serial.begin(57600);

    int error = 1;
    while (s_loops > 0) {
        memset(pubkey, 0, sizeof(pubkey));
        memset(privkey, 0, sizeof(privkey));
        error = gen_rsa_key(privkey, EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH, key_gen_random, NULL);
        if (!error) {
            Serial.print("keys:");
            extract_public_rsa_key(pubkey, privkey);            
            printHex(privkey, sizeof(privkey));
            Serial.print(":");
            printHex(pubkey, sizeof(pubkey));
            Serial.print("\n");
        }
        s_loops--;
    }
    Serial.print("done");
}
