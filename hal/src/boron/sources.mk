HAL_SRC_BORON_PATH = $(TARGET_HAL_PATH)/src/boron

INCLUDE_DIRS += $(HAL_SRC_BORON_PATH)

CSRC += $(call target_files,$(HAL_SRC_BORON_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_BORON_PATH)/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/ublox/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/ublox/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../nRF52840/sources.mk

