#pragma once

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "dct_hal.h"
#endif

DYNALIB_BEGIN(hal_dct)

DYNALIB_FN(0, hal_dct, dct_read_app_data, const void*(uint32_t))
DYNALIB_FN(1, hal_dct, dct_read_app_data_copy, int(uint32_t, void*, size_t))
DYNALIB_FN(2, hal_dct, dct_read_app_data_lock, const void*(uint32_t))
DYNALIB_FN(3, hal_dct, dct_read_app_data_unlock, int(uint32_t))
DYNALIB_FN(4, hal_dct, dct_write_app_data, int(const void*, uint32_t, uint32_t))

DYNALIB_END(hal_dct)
