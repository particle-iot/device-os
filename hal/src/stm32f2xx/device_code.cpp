
#include "dct.h"
#include "device_code.h"
#include "rng_hal.h"
#include <string.h>

const int DEVICE_CODE_LEN = 6;

STATIC_ASSERT(device_code_len_is_same_as_dct_storage, DEVICE_CODE_LEN<=DCT_DEVICE_CODE_SIZE);


/**
 * Converts a given 32-bit value to a alphanumeric code
 * @param value     The value to convert
 * @param dest      The number of charactres
 * @param len
 */
void bytesToCode(uint32_t value, char* dest, unsigned len) {
    static const char* symbols = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    while (len --> 0) {
        *dest++ = symbols[value % 32];
        value /= 32;
    }
}

/**
 * Generates a random code.
 * @param dest
 * @param len   The length of the code, should be event.
 */
void random_code(uint8_t* dest, unsigned len) {
    unsigned value = HAL_RNG_GetRandomNumber();
    bytesToCode(value, (char*)dest, len);
}



bool fetch_or_generate_setup_ssid(device_code_t* SSID) {
    bool result = fetch_or_generate_ssid_prefix(SSID);
    SSID->value[SSID->length++] = '-';
    result |= fetch_or_generate_device_code(SSID);
    return result;
}


/**
 * Copies the device code to the destination, generating it if necessary.
 * @param dest      A buffer with room for at least 6 characters. The
 *  device code is copied here, without a null terminator.
 * @return true if the device code was generated.
 */
bool fetch_or_generate_device_code(device_code_t* SSID) {
    const uint8_t* suffix = (const uint8_t*)dct_read_app_data_lock(DCT_DEVICE_CODE_OFFSET);
    int8_t c = (int8_t)*suffix;    // check out first byte
    bool generate = (!c || c<0);
    uint8_t* dest = SSID->value+SSID->length;
    uint8_t code_length = DEVICE_CODE_LEN;
    if (generate) {
        dct_read_app_data_unlock(DCT_DEVICE_CODE_OFFSET);
        random_code(dest, DEVICE_CODE_LEN);
        dct_write_app_data(dest, DCT_DEVICE_CODE_OFFSET, DEVICE_CODE_LEN);
    }
    else {
        memcpy(dest, suffix, DEVICE_CODE_LEN);
        // prior to 0.7.0-rc.3 the device code was 4 digits
        // termination character is just unwritten flash, 0xFF
        code_length = (dest[4]>127 || dest[4]<32) ? 4 : DEVICE_CODE_LEN;
        dct_read_app_data_unlock(DCT_DEVICE_CODE_OFFSET);
    }
    SSID->length += code_length;

    return generate;
}
