HAL_SRC_ARGON_PATH = $(TARGET_HAL_PATH)/src/argon

INCLUDE_DIRS += $(HAL_SRC_ARGON_PATH)

CSRC += $(call target_files,$(HAL_SRC_ARGON_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_ARGON_PATH)/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/esp32/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/esp32/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../nRF52840/sources.mk

