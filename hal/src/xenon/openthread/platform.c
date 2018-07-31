/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes the platform-specific initializers.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/platform/logging.h>

#include "platform-nrf5.h"
#include <nrf_drv_clock.h>
#include <nrf.h>
#if SOFTDEVICE_PRESENT
#include <nrf_sdh_soc.h>
#include "softdevice.h"
#include "platform-softdevice.h"
#endif /* SOFTDEVICE_PRESENT */

#include <openthread/config.h>

#include "gpio_hal.h"
#include "platforms.h"

static void selectAntenna(bool external) {
    HAL_Pin_Mode(ANTSW1, OUTPUT);
#if (PLATFORM_ID == PLATFORM_XENON) || (PLATFORM_ID == PLATFORM_ARGON)
    HAL_Pin_Mode(ANTSW2, OUTPUT);
#endif

    if (external) {
#if (PLATFORM_ID == PLATFORM_ARGON)
        HAL_GPIO_Write(ANTSW1, 1);
        HAL_GPIO_Write(ANTSW2, 0);
#elif (PLATFORM_ID == PLATFORM_BORON)
        HAL_GPIO_Write(ANTSW1, 0);
#else
        HAL_GPIO_Write(ANTSW1, 0);
        HAL_GPIO_Write(ANTSW2, 1);
#endif
    } else {
#if (PLATFORM_ID == PLATFORM_ARGON)
        HAL_GPIO_Write(ANTSW1, 0);
        HAL_GPIO_Write(ANTSW2, 1);
#elif (PLATFORM_ID == PLATFORM_BORON)
        HAL_GPIO_Write(ANTSW1, 1);
#else
        HAL_GPIO_Write(ANTSW1, 1);
        HAL_GPIO_Write(ANTSW2, 0);
#endif
    }
}

static void processSocEvent(uint32_t event, void* data) {
    PlatformSoftdeviceSocEvtHandler(event);
}

void PlatformInit(int argc, char *argv[])
{
    extern bool gPlatformPseudoResetWasRequested;

    if (gPlatformPseudoResetWasRequested)
    {
        nrf5AlarmDeinit();
        nrf5AlarmInit();

        gPlatformPseudoResetWasRequested = false;

        return;
    }

    (void)argc;
    (void)argv;

#if !SOFTDEVICE_PRESENT
    // Enable I-code cache
    NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Enabled;
#endif

    /* Just in case force the antenna to internal one */
    selectAntenna(false);

//     nrf_drv_clock_init();

// #if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
//     nrf5LogInit();
// #endif
    nrf5AlarmInit();
    nrf5RandomInit();
//     nrf5UartInit();
// #ifndef SPIS_TRANSPORT_DISABLE
//     nrf5SpiSlaveInit();
// #endif
//     nrf5MiscInit();
    nrf5CryptoInit();
    nrf5RadioInit();
    nrf5TempInit();

#if SOFTDEVICE_PRESENT
    NRF_SDH_SOC_OBSERVER(socObserver, NRF_SDH_SOC_STACK_OBSERVER_PRIO, processSocEvent, NULL);
#endif /* SOFTDEVICE_PRESENT */

#if SOFTDEVICE_PRESENT
    // Correct the PPM for the LF crystal
    const PlatformSoftdeviceRaalConfigParams config = {
        .timeslotLength     = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_LENGTH,
        .timeslotTimeout    = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_TIMEOUT,
        .timeslotMaxLength  = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_MAX_LENGTH,
        .timeslotAllocIters = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_ALLOC_ITERS,
        .timeslotSafeMargin = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_SAFE_MARGIN,
        .lfClkAccuracyPpm   = 20
    };
    PlatformSoftdeviceRaalConfig(&config);
#endif /* SOFTDEVICE_PRESENT */
}

void PlatformDeinit(void)
{
    nrf5TempDeinit();
    nrf5RadioDeinit();
    nrf5CryptoDeinit();
//     nrf5MiscDeinit();
// #ifndef SPIS_TRANSPORT_DISABLE
//     nrf5SpiSlaveDeinit();
// #endif
//      nrf5UartDeinit();
    nrf5RandomDeinit();
    nrf5AlarmDeinit();
// #if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
//     nrf5LogDeinit();
// #endif
}

bool PlatformPseudoResetWasRequested(void)
{
    extern bool gPlatformPseudoResetWasRequested;
    return gPlatformPseudoResetWasRequested;
}

void PlatformProcessDrivers(otInstance *aInstance)
{
    nrf5AlarmProcess(aInstance);
    nrf5RadioProcess(aInstance);
//     nrf5UartProcess();
    nrf5TempProcess();
// #ifndef SPIS_TRANSPORT_DISABLE
//     nrf5SpiSlaveProcess();
// #endif
}

__WEAK void PlatformEventSignalPending(void)
{
    // Intentionally empty
}
