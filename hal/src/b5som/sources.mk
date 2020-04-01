HAL_SRC_B5SOM_PATH = $(TARGET_HAL_PATH)/src/b5som

INCLUDE_DIRS += $(HAL_SRC_B5SOM_PATH)

CSRC += $(call target_files,$(HAL_SRC_B5SOM_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_B5SOM_PATH)/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/cellular/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/quectel/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/quectel/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../nRF52840/sources.mk

