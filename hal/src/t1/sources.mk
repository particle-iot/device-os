HAL_SRC_T1_PATH = $(TARGET_HAL_PATH)/src/t1

INCLUDE_DIRS += $(HAL_SRC_T1_PATH)

CSRC += $(call target_files,$(HAL_SRC_T1_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_T1_PATH)/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../rtl872x/sources.mk

