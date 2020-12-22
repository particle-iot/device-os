HAL_SRC_TRACKER_PATH = $(TARGET_HAL_PATH)/src/tracker

INCLUDE_DIRS += $(HAL_SRC_TRACKER_PATH)

CSRC += $(call target_files,$(HAL_SRC_TRACKER_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_TRACKER_PATH)/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/cellular/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/quectel/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/quectel/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/esp32/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/esp32/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/wifi/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/wifi/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/esp32/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/esp32/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../nRF52840/sources.mk

