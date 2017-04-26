#include "system_flags_impl.h"

#include "stubs/stm32f2xx_rtc.h"

#include "tools/catch.h"
#include "hippomocks.h"

#include <map>

namespace {

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

platform_system_flags loadSystemFlags() {
    platform_system_flags flags;
    std::memset(&flags, 0, sizeof(flags));
    Load_SystemFlags_Impl(&flags);
    return flags;
}

void saveSystemFlags(const platform_system_flags& flags) {
    Save_SystemFlags_Impl(&flags);
}

template<typename T>
void checkSystemFlagValue(T platform_system_flags::*flag, T val) {
    const uint8_t fill = (uint8_t)~val; // Define a fill byte
    platform_system_flags f1;
    std::memset(&f1, fill, sizeof(f1));
    saveSystemFlags(f1);
    f1 = loadSystemFlags(); // Initialize header fields
    platform_system_flags f2;
    std::memcpy(&f2, &f1, sizeof(platform_system_flags));
    f2.*flag = val; // Set flag value
    saveSystemFlags(f2);
    f2 = loadSystemFlags();
    REQUIRE((f2.*flag) == val); // Check flag value
    std::memset(&(f2.*flag), fill, sizeof(T)); // Reset flag field
    REQUIRE((std::memcmp(&f1, &f2, sizeof(platform_system_flags)) == 0));
}

template<typename T>
void checkSystemFlag(T platform_system_flags::*flag) {
    const T minVal = std::numeric_limits<T>::min();
    checkSystemFlagValue(flag, minVal);
    const T maxVal = std::numeric_limits<T>::max();
    checkSystemFlagValue(flag, maxVal);
    const T avgVal = (minVal + maxVal) / 2;
    checkSystemFlagValue(flag, avgVal);
}

const uint32_t SYSTEM_FLAGS_MAGIC_NUMBER = 0x1adeacc0;

} // namespace

TEST_CASE("System flags") {
    RtcMocks mocks;
    SECTION("default flag values") {
        auto f = loadSystemFlags();
        CHECK(f.header[0] == 0xffff);
        CHECK(f.header[1] == 0xffff);
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
    SECTION("header contains correct values when flags are initialized") {
        platform_system_flags f;
        f.header[0] = 0;
        f.header[1] = 0;
        saveSystemFlags(f);
        f = loadSystemFlags();
        CHECK(f.header[0] == (SYSTEM_FLAGS_MAGIC_NUMBER & 0xffff));
        CHECK(f.header[1] == ((SYSTEM_FLAGS_MAGIC_NUMBER >> 16) & 0xffff));
    }
    SECTION("changing of a flag value doesn't affect other flags") {
        checkSystemFlag(&platform_system_flags::Bootloader_Version_SysFlag);
        checkSystemFlag(&platform_system_flags::NVMEM_SPARK_Reset_SysFlag);
        checkSystemFlag(&platform_system_flags::FLASH_OTA_Update_SysFlag);
        checkSystemFlag(&platform_system_flags::OTA_FLASHED_Status_SysFlag);
        checkSystemFlag(&platform_system_flags::Factory_Reset_SysFlag);
        checkSystemFlag(&platform_system_flags::IWDG_Enable_SysFlag);
        checkSystemFlag(&platform_system_flags::dfu_on_no_firmware);
        checkSystemFlag(&platform_system_flags::Factory_Reset_Done_SysFlag);
        checkSystemFlag(&platform_system_flags::StartupMode_SysFlag);
        checkSystemFlag(&platform_system_flags::FeaturesEnabled_SysFlag);
        checkSystemFlag(&platform_system_flags::RCC_CSR_SysFlag);
    }
}
