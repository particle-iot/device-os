/**
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

#include "delay_hal.h"
#include "timer_hal.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <boost/date_time/posix_time/posix_time.hpp>
#include "boost_thread_wrap.h"

#pragma GCC diagnostic pop

void HAL_Delay_Milliseconds(uint32_t millis)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(millis));
}

void HAL_Delay_Microseconds(uint32_t micros)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(1));
}

