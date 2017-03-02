#include "dct_hal.h"
#include "wiced_result.h"
#include "wiced_dct_common.h"
#include "waf_platform.h"

extern wiced_dct_sdk_ver_t wiced_dct_validate_and_determine_version(uint32_t device_start_address, uint32_t device_end_address, int *initial_write, int *sequence);

const void* dct_read_app_data(uint32_t offset) {
    uint32_t current = (uint32_t)wiced_dct_get_current_address(DCT_INTERNAL_SECTION);
    uint32_t end = 0;
    if (current == PLATFORM_DCT_COPY1_START_ADDRESS) {
        end = PLATFORM_DCT_COPY1_END_ADDRESS;
    } else if (current == PLATFORM_DCT_COPY2_START_ADDRESS) {
        end = PLATFORM_DCT_COPY2_END_ADDRESS;
    }
    wiced_dct_sdk_ver_t ver = wiced_dct_validate_and_determine_version(current, end, NULL, NULL);

    if (ver == DCT_BOOTLOADER_SDK_CURRENT) {
        return ((const void*)((uint32_t)wiced_dct_get_current_address(DCT_APP_SECTION) + offset));
    } else {
        return ((const void*)((uint32_t)wiced_dct_get_current_address(DCT_APP_SECTION) + offset - (sizeof(platform_dct_data_t) - sizeof(bootloader_dct_data_t))));
    }
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size ) {
    return wiced_dct_write(data, DCT_APP_SECTION, offset, size);
}
