
HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template
HAL_SRC_STM32F2XX_PATH = $(TARGET_HAL_PATH)/src/stm32f2xx
HAL_SRC_STM32_PATH = $(TARGET_HAL_PATH)/src/stm32
HAL_SRC_ELECTRON_PATH = $(TARGET_HAL_PATH)/src/electron

INCLUDE_DIRS += $(HAL_SRC_STM32F2XX_PATH)
INCLUDE_DIRS += $(HAL_SRC_STM32_PATH)
INCLUDE_DIRS += $(HAL_SRC_ELECTRON_PATH)

CSRC += $(call here_files,$(HAL_SRC_ELECTRON_PATH)/,*.c)
CPPSRC += $(call here_files,$(HAL_SRC_ELECTRON_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_SRC_STM32F2XX_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_STM32F2XX_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_SRC_STM32_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_STM32_PATH)/,*.cpp)
# ASM source files included in this build.
ASRC +=

CPPSRC += $(call target_files,$(HAL_SRC_ELECTRON_PATH)/modem/,*.cpp)

# use malloc/free
CSRC += $(TARGET_HAL_PATH)/src/portable/FreeRTOS/heap_4_lock.c
