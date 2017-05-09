/**
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "application.h"
#include "unit-test/unit-test.h"

test(WIRE_01_Wire_end_begin_end)
{
    Wire.end();
    assertTrue(pinAvailable(D0));
    assertTrue(pinAvailable(D1));

    Wire.begin();
    assertFalse(pinAvailable(D0));
    assertFalse(pinAvailable(D1));

    Wire.end();
    assertTrue(pinAvailable(D0));
    assertTrue(pinAvailable(D1));
}

#if PLATFORM_ID == 10
test(WIRE_02_Wire_and_Wire1_cannot_be_enabled_simultaneously_on_Electron)
{
    // Just in case disable both of them beforehand to enter a know state
    Wire.end();
    Wire1.end();

    Wire.begin();
    assertTrue(Wire.isEnabled());
    Wire1.begin();
    assertFalse(Wire1.isEnabled());

    Wire.end();
    assertFalse(Wire.isEnabled());

    Wire1.begin();
    assertTrue(Wire1.isEnabled());
    Wire.begin();
    assertFalse(Wire.isEnabled());

    Wire1.end();
    assertFalse(Wire1.isEnabled());
}
#endif // PLATFORM_ID == 10
