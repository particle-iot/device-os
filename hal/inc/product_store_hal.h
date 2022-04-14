/*
 * File:   appstore_hal.h
 * Author: mat
 *
 * Created on 02 March 2015, 11:12
 */

#ifndef APPSTORE_HAL_H
#define	APPSTORE_HAL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum ProductStoreIndex
{
    /**
     * The persisted Product ID, 0xFFFF if none set.
     */
    PRODUCT_STORE_ID = 0,

    /**
     * The persisted product version. 0xFFFF if none set.
     */
    PRODUCT_STORE_VERSION = 1


} ProductStoreIndex;

/**
 * Sets the value at a specific product store index.
 * @return The previous value.
 */
uint16_t HAL_SetProductStore(ProductStoreIndex index, uint16_t value);

/**
 * Fetches the value at a given index in the product store.
 * @param index
 * @return
 */
uint16_t HAL_GetProductStore(ProductStoreIndex index);

#ifdef	__cplusplus
}
#endif

#endif	/* APPSTORE_HAL_H */

