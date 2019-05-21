TARGET_NRF5_SDK_SRC_PATH = $(NRF5_SDK_MODULE_PATH)/nrf5_sdk

TARGET_NRF5_SDK_LIBRARIES_PATH = $(TARGET_NRF5_SDK_PATH)/nrf5_sdk/components/libraries
TARGET_NRF5_SDK_LIBRARY_UTIL_PATH = $(TARGET_NRF5_SDK_SRC_PATH)/components/libraries/util
TARGET_NRF5_SDK_NRFX_SRC_PATH = $(TARGET_NRF5_SDK_SRC_PATH)/modules/nrfx
TARGET_NRF5_SDK_INTEGRATION_NRFX_SRC_PATH = $(TARGET_NRF5_SDK_SRC_PATH)/integration/nrfx
TARGET_NRF5_SDK_DRIVERS_NRF_SRC_PATH = $(TARGET_NRF5_SDK_SRC_PATH)/components/drivers_nrf
TARGET_NRF5_SDK_SOFTDEVICE_SRC_PATH = $(TARGET_NRF5_SDK_SRC_PATH)/components/softdevice
TARGET_NRF5_SDK_BLE_SRC_PATH = $(TARGET_NRF5_SDK_PATH)/nrf5_sdk/components/ble
TARGET_NRF5_SDK_EXTERNAL_SRC_PATH = $(TARGET_NRF5_SDK_PATH)/nrf5_sdk/external

# C source files included in this build.
CSRC += $(call target_files,$(TARGET_NRF5_SDK_NRFX_SRC_PATH)/drivers/src/,*.c)
CSRC += $(call target_files,$(TARGET_NRF5_SDK_NRFX_SRC_PATH)/drivers/src/prs/,*.c)
CSRC += $(call target_files,$(TARGET_NRF5_SDK_NRFX_SRC_PATH)/hal/,*.c)
CSRC += $(call target_files,$(TARGET_NRF5_SDK_NRFX_SRC_PATH)/soc/,*.c)
CSRC += $(TARGET_NRF5_SDK_NRFX_SRC_PATH)/mdk/system_nrf52840.c
CSRC += $(TARGET_NRF5_SDK_INTEGRATION_NRFX_SRC_PATH)/legacy/nrf_drv_clock.c
CSRC += $(TARGET_NRF5_SDK_INTEGRATION_NRFX_SRC_PATH)/legacy/nrf_drv_power.c
CSRC += $(TARGET_NRF5_SDK_INTEGRATION_NRFX_SRC_PATH)/legacy/nrf_drv_rng.c
CSRC += $(TARGET_NRF5_SDK_LIBRARY_UTIL_PATH)/app_util_platform.c
CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/atomic/nrf_atomic.c
CSRS += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/atomic_fifo/nrf_atfifo.c
CSRC += $(TARGET_NRF5_SDK_INTEGRATION_NRFX_SRC_PATH)/legacy/nrf_drv_spi.c

CFLAGS += -Wno-unused-but-set-variable

CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/fstorage/nrf_fstorage.c
ifeq ($(SOFTDEVICE_PRESENT),y)
CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/fstorage/nrf_fstorage_sd.c
else
CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/fstorage/nrf_fstorage_nvmc.c
endif
CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/experimental_section_vars/nrf_section_iter.c
CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/crc32/crc32.c
CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/queue/nrf_queue.c

CFLAGS += -DCONFIG_GPIO_AS_PINRESET

ifeq ("$(SOFTDEVICE_PRESENT)","y")
CSRC += $(TARGET_NRF5_SDK_SOFTDEVICE_SRC_PATH)/common/nrf_sdh.c
CSRC += $(TARGET_NRF5_SDK_SOFTDEVICE_SRC_PATH)/common/nrf_sdh_ble.c
CSRC += $(TARGET_NRF5_SDK_SOFTDEVICE_SRC_PATH)/common/nrf_sdh_soc.c
CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/experimental_section_vars/nrf_section_iter.c
CSRC += $(TARGET_NRF5_SDK_LIBRARIES_PATH)/atomic_fifo/nrf_atfifo.c
else
CSRC += $(TARGET_NRF5_SDK_DRIVERS_NRF_SRC_PATH)/nrf_soc_nosd/nrf_soc.c
CSRC += $(TARGET_NRF5_SDK_DRIVERS_NRF_SRC_PATH)/nrf_soc_nosd/nrf_nvic.c
endif

ifeq ("$(SOFTDEVICE_PRESENT)","y")
# Libraries
CSRC += \
	$(call target_files,$(TARGET_NRF5_SDK_LIBRARIES_PATH)/fifo/,*.c) \
	$(call target_files,$(TARGET_NRF5_SDK_LIBRARIES_PATH)/atomic_flags/,*.c) \
	$(call target_files,$(TARGET_NRF5_SDK_LIBRARIES_PATH)/atomic_fifo/,*.c) \
	$(TARGET_NRF5_SDK_LIBRARIES_PATH)/fstorage/nrf_fstorage.c \
	$(TARGET_NRF5_SDK_LIBRARIES_PATH)/fstorage/nrf_fstorage_sd.c

# BLE
CSRC += \
	$(call target_files,$(TARGET_NRF5_SDK_BLE_SRC_PATH)/common/,*.c) \
	$(call target_files,$(TARGET_NRF5_SDK_BLE_SRC_PATH)/nrf_ble_gatt/,*.c) \
	$(call target_files,$(TARGET_NRF5_SDK_BLE_SRC_PATH)/ble_advertising/,*.c)

# USBD
CSRC += \
	$(TARGET_NRF5_SDK_LIBRARIES_PATH)/usbd/app_usbd.c \
	$(TARGET_NRF5_SDK_LIBRARIES_PATH)/usbd/app_usbd_core.c \
	$(TARGET_NRF5_SDK_LIBRARIES_PATH)/usbd/app_usbd_serial_num.c \
	$(TARGET_NRF5_SDK_LIBRARIES_PATH)/usbd/app_usbd_string_desc.c \
	$(TARGET_NRF5_SDK_LIBRARIES_PATH)/usbd/class/cdc/acm/app_usbd_cdc_acm.c

# FreeRTOS
ifeq ($(SOFTDEVICE_PRESENT),y)
CSRC += $(TARGET_NRF5_SDK_SRC_PATH)/external/freertos/portable/GCC/nrf52/port.c
CSRC += $(TARGET_NRF5_SDK_SRC_PATH)/external/freertos/portable/CMSIS/nrf52/port_cmsis.c
CSRC += $(TARGET_NRF5_SDK_SRC_PATH)/external/freertos/portable/CMSIS/nrf52/port_cmsis_systick.c
endif

#ifeq ($(DEBUG_BUILD),y)
#CSRC += \
#	$(TARGET_NRF5_SDK_EXTERNAL_SRC_PATH)/segger_rtt/SEGGER_RTT.c
#endif
endif
