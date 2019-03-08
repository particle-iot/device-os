MODULE=system-part1
SYSTEM_PART1_MODULE_PATH=.
PROJECT_ROOT=../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = 0x8020000

# HAL Dependency only for the WICED wifi nvmem resource
# Platform dependency is for the LED implementation used by rgbled.c in services.
MAKE_DEPENDENCIES = communication hal-dynalib services platform hal crypto third_party/miniz
DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib rt-dynalib modules/photon/system-part2
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

# dependent on hal lib only for the wwd_nvmem_resource, which is managed specifically in build.mk
LIBS += $(notdir $(filter-out hal,$(MAKE_DEPENDENCIES)))
LIB_DEPS += $(COMMUNICATION_LIB_DEP) $(HAL_DYNALIB_LIB_DEP) $(SERVICES_LIB_DEP) $(PLATFORM_LIB_DEP) $(CRYPTO_LIB_DEP) $(MINIZ_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size

include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)


