# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_COREV2_PATH = $(TARGET_HAL_PATH)/src/core-v2

# if we are being compiled with platform as a dependency, then also include
# implementation headers.
ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)
endif

ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))
# if hal is used as a make dependency (linked) then add linker commands

ifeq ("$(USE_WICED_SDK)","1")

HAL_LIB_COREV2 = $(HAL_SRC_COREV2_PATH)/lib
HAL_WICED_LIBS += common_GCC Platform_BCM9WCDUSI09 FreeRTOS LwIP WICED SPI_Flash_Library_BCM9WCDUSI09 WWD_FreeRTOS_Interface_BCM9WCDUSI09 WICED_FreeRTOS_Interface WWD_LwIP_Interface_FreeRTOS WICED_LwIP_Interface Supplicant_BESL Lib_HTTP_Server Lib_DNS_Redirect_Daemon Lib_DNS WWD_for_SDIO_FreeRTOS Lib_Wiced_RO_FS STM32F2xx Wiced_Network_LwIP_FreeRTOS Lib_DHCP_Server Lib_base64 STM32F2xx_Peripheral_Drivers Ring_Buffer STM32F2xx_Peripheral_Libraries

HAL_WICED_LIB_FILES += $(addprefix $(HAL_LIB_COREV2)/,$(addsuffix .a,$(HAL_WICED_LIBS)))
WICED_MCU = $(HAL_SRC_COREV2_PATH)/wiced/platform/MCU/STM32F2xx/GCC

# LDFLAGS += -Wl,--start-group $(HAL_WICED_LIB_FILES) -Wl,--end-group )
LDFLAGS += --specs=nano.specs -lc -lnosys -Wl,--whole-archive $(HAL_WICED_LIB_FILES) -Wl,--no-whole-archive
LDFLAGS += -T$(WICED_MCU)/app_no_bootloader.ld
LDFLAGS += -L$(WICED_MCU)/STM32F2x5
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400
LDFLAGS += -u _printf_float
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

else

LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC)_dfu.ld
LDFLAGS += --specs=nano.specs -lc -lnosys
LDFLAGS += -u _printf_float
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S 
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1

endif

endif

# not using assembler startup script, but will use startup linked in with wiced


