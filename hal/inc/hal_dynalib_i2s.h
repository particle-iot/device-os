#pragma once

#include "dynalib.h"

#include "platforms.h"

#ifdef DYNALIB_EXPORT
#include "i2s_hal.h"
#endif // DYNALIB_EXPORT

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_i2s)

DYNALIB_FN(0, hal_i2s, hal_i2s_init, int(void))
DYNALIB_FN(1, hal_i2s, hal_i2s_deinit, int(void))
DYNALIB_FN(2, hal_i2s, hal_i2s_play, void(const void*, size_t))
DYNALIB_FN(3, hal_i2s, hal_i2s_ready, bool(void))

DYNALIB_END(hal_i2s)

#undef BASE_IDX
