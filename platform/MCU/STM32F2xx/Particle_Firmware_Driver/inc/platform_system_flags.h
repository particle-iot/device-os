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

typedef struct platform_system_flags {
    uint16_t header[2];
    uint16_t Bootloader_Version_SysFlag;
    uint16_t NVMEM_PARTICLE_Reset_SysFlag;
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
    uint8_t unused;
    uint32_t RCC_CSR_SysFlag;
    uint16_t reserved[4];
} platform_system_flags_t;


#ifdef	__cplusplus
}
#endif

#endif	/* PLATFORM_SYSTEM_FLAGS_H */

