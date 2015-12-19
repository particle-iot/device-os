/* Copyright (c) 2014 Paul Kourany, based on work by Dianel Gilbert

UPDATED Sept 3, 2015 - Added support for Particle Photon

Copyright (c) 2013 Daniel Gilbert, loglow@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */


#ifndef __INTERVALTIMER_H__
#define __INTERVALTIMER_H__

#include "application.h"


#if defined(STM32F10X_MD) || !defined(PLATFORM_ID)		//Core
  #define SYSCORECLOCK	72000000UL

#elif defined(STM32F2XX) && defined(PLATFORM_ID)	//Photon
  #define SYSCORECLOCK	60000000UL		// Timer clock tree uses core clock / 2
#else
  #error "*** PARTICLE device not supported by this library. PLATFORM should be Core or Photon ***"
#endif

enum {uSec, hmSec};			// microseconds or half-milliseconds
enum action {INT_DISABLE, INT_ENABLE};

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(PLATFORM_ID)							//Core v0.3.4
#warning "CORE"
extern void (*Wiring_TIM2_Interrupt_Handler)(void);
extern void (*Wiring_TIM3_Interrupt_Handler)(void);
extern void (*Wiring_TIM4_Interrupt_Handler)(void);

extern void Wiring_TIM2_Interrupt_Handler_override(void);
extern void Wiring_TIM3_Interrupt_Handler_override(void);
extern void Wiring_TIM4_Interrupt_Handler_override(void);

enum TIMid {TIMER2, TIMER3, TIMER4, AUTO=255};
typedef uint16_t intPeriod;
#elif defined(STM32F10X_MD)							//Core
#warning "CORE NEW"
extern void Wiring_TIM2_Interrupt_Handler_override(void);
extern void Wiring_TIM3_Interrupt_Handler_override(void);
extern void Wiring_TIM4_Interrupt_Handler_override(void);

enum TIMid {TIMER2, TIMER3, TIMER4, AUTO=255};
typedef uint16_t intPeriod;
#elif defined(STM32F2XX)							//Photon
extern void Wiring_TIM3_Interrupt_Handler_override(void);
extern void Wiring_TIM4_Interrupt_Handler_override(void);
extern void Wiring_TIM5_Interrupt_Handler_override(void);
extern void Wiring_TIM6_Interrupt_Handler_override(void);
extern void Wiring_TIM7_Interrupt_Handler_override(void);

enum TIMid {TIMER3, TIMER4, TIMER5, TIMER6, TIMER7, AUTO=255};
typedef uint32_t intPeriod;
#endif

class IntervalTimer {
  private:
	typedef void (*ISRcallback)();
    enum {TIMER_OFF, TIMER_SIT};
#if defined(STM32F10X_MD) || !defined(PLATFORM_ID)		//Core
    static const uint8_t NUM_SIT = 3;
#elif defined(STM32F2XX) && defined(PLATFORM_ID)	//Photon
    static const uint8_t NUM_SIT = 5;
#endif

	bool sysIntSetupDone = false;

	// Timer ClockDivision = DIV4
	const uint16_t SIT_PRESCALERu = (uint16_t)(SYSCORECLOCK / 1000000UL) - 1;	//To get TIM counter clock = 1MHz
	const uint16_t SIT_PRESCALERm = (uint16_t)(SYSCORECLOCK / 2000UL) - 1;	//To get TIM counter clock = 2KHz
    const uint16_t MAX_PERIOD = UINT16_MAX;		// 1-65535 us

    static bool SIT_used[NUM_SIT];
    bool allocate_SIT(intPeriod Period, bool scale, TIMid id);
    void start_SIT(intPeriod Period, bool scale);
    void stop_SIT();
    bool status;
    uint8_t SIT_id;
 	ISRcallback myISRcallback;

    bool beginCycles(void (*isrCallback)(), intPeriod Period, bool scale, TIMid id);

  public:
    IntervalTimer() {
	status = TIMER_OFF;

	for (int i=0; i < NUM_SIT; i++)		//Set all SIT slots to unused
		SIT_used[i] = false;

	// Attach timer interrupt handlers
#if !defined(PLATFORM_ID)							//Core v0.3.4
        Wiring_TIM2_Interrupt_Handler = Wiring_TIM2_Interrupt_Handler_override;
        Wiring_TIM3_Interrupt_Handler = Wiring_TIM3_Interrupt_Handler_override;
        Wiring_TIM4_Interrupt_Handler = Wiring_TIM4_Interrupt_Handler_override;

#elif defined(STM32F10X_MD)							//Core
	if (!sysIntSetupDone) {
		sysIntSetupDone = true;
		if (!attachSystemInterrupt(SysInterrupt_TIM2, Wiring_TIM2_Interrupt_Handler_override)) ;	//error
		if (!attachSystemInterrupt(SysInterrupt_TIM3, Wiring_TIM3_Interrupt_Handler_override)) ;	//error
		if (!attachSystemInterrupt(SysInterrupt_TIM4, Wiring_TIM4_Interrupt_Handler_override)) ;	//error
	}
#elif defined(STM32F2XX) && defined(PLATFORM_ID)	//Photon
	if (!sysIntSetupDone) {
		sysIntSetupDone = true;
		if (!attachSystemInterrupt(SysInterrupt_TIM3_Update, Wiring_TIM3_Interrupt_Handler_override)) ;	//error
		if (!attachSystemInterrupt(SysInterrupt_TIM4_Update, Wiring_TIM4_Interrupt_Handler_override)) ;	//error
		if (!attachSystemInterrupt(SysInterrupt_TIM5_Update, Wiring_TIM5_Interrupt_Handler_override)) ;	//error
		if (!attachSystemInterrupt(SysInterrupt_TIM6_Update, Wiring_TIM6_Interrupt_Handler_override));	//error
		if (!attachSystemInterrupt(SysInterrupt_TIM7_Update, Wiring_TIM7_Interrupt_Handler_override));	//error
	}
#endif

    }

    ~IntervalTimer() { end(); }

    bool begin(void (*isrCallback)(), intPeriod Period, bool scale) {
		if (Period < 10 || Period > MAX_PERIOD)
			return false;
		return beginCycles(isrCallback, Period, scale, AUTO);
    }

    bool begin(void (*isrCallback)(), intPeriod Period, bool scale, TIMid id) {
		if (Period < 10 || Period > MAX_PERIOD)
			return false;
		return beginCycles(isrCallback, Period, scale, id);
    }

    void end();
	void interrupt_SIT(action ACT);
	void resetPeriod_SIT(intPeriod newPeriod, bool scale);
	int8_t isAllocated_SIT(void);

    static ISRcallback SIT_CALLBACK[NUM_SIT];
};


#ifdef __cplusplus
}
#endif

#endif
