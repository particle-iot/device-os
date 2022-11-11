MODULE=system-part1
SYSTEM_PART1_MODULE_PATH=.
PROJECT_ROOT = ../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = 0x30000

DEPENDENCIES = newlib_nano modules/b5som/user-part modules/b5som/system-part1 dynalib services hal platform system wiring communication rt-dynalib crypto proto_defs wiring_globals
LIB_DEPENDENCIES = services system wiring communication hal platform crypto proto_defs wiring_globals
# newlib_nano is special in that it's linked automatically by the system, so no need to add it to the library path here
MAKE_DEPENDENCIES = newlib_nano $(LIB_DEPENDENCIES)
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(LIB_DEPENDENCIES)
LIB_DEPS += $(SERVICES_LIB_DEP) $(PLATFORM_LIB_DEP) $(HAL_LIB_DEP) $(SYSTEM_LIB_DEP) $(COMMUNICATION_LIB_DEP) $(WIRING_LIB_DEP) $(CRYPTO_LIB_DEP) $(PROTO_DEFS_LIB_DEP) $(WIRING_GLOBALS_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size

include $(PROJECT_ROOT)/build/arm-tlm.mk

# NOTE: When compiling with LTO vTaskSwitchContext is getting linked out, despite the fact that it's
# marked as used.
LDFLAGS += -flto -Os -fuse-linker-plugin -Wl,--undefined=vTaskSwitchContext

$(call check_modular)


