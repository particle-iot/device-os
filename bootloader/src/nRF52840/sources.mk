BOOTLOADER_SRC_PATH = $(BOOTLOADER_MODULE_PATH)/src/nRF52840

CSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.c)
CPPSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.cpp)

LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_nrf52840_bootloader.ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/linker_nrf52840_bootloader.ld
