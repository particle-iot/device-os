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

#include "system_error.h"
#include "logging.h"

#ifndef LOG_CHECKED_ERRORS
#define LOG_CHECKED_ERRORS 0
#endif

#if LOG_CHECKED_ERRORS
#define _LOG_CHECKED_ERROR(_expr, _ret) \
        LOG(ERROR, #_expr " failed: %d", (int)_ret)
#else
#define _LOG_CHECKED_ERROR(_expr, _ret)
#endif

#define CHECK_RETURN(_expr, _val) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _val; \
            } \
            _ret; \
        })

#define CHECK(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _ret; \
            } \
            _ret; \
        })

#define CHECK_TRUE(_expr, _ret) \
        do { \
            const bool _ok = (bool)(_expr); \
            if (!_ok) { \
                _LOG_CHECKED_ERROR(_expr, _ret); \
                return _ret; \
            } \
        } while (false)

#define CHECK_TRUE_RETURN(_expr, _val) \
        ({ \
            const auto _ret = _expr; \
            const bool _ok = (bool)(_ret); \
            if (!_ok) { \
                _LOG_CHECKED_ERROR(_expr, _val); \
                return _val; \
            } \
            _ret; \
        })
