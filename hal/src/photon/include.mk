# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_COREV2_PATH = $(TARGET_HAL_PATH)/src/photon
HAL_INCL_STM32F2XX_PATH = $(TARGET_HAL_PATH)/src/stm32f2xx
HAL_INCL_STM32_PATH = $(TARGET_HAL_PATH)/src/stm32

#HAL_WICED_RTOS=ThreadX
#HAL_WICED_NETWORK=NetX
HAL_WICED_RTOS=FreeRTOS
HAL_WICED_NETWORK=LwIP


INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)
INCLUDE_DIRS += $(HAL_INCL_STM32F2XX_PATH)
INCLUDE_DIRS += $(HAL_INCL_STM32_PATH)

# implementation defined details for the platform that can vary
INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)/api

HAL_LIB_COREV2 = $(HAL_SRC_COREV2_PATH)/lib

HAL_WICED_COMMON_LIBS = Lib_SPI_Flash_Library_$(PLATFORM_NET) Lib_HTTP_Server Lib_Wiced_RO_FS Lib_base64 Lib_Ring_Buffer Lib_TLV STM32F2xx_Peripheral_Libraries
HAL_WICED_COMMON_LIBS += Lib_Linked_List

HAL_SHOULD_BE_COMMON = WICED Platform_$(PLATFORM_NET) Lib_DHCP_Server Lib_DNS Lib_DNS_Redirect_Daemon STM32F2xx STM32F2xx_Peripheral_Drivers
HAL_LIB_RTOS = $(HAL_LIB_COREV2)/$(HAL_WICED_RTOS)
ifeq "$(HAL_WICED_RTOS)" "FreeRTOS"
HAL_WICED_RTOS_LIBS = $(HAL_SHOULD_BE_COMMON) FreeRTOS LwIP WWD_FreeRTOS_Interface_$(PLATFORM_NET) WICED_FreeRTOS_Interface WWD_LwIP_Interface_FreeRTOS WICED_LwIP_Interface WWD_for_SDIO_FreeRTOS Wiced_Network_LwIP_FreeRTOS
else
HAL_WICED_RTOS_LIBS = $(HAL_SHOULD_BE_COMMON) ThreadX.ARM_CM3.release NetX WWD_NetX_Interface WICED_ThreadX_Interface WWD_for_SDIO_ThreadX WICED_NetX_Interface WWD_ThreadX_Interface NetX.ARM_CM3.release
endif

HAL_WICED_LIB_FILES += $(addprefix $(HAL_LIB_COREV2)/,$(addsuffix .a,$(HAL_WICED_COMMON_LIBS)))
HAL_WICED_LIB_FILES += $(addprefix $(HAL_LIB_RTOS)/,$(addsuffix .a,$(HAL_WICED_RTOS_LIBS)))
WICED_MCU = $(HAL_SRC_COREV2_PATH)/wiced/platform/MCU/STM32F2xx/GCC

INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)/include $(HAL_SRC_COREV2_PATH)/wiced/security/BESL/host/WICED/ $(HAL_SRC_COREV2_PATH)/wiced/security/BESL/include $(HAL_SRC_COREV2_PATH)/wiced/security/BESL $(HAL_SRC_COREV2_PATH)/wiced/security/BESL/crypto $(HAL_SRC_COREV2_PATH)/wiced/WWD/include/ $(HAL_SRC_COREV2_PATH)/wiced/platform/include/ $(HAL_SRC_COREV2_PATH)/wiced/platform/GCC/ $(HAL_SRC_COREV2_PATH)/wiced/security/BESL/supplicant/
INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)/libraries/crypto
INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)/libraries/daemons/DNS_redirect
HAL_WICED_LIB_FILES += $(HAL_SRC_COREV2_PATH)/lib/Supplicant_BESL.a
HAL_WICED_LIB_FILES += $(HAL_SRC_COREV2_PATH)/lib/BESL.ARM_CM3.release.a

HAL_LINK ?= $(findstring hal,$(MAKE_DEPENDENCIES))

# if hal is used as a make dependency (linked) then add linker commands
ifneq (,$(HAL_LINK))
LINKER_FILE=$(WICED_MCU)/app_no_bootloader.ld
#HAL_WICED_LIB_FILES += $(HAL_SRC_COREV2_PATH)/lib/Lib_crypto_open.a
LINKER_DEPS=$(LINKER_FILE) $(HAL_WICED_LIB_FILES)

# use our version of newlib nano
LINKER_DEPS += $(NEWLIB_TWEAK_SPECS)
LDFLAGS += --specs=nano.specs --specs=$(NEWLIB_TWEAK_SPECS)
LDFLAGS += -Wl,--whole-archive $(HAL_WICED_LIB_FILES) -Wl,--no-whole-archive
LDFLAGS += -T$(LINKER_FILE)
LDFLAGS += -L$(COMMON_BUILD)/arm/linker/stm32f2xx
LDFLAGS += -L$(WICED_MCU)/STM32F2x5
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400
USE_PRINTF_FLOAT ?= n
ifeq ("$(USE_PRINTF_FLOAT)","y")
LDFLAGS += -u _printf_float
endif
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map
LDFLAGS += -u uxTopUsedPriority

# used the -v flag to get gcc to output the commands it passes to the linker when --specs=nano.specs is provided
# LDFLAGS += -lstdc++ -lg -lc -lm -Wl,--start-group  -lstdc++ -lg -lc -lm -Wl,--end-group -Wl,--start-group $(LIBG_TWEAK) -lstdc++ -lg -lc -lm -Wl,--end-group -lg_nano

endif

# not using assembler startup script, but will use startup linked in with wiced


