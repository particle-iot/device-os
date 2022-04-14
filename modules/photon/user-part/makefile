MODULE=user-part
USER_PART_MODULE_PATH=.
PROJECT_ROOT=../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = $(USER_FIRMWARE_IMAGE_LOCATION)
PLATFORM_DFU_LEAVE = y
# disable LTO build on 2015q2 - see issue #471
export COMPILE_LTO=n

# communication used only for header declarations
DEPENDENCIES = user dynalib services services-dynalib hal hal-dynalib system system-dynalib rt-dynalib wiring communication-dynalib modules/photon/system-part1 modules/photon/system-part2 platform wiring_globals
MAKE_DEPENDENCIES = user hal-dynalib services-dynalib system-dynalib rt-dynalib wiring communication-dynalib platform wiring_globals
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(USER_LIB_DEP) $(SERVICES_DYNALIB_LIB_DEP) $(HAL_DYNALIB_LIB_DEP) $(SYSTEM_DYNALIB_LIB_DEP) $(RT_DYNALIB_LIB_DEP) $(WIRING_LIB_DEP) $(COMMUNICATION_DYNALIB_LIB_DEP) $(PLATFORM_LIB_DEP) $(WIRING_GLOBALS_LIB_DEP)
LIB_DEPS += $(PROJECT_ROOT)/hal/src/photon/lib/STM32F2xx_Peripheral_Libraries.a
LIB_DIRS += $(dir $(LIB_DEPS))

LDFLAGS += -Wl,--whole-archive $(PROJECT_ROOT)/hal/src/photon/lib/STM32F2xx_Peripheral_Libraries.a -Wl,--no-whole-archive


TARGET=elf bin lst hex size


include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)


