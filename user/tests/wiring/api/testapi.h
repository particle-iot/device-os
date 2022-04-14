/**
 ******************************************************************************
 * @file    testapi.h
 * @authors Matthew McGowan
 * @date    13 January 2015
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

#ifndef TESTAPI_H
#define	TESTAPI_H

#include "application.h"
#include "unit-test/unit-test.h"

extern bool runthisstuff;

// compile, but do not run
#define API_COMPILE(x) if (!runthisstuff) { x; }

#define API_NO_COMPILE(x)

#endif	/* TESTAPI_H */

