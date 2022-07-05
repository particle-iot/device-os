
#ifndef FLASH_DEVICE_HAL_H
#define	FLASH_DEVICE_HAL_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum flash_device_t {
    FLASH_INTERNAL,
    FLASH_SERIAL,
    FLASH_ADDRESS // Address that is directly accessible (e.g. RAM)
} flash_device_t;


#ifdef	__cplusplus
}
#endif

#endif	/* FLASH_DEVICE_HAL_H */

