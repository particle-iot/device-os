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

#include "openthread-system.h"
#include "platform-fem.h"
#include "platform-nrf5.h"
#include <nrf_drv_clock.h>
#include <nrf.h>

#if SOFTDEVICE_PRESENT
#include <nrf_sdh_soc.h>
#include "softdevice.h"
#include "platform-softdevice.h"
#endif /* SOFTDEVICE_PRESENT */

#include <openthread/config.h>

extern bool gPlatformPseudoResetWasRequested;

#include "gpio_hal.h"
#include "platforms.h"

static void selectAntenna(bool external) {
    // Mesh SoM don't have on-board antenna switch.
#if (PLATFORM_ID == PLATFORM_XSOM) || (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_BSOM)
    return;
#else
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
#endif // (PLATFORM_ID == PLATFORM_XSOM) || (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_ASOM)
}

static void processSocEvent(uint32_t event, void* data) {
    otSysSoftdeviceSocEvtHandler(event);
}

void otSysInit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    if (gPlatformPseudoResetWasRequested)
    {
        otSysDeinit();
    }

#if !SOFTDEVICE_PRESENT
    // Enable I-code cache
    NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Enabled;
#endif

    /* Just in case force the antenna to internal one */
    selectAntenna(false);

    // Alarm is initialize much earlier in our core HAL
    // nrf5AlarmInit();
    nrf5RandomInit();

    if (!gPlatformPseudoResetWasRequested)
    {
        nrf5CryptoInit();
    }
    nrf5RadioInit();
    nrf5TempInit();

#if SOFTDEVICE_PRESENT
    NRF_SDH_SOC_OBSERVER(socObserver, NRF_SDH_SOC_STACK_OBSERVER_PRIO, processSocEvent, NULL);
#endif /* SOFTDEVICE_PRESENT */

#if SOFTDEVICE_PRESENT
    // Correct the PPM for the LF crystal
    const otSysSoftdeviceRaalConfigParams config = {
        .timeslotLength     = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_LENGTH,
        .timeslotTimeout    = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_TIMEOUT,
        .timeslotMaxLength  = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_MAX_LENGTH,
        .timeslotAllocIters = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_ALLOC_ITERS,
        .timeslotSafeMargin = PLATFORM_SOFTDEVICE_RAAL_TIMESLOT_DEFAULT_SAFE_MARGIN,
        .lfClkAccuracyPpm   = PLATFORM_SOFTDEVICE_RAAL_DEFAULT_LF_CLK_ACCURACY_PPM
    };
    otSysSoftdeviceRaalConfig(&config);
#endif /* SOFTDEVICE_PRESENT */

#if PLATFORM_FEM_ENABLE_DEFAULT_CONFIG
    PlatformFemSetConfigParams(&PLATFORM_FEM_DEFAULT_CONFIG);
#endif

    gPlatformPseudoResetWasRequested = false;
}

void otSysDeinit(void)
{
    nrf5TempDeinit();
    nrf5RadioDeinit();

    if (!gPlatformPseudoResetWasRequested) {
        nrf5CryptoDeinit();
    }
    nrf5RandomDeinit();
    // We need Alarm for normal core HAL functionality
    // nrf5AlarmDeinit();
}

bool otSysPseudoResetWasRequested(void)
{
    extern bool gPlatformPseudoResetWasRequested;
    return gPlatformPseudoResetWasRequested;
}

void otSysProcessDrivers(otInstance *aInstance)
{
    nrf5RadioProcess(aInstance);
    nrf5TempProcess();
    nrf5AlarmProcess(aInstance);
}

__WEAK void otSysEventSignalPending(void)
{
    // Intentionally empty
}
