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

#ifndef SERVICES_ATCLIENT_H
#define SERVICES_ATCLIENT_H

#include <cstdint>
#include <cstdarg>
#include <limits>
#include <unistd.h>

namespace particle {

// Forward declaration
class Stream;

namespace services {

namespace at {

const size_t ATCLIENT_BUFFER_SIZE = 1024;

class AtClientBase {
public:
    AtClientBase(Stream* stream);
    virtual ~AtClientBase();

    virtual int init() = 0;
    virtual int destroy() = 0;

    void reset();

    void setTimeout(unsigned int timeout);
    unsigned int getTimeout() const;

    int waitReady(unsigned int timeout);

    void setStream(Stream* stream);
    ::particle::Stream* getStream();

    enum class ResultCode {
        UNKNOWN      = -1,
        // ITU-T Rec. V.250: Table 1/V.250 – Result codes
        OK           = 0,
        CONNECT      = 1,
        RING         = 2,
        NO_CARRIER   = 3,
        ERROR        = 4,
        NO_DIALTONE  = 5,
        BUSY         = 6,
        NO_ANSWER    = 7,
        // As defined in 3GPP TS 27.007
        CME_ERROR    = 8,
        // As defined in 3GPP TS 27.005
        CMS_ERROR    = 9,
        INTERMEDIATE = 10
    };

    enum class State {
        NOT_READY,
        READY,
        COMMAND_SENT,
        // A well-known intermediate result code or a response conforming to
        // "5.7.2 Extended syntax result codes" with an optional value
        INTERMEDIATE_RESULT_CODE,
        // A generic response not conforming to "5.7.2 Extended syntax result codes"
        INFORMATION_TEXT,
        FINAL_RESULT_CODE,
        ERROR
    };

// protected:
    int run();

    int readLine(char* buf, size_t size, bool skipEmpty = false);
    ResultCode parseResultCode(const char* str, size_t len);
    bool isFinalResultCode(ResultCode code);

    int sendCommand(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

    int waitIntermediateResultCode();
    int waitInformationText();
    int waitFinalResultCode();

    const char* getInformationText() const;
    const char* getIntermediateResultCode() const;
    const char* getIntermediateResultCodeValue() const;
    ResultCode getFinalResultCode() const;

    const char* getTerminator() const;

    State getState() const;

    size_t stripNonPrintable(char* str, size_t len);
    int parseIntermediateResultCode(char* str, size_t len);
    int parseInformationText(char* str, size_t len);

    struct TimeoutOverride {
        TimeoutOverride(unsigned int timeout, AtClientBase* self) {
            this->self = self;
            this->timeout = self->getTimeout();
            self->setTimeout(timeout);
        }

        ~TimeoutOverride() {
            self->setTimeout(timeout);
        }

        AtClientBase* self;
        unsigned int timeout;
    };

private:
    void setState(State state);
    int waitState(State state);
    int writeCommand(const char* fmt, ...);
    int writeCommand(const char* fmt, va_list args);
    void resetData();

private:
    ::particle::Stream* stream_;

    State state_ = State::NOT_READY;

    // Intermediate result code, e.g. +CMDNAME (with an optional value)
    char* intermediateResultCode_ = nullptr;
    char* intermediateResultCodeValue_ = nullptr;
    // Full response up to final result code
    char* informationText_ = nullptr;
    ResultCode resultCode_ = ResultCode::UNKNOWN;

    unsigned int timeout_;

    char buffer_[ATCLIENT_BUFFER_SIZE] = {};

    static constexpr const char* resultCodes_[] = {
        "OK",
        "CONNECT",
        "RING",
        "NO_CARRIER",
        "ERROR",
        "NO_DIALTONE",
        "BUSY",
        "NO_ANSWER",
        "+CME ERROR:",
        "+CMS ERROR:"
    };

    const char* terminator_ = "\r\n";
};

class ArgonNcpAtClient: public AtClientBase {
public:
    ArgonNcpAtClient(Stream* stream);
    virtual ~ArgonNcpAtClient();

    int init();
    int destroy();

    int getVersion(char* buf, size_t bufSize);
    int getModuleVersion(uint16_t* module);
    int getMac(int idx, uint8_t* mac);
    int startMuxer();
    int connect();

    int startUpdate(size_t size);
    int finishUpdate();
private:
};

class BoronNcpAtClient: public AtClientBase {
public:
    BoronNcpAtClient(Stream* stream);
    virtual ~BoronNcpAtClient();

    int init();
    int destroy();

    int getImsi();
    int getCcid();
    int selectSim(bool external);
    int startMuxer();
    int registerNet();
    int isRegistered(bool w);

    int connect();
private:
};


} // at
} // services
} // particle

#endif /* SERVICES_ATCLIENT_H */
