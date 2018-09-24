/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "check.h"
#include <nrf_error.h>
#include <nrfx_errors.h>

#define CHECK_NRF_RETURN(_expr, _val) \
        ({ \
            const auto _ret = _expr; \
            if (_ret != NRF_SUCCESS && _ret != NRFX_SUCCESS) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _val; \
            } \
            _ret; \
        })

#define CHECK_NRF(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret != NRF_SUCCESS && _ret != NRFX_SUCCESS) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _ret; \
            } \
            _ret; \
        })
