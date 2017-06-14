MODULE=system-part3
SYSTEM_PART2_MODULE_PATH=.
PROJECT_ROOT = ../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = 0x8040000

DEPENDENCIES = newlib_nano modules/electron/user-part modules/electron/system-part1 modules/electron/system-part2 dynalib services services-dynalib hal platform system wiring crypto-dynalib crypto communication-dynalib communication rt-dynalib wiring_globals
LIB_DEPENDENCIES = services-dynalib system wiring crypto-dynalib crypto communication-dynalib hal platform wiring_globals
# newlib_nano is special in that it's linked automatically by the system, so no need to add it to the library path here
MAKE_DEPENDENCIES = newlib_nano $(LIB_DEPENDENCIES)

# exclude hal_cellular and hal_usb
GLOBAL_DEFINES += HAL_CELLULAR_EXCLUDE=1 HAL_USB_EXCLUDE=1 HAL_BOOTLOADER_EXCLUDE=1 MODULE_HAS_SYSTEM_PART3=1


include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(LIB_DEPENDENCIES)
LIB_DEPS += $(SERVICES_DYNALIB_LIB_DEP) $(PLATFORM_LIB_DEP) $(HAL_LIB_DEP) $(SYSTEM_LIB_DEP) $(CRYPTO_DYNALIB_LIB_DEP) $(CRYPTO_LIB_DEP) $(COMMUNICATION_DYNALIB_LIB_DEP) $(WIRING_LIB_DEP) $(WIRING_GLOBALS_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size


include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)


