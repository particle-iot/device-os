PREBOOTLOADER_SRC_PATH = $(PREBOOTLOADER_MODULE_PATH)/src/tron

CSRC += $(call target_files,$(PREBOOTLOADER_SRC_PATH)/,*.c)
CPPSRC += $(call target_files,$(PREBOOTLOADER_SRC_PATH)/,*.cpp)

LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_rtl872x_prebootloader.ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/linker_rtl872x_prebootloader.ld
