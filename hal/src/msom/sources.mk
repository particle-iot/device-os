HAL_SRC_MSOM_PATH = $(TARGET_HAL_PATH)/src/msom

INCLUDE_DIRS += $(HAL_SRC_MSOM_PATH)

CSRC += $(call target_files,$(HAL_SRC_MSOM_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_MSOM_PATH)/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/cellular/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/cellular/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/cellular/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/quectel/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/quectel/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/wifi/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp/wifi/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/realtek/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/realtek/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/realtek/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/ncp_client/realtek/,*.cpp)

include $(HAL_PLATFORM_SRC_PATH)/../rtl872x/sources.mk

