#include "led_signal.h"

#include "system_led_signal.h"

#include "dct.h"

// See system_led_signal.cpp for the description of the LED theme data format
STATIC_ASSERT(led_theme_version_has_changed, LED_SIGNAL_THEME_VERSION == 1);

#define UNPACK_COLOR_COMPONENT(_value) \
        ((((_value) & 0x0f) << 4) | ((_value) & 0x0f)) /* 0 -> 0, 1 -> 17, 2 -> 34, ..., 15 -> 255 */ \

static uint32_t led_signal_color(int signal, const uint8_t* data) {
    data += signal * 3 + 1; // 3 bytes per signal; first byte is reserved for a version number
    return ((uint32_t)UNPACK_COLOR_COMPONENT(*data >> 4) << 16) | // R
            ((uint32_t)UNPACK_COLOR_COMPONENT(*data & 0x0f) << 8) | // G
            (uint32_t)UNPACK_COLOR_COMPONENT(*(data + 1) >> 4); // B
}

void get_led_theme_colors(uint32_t* firmware_update, uint32_t* safe_mode, uint32_t* dfu_mode) {
    // Check if theme data is initialized in DCT
    const uint8_t* d = (const uint8_t*)dct_read_app_data_lock(DCT_LED_THEME_OFFSET);
    if (d && *d == LED_SIGNAL_THEME_VERSION) {
        *firmware_update = led_signal_color(LED_SIGNAL_FIRMWARE_UPDATE, d);
        *safe_mode = led_signal_color(LED_SIGNAL_SAFE_MODE, d);
        *dfu_mode = led_signal_color(LED_SIGNAL_DFU_MODE, d);
    }
    dct_read_app_data_unlock(DCT_LED_THEME_OFFSET);
}
