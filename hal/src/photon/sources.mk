
HAL_SRC_STM32F2XX_PATH = $(TARGET_HAL_PATH)/src/stm32f2xx
HAL_SRC_STM32_PATH = $(TARGET_HAL_PATH)/src/stm32
HAL_SRC_PHOTON_PATH = $(TARGET_HAL_PATH)/src/photon

# private includes - WICED is not exposed to the HAL clients

HAL_WICED_INCLUDE_DIRS +=   include
HAL_WICED_INCLUDE_DIRS +=   platforms/$(PLATFORM_NET)
HAL_WICED_INCLUDE_DIRS +=   libraries/daemons/DNS_redirect \
			    libraries/daemons/HTTP_server \
			    libraries/protocols/DNS \
			    libraries/utilities/ring_buffer
HAL_WICED_INCLUDE_DIRS +=   wiced wiced/internal
HAL_WICED_INCLUDE_DIRS +=   wiced/network/$(HAL_WICED_NETWORK) \
			    wiced/network/$(HAL_WICED_NETWORK)/WWD \
			    wiced/network/$(HAL_WICED_NETWORK)/WICED
HAL_WICED_INCLUDE_DIRS +=   wiced/platform/ARM_CM3 \
			    wiced/platform/ARM_CM3/CMSIS \
			    wiced/platform/GCC \
			    wiced/platform/include \
			    wiced/platform/MCU \
			    wiced/platform/MCU/STM32F2xx \
			    wiced/platform/MCU/STM32F2xx/peripherals \
			    wiced/platform/MCU/STM32F2xx/WAF
HAL_WICED_INCLUDE_DIRS +=   wiced/RTOS/$(HAL_WICED_RTOS) \
			    wiced/RTOS/$(HAL_WICED_RTOS)/WWD \
			    wiced/RTOS/$(HAL_WICED_RTOS)/WICED
HAL_WICED_INCLUDE_DIRS +=   wiced/WWD
HAL_WICED_INCLUDE_DIRS +=   libraries/utilities/linked_list

ifeq "$(HAL_WICED_NETWORK)" "LwIP"
HAL_WICED_INCLUDE_DIRS +=   wiced/network/LwIP/ver1.4.1/src/include \
			    wiced/network/LwIP/ver1.4.1/src/include/ipv4 \
			    wiced/network/LwIP/WWD/FreeRTOS
endif
ifeq "$(HAL_WICED_NETWORK)" "NetX"
HAL_WICED_INCLUDE_DIRS += wiced/network/NetX/ver5.5_sp1
endif
ifeq "$(HAL_WICED_RTOS)" "ThreadX"
HAL_WICED_INCLUDE_DIRS +=   wiced/RTOS/ThreadX/ver5.6 \
			    wiced/RTOS/ThreadX/ver5.6/Cortex_M3_M4/GCC
endif
ifeq "$(HAL_WICED_RTOS)" "FreeRTOS"
HAL_WICED_INCLUDE_DIRS +=   wiced/RTOS/FreeRTOS/WWD/ARM_CM3 \
			    wiced/RTOS/FreeRTOS/ver8.2.1/Source/include \
			    wiced/RTOS/FreeRTOS/ver8.2.1/Source/portable/GCC/ARM_CM3
CSRC += $(HAL_SRC_PHOTON_PATH)/wiced/RTOS/FreeRTOS/ver8.2.1/Source/portable/MemMang/heap_4_lock.c

endif



INCLUDE_DIRS += $(addprefix $(HAL_SRC_PHOTON_PATH)/,$(sort $(HAL_WICED_INCLUDE_DIRS)))
INCLUDE_DIRS += $(dir $(call rwildcard,$(HAL_SRC_PHOTON_PATH)/wiced/security,*.h))
INCLUDE_DIRS += $(dir $(call rwildcard,$(HAL_SRC_PHOTON_PATH)/wiced/WWD,*.h))
INCLUDE_DIRS += $(HAL_SRC_STM32F2XX_PATH)
INCLUDE_DIRS += $(HAL_SRC_STM32_PATH)

CSRC += $(call target_files,$(HAL_SRC_PHOTON_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_PHOTON_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_SRC_STM32F2XX_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_STM32F2XX_PATH)/,*.cpp)

CSRC += $(call target_files,$(HAL_SRC_STM32_PATH)/,*.c)
CPPSRC += $(call target_files,$(HAL_SRC_STM32_PATH)/,*.cpp)


CFLAGS += -DSFLASH_APPS_HEADER_LOC=0x0000 -DUSE_STDPERIPH_DRIVER -D_STM32F215RGT6_ -D_STM3x_ -D_STM32x_ -DMAX_WATCHDOG_TIMEOUT_SECONDS=22 -DFIRMWARE_WITH_PMK_CALC_SUPPORT -DADD_LWIP_EAPOL_SUPPORT -DNXD_EXTENDED_BSD_SOCKET_SUPPORT -DOPENSSL -DSTDC_HEADERS -DUSE_SRP_SHA_512 -DADD_NETX_EAPOL_SUPPORT -DUSE_MICRORNG -DWWD_STARTUP_DELAY=10 -DBOOTLOADER_MAGIC_NUMBER=0x4d435242 -DNETWORK_NetX=1 -DNetX_VERSION=\"v5.5_sp1\" -DNX_INCLUDE_USER_DEFINE_FILE -D__fd_set_defined -DSYS_TIME_H_AVAILABLE -DRTOS_ThreadX=1 -DThreadX_VERSION=\"v5.6\" -DTX_INCLUDE_USER_DEFINE_FILE -DWWD_DIRECT_RESOURCES

# Disable GNU extensions for libc to avoid conflicts with WICED
NO_GNU_EXTENSIONS=1

# ASM source files included in this build.
ASRC +=



# Uncomment this to build the bootloader source file.
#$(HAL_PLATFORM_SRC_PATH)/bootloader_platform_$(PLATFORM_ID).cc $(COMMON_BUILD)/target/bootloader/platform-$(PLATFORM_ID)-lto/bootloader.bin
#	xxd -i $< > $@