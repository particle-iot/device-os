/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stddef.h>

typedef enum {
    HAL_AUDIO_OUT_DEVICE_NONE,
    HAL_AUDIO_OUT_DEVICE_LINEOUT,
} hal_audio_out_device_t;

typedef enum {
    HAL_AUDIO_MODE_MONO,
    HAL_AUDIO_MODE_STEREO
} hal_audio_mode_t;

typedef enum {
    HAL_AUDIO_WORD_LEN_8,
    HAL_AUDIO_WORD_LEN_16,
    HAL_AUDIO_WORD_LEN_24
} hal_audio_word_len_t;

typedef enum {
    HAL_AUDIO_SAMPLE_RATE_8K,
    HAL_AUDIO_SAMPLE_RATE_16K,
    HAL_AUDIO_SAMPLE_RATE_32K,
    HAL_AUDIO_SAMPLE_RATE_44P1K,
    HAL_AUDIO_SAMPLE_RATE_48K,
    HAL_AUDIO_SAMPLE_RATE_88P2K,
    HAL_AUDIO_SAMPLE_RATE_96K,
} hal_audio_sample_rate_t;

#ifdef __cplusplus
extern "C" {
#endif

int hal_audio_init(hal_audio_out_device_t outDevice, hal_audio_mode_t monoStereo, hal_audio_sample_rate_t sampleRate, hal_audio_word_len_t wordLen);
int hal_audio_deinit();
int hal_audio_read_dmic(void* data, size_t size);
int hal_audio_write_lineout(const void* data, size_t size);

#ifdef __cplusplus
}
#endif
