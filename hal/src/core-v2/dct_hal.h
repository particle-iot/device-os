/* 
 * File:   dct_hal.h
 * Author: mat
 *
 * Created on 11 November 2014, 09:32
 */

#ifndef DCT_HAL_H
#define	DCT_HAL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "platform_dct.h"    
    
struct hal_system_flags_t {
    platform_system_flags_t flags;
};

struct hal_server_endpoint_t {
    
};
    
/**
 * Custom extensions to the DCT data stored
 */    
struct hal_dct {
    
    hal_system_flags_t hal_system_flags;
    
};


#ifdef	__cplusplus
}
#endif

#endif	/* DCT_HAL_H */

