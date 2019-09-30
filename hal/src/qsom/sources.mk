HAL_SRC_QSOM_PATH = $(TARGET_HAL_PATH)/src/qsom

INCLUDE_DIRS += $(HAL_SRC_QSOM_PATH)

CSRC += $(call target_files,$(HAL_SRC_QSOM_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_QSOM_PATH)/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../nRF52840/sources.mk

