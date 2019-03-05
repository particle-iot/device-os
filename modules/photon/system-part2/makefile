MODULE=system-part2
SYSTEM_PART2_MODULE_PATH=.
PROJECT_ROOT = ../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = 0x8060000

DEPENDENCIES = newlib_nano modules/photon/user-part modules/photon/system-part1 dynalib services services-dynalib hal platform system wiring communication-dynalib communication rt-dynalib crypto-dynalib crypto
LIB_DEPENDENCIES = services-dynalib system wiring communication-dynalib hal platform crypto-dynalib crypto
# newlib_nano is special in that it's linked automatically by the system, so no need to add it to the library path here
MAKE_DEPENDENCIES = newlib_nano $(LIB_DEPENDENCIES)
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(notdir $(LIB_DEPENDENCIES))
LIB_DEPS += $(SERVICES_DYNALIB_LIB_DEP) $(PLATFORM_LIB_DEP) $(HAL_LIB_DEP) $(SYSTEM_LIB_DEP) $(COMMUNICATION_DYNALIB_LIB_DEP) $(WIRING_LIB_DEP) $(CRYPTO_DYNALIB_LIB_DEP) $(CRYPTO_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size


include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)


