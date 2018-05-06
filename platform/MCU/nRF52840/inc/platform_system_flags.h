/*
 * File:   platform_system_flags.h
 * Author: mat
 *
 * Created on 12 November 2014, 06:24
 */

#ifndef PLATFORM_SYSTEM_FLAGS_H
#define	PLATFORM_SYSTEM_FLAGS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "static_assert.h"

typedef struct __attribute__((packed)) platform_system_flags {
    uint32_t header;
    uint16_t Bootloader_Version_SysFlag;
    uint16_t NVMEM_SPARK_Reset_SysFlag;
    uint16_t FLASH_OTA_Update_SysFlag;
    uint16_t OTA_FLASHED_Status_SysFlag;
    uint16_t Factory_Reset_SysFlag;
    uint16_t IWDG_Enable_SysFlag;
    uint8_t dfu_on_no_firmware;     // flag to enable DFU mode when no firmware is available.
    /**
     * Set to 0x5A to indicate that the bootloader just performed a factory reset.
     * The application should clear this flag.
     */
    uint8_t Factory_Reset_Done_SysFlag;
    /**
     * This is a placeholder for when the bootloader can influence how the system module executes.
     */
    uint8_t StartupMode_SysFlag;
    uint8_t FeaturesEnabled_SysFlag;        // default is 0xFF all features enabled. If any bits are cleared in the bottom 4-bits, then the upper 4 bits should be the logical inverse of these.
                                            // This is to prevent against corrupted data causing the bootloader to be unavailable.
    uint32_t RCC_CSR_SysFlag;
    uint16_t reserved[4];
} platform_system_flags_t;

STATIC_ASSERT(platform_system_flags_size_changed, sizeof(platform_system_flags_t) == 32);

#ifdef	__cplusplus
}
#endif

#endif	/* PLATFORM_SYSTEM_FLAGS_H */

