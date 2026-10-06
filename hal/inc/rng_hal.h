/**
 ******************************************************************************
 * @file    rng_hal.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-Jan-2015
 * @brief
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RNG_HAL_H
#define __RNG_HAL_H

#include <stddef.h>
#include <stdint.h>
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_RNG_Configuration(void);
uint32_t HAL_RNG_GetRandomNumber(void);

#if !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)

#define HAL_RNG_ENTROPY_REPETITION_COUNT_CUTOFF     (81)
#define HAL_RNG_ENTROPY_ADAPTIVE_PROPORTION_WINDOW  (1024)
#define HAL_RNG_ENTROPY_ADAPTIVE_PROPORTION_CUTOFF  (914)

/**
 * Reseed the DRBG from the platform entropy source.
 */
int hal_rng_reseed(void* reserved);

/**
 * Read raw entropy source samples, bypassing the health tests, for source qualification.
 *
 * @param samples: filled in with raw samples
 * @param count: number of samples to read
 */
int hal_rng_entropy_read_raw(uint16_t* samples, size_t count, void* reserved);

#endif // !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)

#ifdef __cplusplus
}
#endif

#endif  /* __RNG_HAL_H */
