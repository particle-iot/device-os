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


#ifndef DEBUG_OUTPUT_HANDLER_H
#define	DEBUG_OUTPUT_HANDLER_H

#include "spark_wiring_usartserial.h"
#include "spark_wiring_usbserial.h"
#include "spark_wiring_logging.h"
#include "service_debug.h"
#include "delay_hal.h"

class SerialDebugOutput: public spark::FormattingLogger
{
public:
    explicit SerialDebugOutput(int baud = 9600, LoggerOutputLevel level = ALL_LEVEL, const CategoryFilters &filters = {}) :
            FormattingLogger(level, filters)
    {
        Serial.begin(baud);
        Logger::install(this);
    }

private:
    virtual void write(const char* data, size_t size) override
    {
        Serial.write((const uint8_t*)data, size);
    }
};

typedef SerialDebugOutput SerialLogger;

class Serial1DebugOutput: public spark::FormattingLogger
{
public:
    explicit Serial1DebugOutput(int baud = 9600, LoggerOutputLevel level = ALL_LEVEL, const CategoryFilters &filters = {}) :
            FormattingLogger(level, filters)
    {
        Serial1.begin(baud);
        Logger::install(this);
    }

private:
    virtual void write(const char* data, size_t size) override
    {
        Serial1.write((const uint8_t*)data, size);
    }
};

typedef Serial1DebugOutput Serial1Logger;


#endif	/* DEBUG_OUTPUT_HANDLER_H */
