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

typedef struct platform_system_flags {    
    uint16_t header[2];
    uint16_t CORE_FW_Version_SysFlag;
    uint16_t NVMEM_SPARK_Reset_SysFlag;
    uint16_t FLASH_OTA_Update_SysFlag;
    uint16_t Factory_Reset_SysFlag;    
    uint16_t reserved[10];
    
} platform_system_flags_t;


#ifdef	__cplusplus
}
#endif

#endif	/* PLATFORM_SYSTEM_FLAGS_H */

