#include "dct_hal.h"
#include "dct_hal_mock.h"

using namespace particle::test;

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size) {
    auto mock = DctHalMock::instance();
    if (mock) {
        return mock->get().readAppDataCopy(offset, ptr, size);
    }
    return 0;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    auto mock = DctHalMock::instance();
    if (mock) {
        return mock->get().writeAppData(offset, std::string((const char*)data, size));
    }
    return 0;
}
