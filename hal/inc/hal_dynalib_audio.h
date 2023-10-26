#pragma once

#include "dynalib.h"

#include "platforms.h"

#ifdef DYNALIB_EXPORT
#include "audio_hal.h"
#endif // DYNALIB_EXPORT

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_audio)

DYNALIB_FN(0, hal_audio, hal_audio_init, int(hal_audio_out_device_t, hal_audio_mode_t, hal_audio_sample_rate_t, hal_audio_word_len_t))
DYNALIB_FN(1, hal_audio, hal_audio_deinit, int(void))
DYNALIB_FN(2, hal_audio, hal_audio_read_dmic, int(void*, size_t))
DYNALIB_FN(3, hal_audio, hal_audio_write_lineout, int(const void*, size_t))

DYNALIB_END(hal_audio)

#undef BASE_IDX
