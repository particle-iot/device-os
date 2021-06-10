/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include <string.h>
#include <basic_types.h>
#include "dlist.h"

// --------------------------------------------
//	Platform dependent type define
// --------------------------------------------
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(s,field)	((SSIZE_T)&((s*)(0))->field)
#endif

// os types
typedef char			    osdepCHAR;
typedef float			    osdepFLOAT;
typedef double			    osdepDOUBLE;
typedef long			    osdepLONG;
typedef short			    osdepSHORT;
typedef unsigned long	    osdepSTACK_TYPE;
typedef long			    osdepBASE_TYPE;
typedef unsigned long	    osdepTickType;

typedef void*	            _timerHandle;
typedef void*	            _sema;
typedef void*	            _mutex;
typedef void*	            _lock;
typedef void*	            _queueHandle;
typedef void*	            _xqueue;
typedef void*           	_timer;

#define rtw_timer_list void*

typedef	void*       		_pkt;
typedef unsigned char		_buffer;
typedef unsigned int        systime;

typedef void*			    _thread_hdl_;
typedef void			    thread_return;
typedef void*			    thread_context;

