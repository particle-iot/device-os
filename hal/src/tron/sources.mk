HAL_SRC_TRON_PATH = $(TARGET_HAL_PATH)/src/tron

INCLUDE_DIRS += $(HAL_SRC_TRON_PATH)

CSRC += $(call target_files,$(HAL_SRC_TRON_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_TRON_PATH)/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/wifi/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/wifi/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../rtl872x/sources.mk

