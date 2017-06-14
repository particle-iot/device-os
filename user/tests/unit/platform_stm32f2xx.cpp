#include "system_flags_impl.h"

#include "stubs/stm32f2xx_rtc.h"

#include "tools/catch.h"
#include "hippomocks.h"

#include <map>

namespace {

const uint32_t SYSTEM_FLAGS_MAGIC_NUMBER = 0x1adeacc0;

// Class mocking STM32's RTC functions
class RtcMocks {
public:
    RtcMocks() {
        // RTC_ReadBackupRegister()
        mocks_.OnCallFunc(RTC_ReadBackupRegister).Do([&](uint32_t reg) -> uint32_t {
            const auto it = bkpRegs_.find(reg);
            if (it == bkpRegs_.end()) {
                return 0;
            }
            return it->second;
        });
        // RTC_WriteBackupRegister()
        mocks_.OnCallFunc(RTC_WriteBackupRegister).Do([&](uint32_t reg, uint32_t val) {
            bkpRegs_[reg] = val;
        });
    }

private:
    MockRepository mocks_;
    std::map<uint32_t, uint32_t> bkpRegs_; // Backup registers
};

template<typename T>
void checkSystemFlagValue(T platform_system_flags_t::*flag, T val) {
    const uint8_t fill = (uint8_t)~val; // Define a fill byte
    platform_system_flags_t f1;
    std::memset(&f1, fill, sizeof(f1));
    f1.header = SYSTEM_FLAGS_MAGIC_NUMBER; // Initialize header data
    REQUIRE(Save_SystemFlags_Impl(&f1) == 0);
    platform_system_flags_t f2;
    std::memcpy(&f2, &f1, sizeof(platform_system_flags_t));
    f2.*flag = val; // Set flag value
    REQUIRE(Save_SystemFlags_Impl(&f2) == 0);
    std::memset(&f2, fill, sizeof(f2));
    REQUIRE(Load_SystemFlags_Impl(&f2) == 0);
    REQUIRE((f2.*flag) == val); // Check flag value
    std::memset(&(f2.*flag), fill, sizeof(T)); // Reset flag field
    REQUIRE((std::memcmp(&f1, &f2, sizeof(platform_system_flags_t)) == 0));
}

template<typename T>
void checkSystemFlag(T platform_system_flags_t::*flag) {
    const T minVal = std::numeric_limits<T>::min();
    checkSystemFlagValue(flag, minVal);
    const T maxVal = std::numeric_limits<T>::max();
    checkSystemFlagValue(flag, maxVal);
    const T avgVal = (minVal + maxVal) / 2;
    checkSystemFlagValue(flag, avgVal);
}

} // namespace

TEST_CASE("System flags") {
    RtcMocks mocks;
    SECTION("default flag values") {
        platform_system_flags_t f = { 0 };
        CHECK(Load_SystemFlags_Impl(&f) != 0);
        CHECK(f.header == SYSTEM_FLAGS_MAGIC_NUMBER);
        CHECK(f.Bootloader_Version_SysFlag == 0xffff);
        CHECK(f.NVMEM_SPARK_Reset_SysFlag == 0xffff);
        CHECK(f.FLASH_OTA_Update_SysFlag == 0xffff);
        CHECK(f.OTA_FLASHED_Status_SysFlag == 0xffff);
        CHECK(f.Factory_Reset_SysFlag == 0xffff);
        CHECK(f.IWDG_Enable_SysFlag == 0xffff);
        CHECK(f.dfu_on_no_firmware == 0xff);
        CHECK(f.Factory_Reset_Done_SysFlag == 0xff);
        CHECK(f.StartupMode_SysFlag == 0xff);
        CHECK(f.FeaturesEnabled_SysFlag == 0xff);
        CHECK(f.RCC_CSR_SysFlag == 0xffffffff);
    }
    SECTION("flags cannot be updated with uninitialized data") {
        platform_system_flags_t f = { 0 };
        CHECK(Save_SystemFlags_Impl(&f) != 0); // Invalid header field
        f.header = SYSTEM_FLAGS_MAGIC_NUMBER;
        CHECK(Save_SystemFlags_Impl(&f) == 0);
    }
    SECTION("changing individual flag values") {
        checkSystemFlag(&platform_system_flags_t::Bootloader_Version_SysFlag);
        checkSystemFlag(&platform_system_flags_t::NVMEM_SPARK_Reset_SysFlag);
        checkSystemFlag(&platform_system_flags_t::FLASH_OTA_Update_SysFlag);
        checkSystemFlag(&platform_system_flags_t::OTA_FLASHED_Status_SysFlag);
        checkSystemFlag(&platform_system_flags_t::Factory_Reset_SysFlag);
        checkSystemFlag(&platform_system_flags_t::IWDG_Enable_SysFlag);
        checkSystemFlag(&platform_system_flags_t::dfu_on_no_firmware);
        checkSystemFlag(&platform_system_flags_t::Factory_Reset_Done_SysFlag);
        checkSystemFlag(&platform_system_flags_t::StartupMode_SysFlag);
        checkSystemFlag(&platform_system_flags_t::FeaturesEnabled_SysFlag);
        checkSystemFlag(&platform_system_flags_t::RCC_CSR_SysFlag);
    }
}
