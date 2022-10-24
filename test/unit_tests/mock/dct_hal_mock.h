#pragma once

#include <stdexcept>
#include <string>

#include <fakeit.hpp>

#include "dct_hal.h"

namespace particle::test {

class DctHal {
public:
    virtual ~DctHal() = default;

    virtual int readAppDataCopy(uint32_t offs, void* data, size_t size) = 0;
    virtual int writeAppData(uint32_t offs, std::string data) = 0;
};

class DctHalMock: public fakeit::Mock<DctHal> {
public:
    DctHalMock() {
        if (s_instance) {
            throw std::runtime_error("DctHalMock is already instantiated");
        }
        s_instance = this;
    }

    ~DctHalMock() {
        s_instance = nullptr;
    }

    static DctHalMock* instance() {
        return s_instance;
    }

private:
    static DctHalMock* s_instance;
};

} // namespace particle::test
