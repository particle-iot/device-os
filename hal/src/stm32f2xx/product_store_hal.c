
#include "product_store_hal.h"
#include "dct_hal.h"


inline uint16_t dct_offset(ProductStoreIndex idx) __attribute__((always_inline));
inline uint16_t dct_offset(ProductStoreIndex idx) {
    return DCT_PRODUCT_STORE_OFFSET+(sizeof(uint16_t)*idx);
}

inline uint16_t* dct_product_store_offset(ProductStoreIndex idx) __attribute__((always_inline));
inline uint16_t* dct_product_store_offset(ProductStoreIndex idx)
{
    return (uint16_t*)dct_read_app_data(dct_offset(idx));
}

/**
 * Sets the value at a specific product store index.
 * @return The previous value.
 */
uint16_t HAL_SetProductStore(ProductStoreIndex index, uint16_t value)
{
    uint16_t* store = dct_product_store_offset(index);
    uint16_t oldValue = *store;
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
    uint16_t* value = dct_product_store_offset(index);
    return *value;
}

