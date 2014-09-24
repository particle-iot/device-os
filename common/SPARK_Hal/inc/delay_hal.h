/* 
 * File:   delay_hal.h
 * Author: mat
 *
 * Created on 23 September 2014, 22:09
 */

#ifndef DELAY_HAL_H
#define	DELAY_HAL_H

#ifdef	__cplusplus
extern "C" {
#endif

    void HAL_Delay_Milliseconds(uint32_t millis);
    void HAL_Delay_Microseconds(uint32_t micros);


#ifdef	__cplusplus
}
#endif

#endif	/* DELAY_HAL_H */

