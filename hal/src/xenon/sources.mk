HAL_SRC_XENON_PATH = $(TARGET_HAL_PATH)/src/xenon

INCLUDE_DIRS += $(HAL_SRC_XENON_PATH)

include $(HAL_PLATFORM_SRC_PATH)/../nRF52840/sources.mk

CSRC += $(call target_files,$(HAL_SRC_XENON_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_XENON_PATH)/,*.cpp)