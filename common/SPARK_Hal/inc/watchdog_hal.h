/* 
 * File:   watchdog_hal.h
 * Author: mat
 *
 * Created on 23 September 2014, 22:05
 */

#ifndef WATCHDOG_HAL_H
#define	WATCHDOG_HAL_H

#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

bool HAL_watchdog_reset_flagged();
    
void HAL_Notify_WDT();


#ifdef	__cplusplus
}
#endif

#endif	/* WATCHDOG_HAL_H */

