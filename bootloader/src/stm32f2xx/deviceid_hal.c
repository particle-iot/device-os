// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/stm32f2xx/deviceid_hal.c"

// Pulled from system_cloud_internal.cpp

static inline char ascii_nibble(uint8_t nibble) {
    char hex_digit = nibble + 48;
    if (57 < hex_digit)
        hex_digit += 7;
    return hex_digit;
}

static inline char* concat_nibble(char* p, uint8_t nibble)
{
    *p++ = ascii_nibble(nibble);
    return p;
}

char* bytes2hexbuf(const uint8_t* buf, unsigned len, char* out)
{
    unsigned i;
    char* result = out;
    for (i = 0; i < len; ++i)
    {
        concat_nibble(out, (buf[i] >> 4));
        out++;
        concat_nibble(out, (buf[i] & 0xF));
        out++;
    }
    return result;
}
