/* 
 * File:   syshealth_hal.h
 * Author: mat
 *
 * Created on 21 September 2014, 12:06
 */

#ifndef SYSHEALTH_HAL_H
#define	SYSHEALTH_HAL_H

#ifdef	__cplusplus
extern "C" {
#endif

enum eSystemHealth {
  FIRST_RETRY = 1,
  SECOND_RETRY = 2,
  THIRD_RETRY = 3,
  ENTERED_SparkCoreConfig,
  ENTERED_Main,
  ENTERED_WLAN_Loop,
  ENTERED_Setup,
  ENTERED_Loop,
  RAN_Loop,
  PRESERVE_APP,
};


#if PLATFORM_MCU==STM32
#define SET_SYS_HEALTH(health) BKP_WriteBackupRegister(BKP_DR1, (health))
#define GET_SYS_HEALTH() BKP_ReadBackupRegister(BKP_DR1)
#endif

extern uint16_t sys_health_cache;
#define DECLARE_SYS_HEALTH(health)  do { if ((health) > sys_health_cache) {SET_SYS_HEALTH(sys_health_cache=(health));}} while(0)



#ifdef	__cplusplus
}
#endif

#endif	/* SYSHEALTH_HAL_H */

