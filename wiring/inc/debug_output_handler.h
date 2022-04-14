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
#include "rtt_output_stream.h"

namespace spark {

class SerialLogHandler: public StreamLogHandler {
public:
    explicit SerialLogHandler(LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {}) :
            StreamLogHandler(Serial, level, filters) {
        Serial.begin();
        LogManager::instance()->addHandler(this);
    }

    explicit SerialLogHandler(int baud, LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {}) :
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
    explicit Serial1LogHandler(LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {}) :
            Serial1LogHandler(9600, level, filters) {
    }

    explicit Serial1LogHandler(int baud, LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {}) :
            StreamLogHandler(Serial1, level, filters) {
        Serial1.begin(baud);
        LogManager::instance()->addHandler(this);
    }

    virtual ~Serial1LogHandler() {
        LogManager::instance()->removeHandler(this);
        Serial1.end();
    }
};

#if Wiring_USBSerial1

class USBSerial1LogHandler: public StreamLogHandler {
public:
    explicit USBSerial1LogHandler(LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {}) :
            StreamLogHandler(USBSerial1, level, filters) {
        USBSerial1.begin();
        LogManager::instance()->addHandler(this);
    }

    explicit USBSerial1LogHandler(int baud, LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {}) :
            StreamLogHandler(USBSerial1, level, filters) {
        USBSerial1.begin(baud);
        LogManager::instance()->addHandler(this);
    }

    virtual ~USBSerial1LogHandler() {
        LogManager::instance()->removeHandler(this);
        USBSerial1.end();
    }
};

#endif // Wiring_USBSerial1

#if Wiring_Rtt

class RttLogHandler: public StreamLogHandler {
public:
    explicit RttLogHandler(LogLevel level = LOG_LEVEL_INFO, LogCategoryFilters filters = {}) :
            StreamLogHandler(strm_, level, filters) {
        if (strm_.open() == 0) {
            LogManager::instance()->addHandler(this);
        }
    }

    virtual ~RttLogHandler() {
        if (strm_.isOpen()) {
            LogManager::instance()->removeHandler(this);
            strm_.close();
        }
    }

private:
    particle::RttOutputStream strm_;
};

#endif // Wiring_Rtt

} // namespace spark

// Compatibility API
class SerialDebugOutput: public spark::SerialLogHandler {
public:
    explicit SerialDebugOutput(int baud = 9600, LogLevel level = LOG_LEVEL_ALL) :
        SerialLogHandler(level) {
    }
};

class Serial1DebugOutput: public spark::Serial1LogHandler {
public:
    explicit Serial1DebugOutput(int baud = 9600, LogLevel level = LOG_LEVEL_ALL) :
        Serial1LogHandler(baud, level) {
    }
};

#endif	/* DEBUG_OUTPUT_HANDLER_H */
