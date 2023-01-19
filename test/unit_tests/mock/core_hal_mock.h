#pragma once

#include <stdexcept>

#include <fakeit.hpp>

#include "core_hal.h"

namespace particle::test {

class CoreHal {
public:
    virtual ~CoreHal() = 0;

    virtual bool featureGet(HAL_Feature feature) = 0;
};

class CoreHalMock: public fakeit::Mock<CoreHal> {
public:
    CoreHalMock() {
        if (s_instance) {
            throw std::runtime_error("CoreHalMock is already instantiated");
        }
        s_instance = this;
    }

    ~CoreHalMock() {
        s_instance = nullptr;
    }

    static CoreHalMock* instance() {
        return s_instance;
    }

private:
    static CoreHalMock* s_instance;
};

} // namespace particle::test
