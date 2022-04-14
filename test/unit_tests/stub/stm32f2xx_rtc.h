#ifndef TEST_STUBS_STM32F2XX_RTC_H
#define TEST_STUBS_STM32F2XX_RTC_H

#include <stdint.h>

#define RTC_BKP_DR0 ((uint32_t)0x00000000)
#define RTC_BKP_DR1 ((uint32_t)0x00000001)
#define RTC_BKP_DR2 ((uint32_t)0x00000002)
#define RTC_BKP_DR3 ((uint32_t)0x00000003)
#define RTC_BKP_DR4 ((uint32_t)0x00000004)
#define RTC_BKP_DR5 ((uint32_t)0x00000005)
#define RTC_BKP_DR6 ((uint32_t)0x00000006)
#define RTC_BKP_DR7 ((uint32_t)0x00000007)
#define RTC_BKP_DR8 ((uint32_t)0x00000008)
#define RTC_BKP_DR9 ((uint32_t)0x00000009)
#define RTC_BKP_DR10 ((uint32_t)0x0000000A)
#define RTC_BKP_DR11 ((uint32_t)0x0000000B)
#define RTC_BKP_DR12 ((uint32_t)0x0000000C)
#define RTC_BKP_DR13 ((uint32_t)0x0000000D)
#define RTC_BKP_DR14 ((uint32_t)0x0000000E)
#define RTC_BKP_DR15 ((uint32_t)0x0000000F)
#define RTC_BKP_DR16 ((uint32_t)0x00000010)
#define RTC_BKP_DR17 ((uint32_t)0x00000011)
#define RTC_BKP_DR18 ((uint32_t)0x00000012)
#define RTC_BKP_DR19 ((uint32_t)0x00000013)

#ifdef __cplusplus
extern "C" {
#endif

uint32_t RTC_ReadBackupRegister(uint32_t RTC_BKP_DR);
void RTC_WriteBackupRegister(uint32_t RTC_BKP_DR, uint32_t Data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // TEST_STUBS_STM32F2XX_RTC_H
