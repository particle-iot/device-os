/*
 * File:   bootloader.h
 * Author: mat1
 *
 * Created on June 8, 2015, 4:45 PM
 */

#ifndef BOOTLOADER_H
#define	BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

bool bootloader_requires_update(const uint8_t* bootloader_image, uint32_t length);
bool bootloader_update_if_needed();

int bootloader_update(const void* bootloader_image, unsigned length);


#ifdef __cplusplus
}
#endif

#endif	/* BOOTLOADER_H */

