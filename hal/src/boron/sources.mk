HAL_SRC_BORON_PATH = $(TARGET_HAL_PATH)/src/boron

INCLUDE_DIRS += $(HAL_SRC_BORON_PATH)

CSRC += $(call target_files,$(HAL_SRC_BORON_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_BORON_PATH)/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/cellular/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/sara/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/sara/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../nRF52840/sources.mk

