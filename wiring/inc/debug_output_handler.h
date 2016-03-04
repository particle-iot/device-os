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

namespace spark {

class SerialLogger: public Logger {
public:
    explicit SerialLogger(int baud = 9600, LogLevel level = ALL_LEVEL, const CategoryFilters &filters = {}) :
            Logger(level, filters) {
        Serial.begin(baud);
        Logger::install(this);
    }

    virtual ~SerialLogger() {
        Logger::uninstall(this);
    }

protected:
    virtual void write(const char* data, size_t size) override { // spark::Logger
        Serial.write((const uint8_t*)data, size);
    }
};

class Serial1Logger: public Logger {
public:
    explicit Serial1Logger(int baud = 9600, LogLevel level = ALL_LEVEL, const CategoryFilters &filters = {}) :
            Logger(level, filters) {
        Serial1.begin(baud);
        Logger::install(this);
    }

    virtual ~Serial1Logger() {
        Logger::uninstall(this);
    }

protected:
    virtual void write(const char* data, size_t size) override { // spark::Logger
        Serial1.write((const uint8_t*)data, size);
    }
};

} // namespace spark

// Compatibility typedefs
typedef spark::SerialLogger SerialDebugOutput;
typedef spark::Serial1Logger Serial1DebugOutput;

#endif	/* DEBUG_OUTPUT_HANDLER_H */
