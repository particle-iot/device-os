/* 
 * File:   dct.h
 * Author: mat
 *
 * Created on 12 November 2014, 04:41
 */

#ifndef DCT_H
#define	DCT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>    
    

extern void* dct_read_app_data(uint32_t offset);

extern int dct_write_app_data( const void* data, uint32_t offset, uint32_t size );
    

#ifdef	__cplusplus
}
#endif

#endif	/* DCT_H */

