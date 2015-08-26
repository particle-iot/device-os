#ifndef SPARK_WIRING_PLATFORM_H
#define	SPARK_WIRING_PLATFORM_H

#include "inet_hal.h"

/**
 *  This header file maps platform ID to compile-time switches for the Wiring API.
 */        
   
#if PLATFORM_ID==0      // core
#define Wiring_WiFi 1
#define Wiring_IPv6 0
#endif    

#if PLATFORM_ID==1      // unused
#error Unkonwn platform ID
#endif    

#if PLATFORM_ID==2      // core-hd
#define Wiring_WiFi 1
#define Wiring_IPv6 0
#endif    

#if PLATFORM_ID==3      // gcc
#define Wiring_WiFi 1
#define Wiring_IPv6 0
#define Wiring_SPI1 1
#endif    

#if PLATFORM_ID==4      // photon dev
#define Wiring_WiFi 1
#define Wiring_IPv6 1
#define Wiring_SPI1 1
#endif    

#if PLATFORM_ID==5      
#define Wiring_WiFi 1
#define Wiring_IPv6 1
#define Wiring_SPI1 1
#endif    

#if PLATFORM_ID==6      // photon
#define Wiring_WiFi 1
#define Wiring_IPv6 1
#define Wiring_SPI1 1
#endif    

#if PLATFORM_ID==7      
#define Wiring_WiFi 1
#define Wiring_IPv6 1
#define Wiring_SPI1 1
#endif    
        
#if PLATFORM_ID==8      // P1 / bm14
#define Wiring_WiFi 1
#define Wiring_IPv6 1
#define Wiring_SPI1 1
#endif    
    
#if PLATFORM_ID==9      // ethernet
#define Wiring_WiFi 0
#define Wiring_IPv6 1
#endif    

#if PLATFORM_ID==10      // electron
#define Wiring_SPI1 1
#define Wiring_SPI2 1

#define Wiring_Wire1 1
#define Wiring_Wire3 1 /* System PMIC and Fuel Guage I2C3 */
#endif

#ifndef Wiring_SPI1
#define Wiring_SPI1 0
#endif

#ifndef Wiring_SPI2
#define Wiring_SPI2 0
#endif
    
#ifndef Wiring_Wire1
#define Wiring_Wire1 0
#endif

#ifndef Wiring_Wire3
#define Wiring_Wire3 0
#endif

#endif	/* SPARK_WIRING_PLATFORM_H */

