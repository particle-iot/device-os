
#include "product_store_hal.h"
#include "dct_hal.h"


inline uint16_t dct_offset(ProductStoreIndex idx) __attribute__((always_inline));
inline uint16_t dct_offset(ProductStoreIndex idx) {
    return DCT_PRODUCT_STORE_OFFSET+(sizeof(uint16_t)*idx);
}

inline int dct_product_store_offset(ProductStoreIndex idx, uint16_t* ptr) __attribute__((always_inline));
inline int dct_product_store_offset(ProductStoreIndex idx, uint16_t* ptr)
{
    return dct_read_app_data_copy(dct_offset(idx), ptr, sizeof(uint16_t));
}

/**
 * Sets the value at a specific product store index.
 * @return The previous value.
 */
uint16_t HAL_SetProductStore(ProductStoreIndex index, uint16_t value)
{
    uint16_t oldValue = 0;
    dct_product_store_offset(index, &oldValue);
    if (oldValue!=value) {
        dct_write_app_data(&value, dct_offset(index), sizeof(value));
    }
    return oldValue;
}

/**
 * Fetches the value at a given index in the product store.
 * @param index
 * @return
 */
uint16_t HAL_GetProductStore(ProductStoreIndex index)
{
    uint16_t value = 0;
    dct_product_store_offset(index, &value);
    return value;
}

