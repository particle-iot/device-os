#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>

namespace particle::test {

inline void dump(const void* data, size_t size) {
    std::ostringstream s;
    s << std::hex << std::setfill('0');
    auto d = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        s << std::setw(2) << (int)d[i];
    }
    std::cerr << "*******************************************************************************" << std::endl;
    std::cerr << "dump(): " << s.str() << " (" << size << " bytes)" << std::endl;
}

} // namespace particle::test
