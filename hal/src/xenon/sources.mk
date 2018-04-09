
HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template
HAL_SRC_NRF52840_PATH = $(TARGET_HAL_PATH)/src/nRF52840
HAL_SRC_ARMV7_PATH = $(TARGET_HAL_PATH)/src/armv7
HAL_SRC_XENON_PATH = $(TARGET_HAL_PATH)/src/xenon

INCLUDE_DIRS += $(HAL_SRC_XENON_PATH)
INCLUDE_DIRS += $(HAL_SRC_NRF52840_PATH)
INCLUDE_DIRS += $(HAL_SRC_ARMV7_PATH)

CSRC += $(call here_files,$(HAL_SRC_XENON_PATH)/,*.c)
CPPSRC += $(call here_files,$(HAL_SRC_XENON_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_SRC_NRF52840_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_NRF52840_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_SRC_ARMV7_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_ARMV7_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_SRC_TEMPLATE_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_TEMPLATE_PATH)/,*.cpp)

# ASM source files included in this build.
ASRC +=

