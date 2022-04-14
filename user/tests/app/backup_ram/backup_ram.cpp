/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */
#include "application.h"

retained int app_backup = 10;
int app_ram = 10;

STARTUP(System.disableFeature(FEATURE_RETAINED_MEMORY));

void setup()
{
    Serial.begin(9600);
    while (!Serial.available()) Particle.process();

    if (int(&app_backup) < 0x40024000) {
        Serial.printlnf("ERROR: expected app_backup in backup memory, but was at %x", &app_backup);
    }

    if (int(&app_ram) >= 0x40024000) {
        Serial.printlnf("ERROR: expected app_ram in sram memory, but was at %x", &app_ram);
    }

    Serial.printlnf("app_backup(%x):%d, app_ram(%x):%d", &app_backup, app_backup, &app_ram, app_ram);
    app_backup++;
    app_ram++;
}
