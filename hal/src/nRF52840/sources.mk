
HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template
HAL_SRC_NRF52840_PATH = $(TARGET_HAL_PATH)/src/nRF52840
HAL_SRC_ARMV7_PATH = $(TARGET_HAL_PATH)/src/armv7

INCLUDE_DIRS += $(HAL_SRC_NRF52840_PATH)
INCLUDE_DIRS += $(HAL_SRC_ARMV7_PATH)

CSRC += $(call target_files,$(HAL_SRC_NRF52840_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_NRF52840_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_SRC_ARMV7_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_ARMV7_PATH)/,*.cpp)

# CSRC += $(call target_files,$(HAL_SRC_TEMPLATE_PATH)/,*.c)
# CPPSRC += $(call target_files,$(HAL_SRC_TEMPLATE_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_MODULE_PATH)/network/api/,*.c)
CPPSRC += $(call target_files,$(HAL_MODULE_PATH)/network/api/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/api/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/api/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/wiznet/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/wiznet/,*.cpp)

CSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/openthread/,*.c)
CPPSRC += $(call here_files,$(HAL_MODULE_PATH)/network/lwip/openthread/,*.cpp)

CSRC += $(call target_files,$(HAL_MODULE_PATH)/network/openthread/,*.c)
CPPSRC += $(call target_files,$(HAL_MODULE_PATH)/network/openthread/,*.cpp)

CPPSRC += $(call target_files,$(HAL_MODULE_PATH)/network/ncp/,*.cpp)

CSRC += $(TARGET_HAL_PATH)/src/portable/FreeRTOS/heap_4_lock.c

# ASM source files included in this build.
ASRC +=
