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

class SerialLogHandler: public StreamLogHandler {
public:
    explicit SerialLogHandler(int baud = 9600, LogLevel level = LOG_LEVEL_INFO, const Filters &filters = {}) :
            StreamLogHandler(Serial, level, filters) {
        Serial.begin(baud);
        LogManager::instance()->addHandler(this);
    }

    virtual ~SerialLogHandler() {
        LogManager::instance()->removeHandler(this);
        Serial.end();
    }
};

class Serial1LogHandler: public StreamLogHandler {
public:
    explicit Serial1LogHandler(int baud = 9600, LogLevel level = LOG_LEVEL_INFO, const Filters &filters = {}) :
            StreamLogHandler(Serial1, level, filters) {
        Serial1.begin(baud);
        LogManager::instance()->addHandler(this);
    }

    virtual ~Serial1LogHandler() {
        LogManager::instance()->removeHandler(this);
        Serial1.end();
    }
};

} // namespace spark

// Compatibility API
class SerialDebugOutput: public spark::SerialLogHandler {
public:
    explicit SerialDebugOutput(int baud = 9600, LogLevel level = LOG_LEVEL_ALL) :
        SerialLogHandler(baud, level) {
    }
};

class Serial1DebugOutput: public spark::Serial1LogHandler {
public:
    explicit Serial1DebugOutput(int baud = 9600, LogLevel level = LOG_LEVEL_ALL) :
        Serial1LogHandler(baud, level) {
    }
};

#endif	/* DEBUG_OUTPUT_HANDLER_H */
