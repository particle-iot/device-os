/* 
 * File:   deviceid_hal.h
 * Author: mat
 *
 * Created on 25 September 2014, 00:22
 */

#ifndef DEVICEID_HAL_H
#define	DEVICEID_HAL_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Fetches the unique ID for this device.
 * 
 * @param dest      The buffer to receive the device ID
 * @param destLen   The maximum length of the buffer. Can be 0.
 * @return          The number of bytes in the device ID. This is independent 
 *                  of the buffer size. 
 * 
 * To find out the size of the device ID, call this method with 
 * {@code destLen}==0.
 */        
unsigned HAL_device_ID(uint8_t* dest, unsigned destLen);


#ifdef	__cplusplus
}
#endif

#endif	/* DEVICEID_HAL_H */

