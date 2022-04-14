
/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RCC_FLAG_IWDGRST 0

// FIXME: These flag names are too common to be defined globally
typedef enum FlagStatus {
    RESET = 0,
    SET = 1
} FlagStatus;

void Save_Reset_Syndrome();
uint8_t RCC_GetFlagStatus(uint8_t flag);
void RCC_ClearFlag(void);

#ifdef __cplusplus
}
#endif
