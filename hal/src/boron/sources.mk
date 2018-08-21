HAL_SRC_ARGON_PATH = $(TARGET_HAL_PATH)/src/argon

INCLUDE_DIRS += $(HAL_SRC_ARGON_PATH)

CSRC += $(call target_files,$(HAL_SRC_ARGON_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_ARGON_PATH)/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../nRF52840/sources.mk

