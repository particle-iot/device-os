
#include "product_store_hal.h"
#include "user_hal.h"
#include "ota_flash_hal.h"
#include "ota_flash_hal_impl.h"
#include <cstring>
#include "check.h"
#include "system_error.h"
#include "storage_hal.h"

/**
 * Sets the value at a specific product store index.
 * @return The previous value.
 */
int HAL_SetProductStore(ProductStoreIndex index, uint16_t value)
{
    return 0xFFFF;
}

/**
 * Fetches the value at a given index in the product store.
 * @param index
 * @return
 */
int HAL_GetProductStore(ProductStoreIndex index)
{
    if (index != PRODUCT_STORE_ID && index != PRODUCT_STORE_VERSION) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (index == PRODUCT_STORE_ID) {
        return PLATFORM_ID;
    }

    hal_user_module_descriptor desc = {};
    CHECK(hal_user_module_get_descriptor(&desc));

    auto storageId = HAL_STORAGE_ID_INVALID;
    if (module_user.location == MODULE_BOUNDS_LOC_INTERNAL_FLASH) {
        storageId = HAL_STORAGE_ID_INTERNAL_FLASH;
    } else if (module_user.location == MODULE_BOUNDS_LOC_EXTERNAL_FLASH) {
        storageId = HAL_STORAGE_ID_EXTERNAL_FLASH;
    }
    CHECK_FALSE(storageId == HAL_STORAGE_ID_INVALID, SYSTEM_ERROR_INVALID_STATE);

    module_info_product_data_ext_t product = {};
    uintptr_t productDataStart = (uintptr_t)desc.info.module_end_address - sizeof(module_info_suffix_base_t) - sizeof(module_info_product_data_ext_t);
    CHECK(hal_storage_read(storageId, productDataStart, (uint8_t*)&product, sizeof(product)));

    return product.version;
}

