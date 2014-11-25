BOOTLOADER_SRC_COREV2_PATH = $(BOOTLOADER_MODULE_PATH)/src/core-v2

CSRC += $(call target_files,$(BOOTLOADER_SRC_COREV2_PATH)/,*.c)


HAL_LIB_COREV2 = $(HAL_SRC_COREV2_PATH)/lib
HAL_WICED_LIBS += Platform_$(USI_PART_NO) SPI_Flash_Library_$(USI_PART_NO) STM32F2xx

HAL_WICED_LIB_FILES += $(addprefix $(HAL_LIB_COREV2)/,$(addsuffix .a,$(HAL_WICED_LIBS)))

LIBS_EXT += -Wl,--whole-archive $(HAL_WICED_LIB_FILES) -Wl,--no-whole-archive

