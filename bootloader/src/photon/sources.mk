BOOTLOADER_SRC_COREV2_PATH = $(BOOTLOADER_MODULE_PATH)/src/photon
include $(BOOTLOADER_MODULE_PATH)/src/stm32f2xx/sources.mk

CSRC += $(call target_files,$(BOOTLOADER_SRC_COREV2_PATH)/,*.c)

HAL_LIB_COREV2 = $(HAL_SRC_COREV2_PATH)/lib
WICED_LIBS = Platform_$(PLATFORM_NET) SPI_Flash_Library_$(PLATFORM_NET)

WICED_LIB_FILES = $(addprefix $(HAL_LIB_COREV2)/,$(addsuffix .a,$(WICED_LIBS)))
WICED_LIB_FILES = $(HAL_LIB_COREV2)/FreeRTOS/STM32F2xx_bootloader.a

LIBS_EXT += -Wl,--whole-archive $(WICED_LIB_FILES) -Wl,--no-whole-archive

LDFLAGS += -L$(PROJECT_ROOT)/modules/photon/user-part
LINKER_DEPS += $(PROJECT_ROOT)/modules/photon/user-part/module_user_memory.ld
