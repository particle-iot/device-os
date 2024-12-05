PREBOOTLOADER_PART1_SRC_PATH = $(PREBOOTLOADER_PART1_MODULE_PATH)

CSRC += $(call target_files,$(PREBOOTLOADER_PART1_SRC_PATH)/,*.c)
CPPSRC += $(call target_files,$(PREBOOTLOADER_PART1_SRC_PATH)/,*.cpp)

CSRC += $(PREBOOTLOADER_PART1_SRC_PATH)/../shared/crc32_nolookup.c
CSRC += $(PROJECT_ROOT)/hal/src/rtl872x/flash_hal.c
CPPSRC += $(PROJECT_ROOT)/hal/src/rtl872x/exflash_hal.cpp
CPPSRC += $(PROJECT_ROOT)/hal/src/rtl872x/exflash_hal_lock.cpp
CSRC += $(PROJECT_ROOT)/hal/src/rtl872x/hal_irq_flag.c

CPPSRC += $(PROJECT_ROOT)/hal/shared/flash_common.cpp
CPPSRC += $(PROJECT_ROOT)/hal/src/rtl872x/km0_km4_ipc.cpp
CPPSRC += $(PROJECT_ROOT)/hal/src/rtl872x/pinmap_hal.cpp

CPPSRC += $(PROJECT_ROOT)/hal/src/$(PLATFORM_NAME)/pinmap_defines.cpp

CSRC += $(PROJECT_ROOT)/hal/src/portable/FreeRTOS/heap_4_lock.c

LDFLAGS += -T$(PREBOOTLOADER_PART1_SRC_PATH)/linker.ld
LINKER_DEPS += $(PREBOOTLOADER_PART1_SRC_PATH)/linker.ld
