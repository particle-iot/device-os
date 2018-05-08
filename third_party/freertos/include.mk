TARGET_FREERTOS_PATH = $(FREERTOS_MODULE_PATH)
INCLUDE_DIRS += $(FREERTOS_MODULE_PATH)/freertos/FreeRTOS/Source/include

ifneq ("$(PLATFORM_FREERTOS)","")
INCLUDE_DIRS += $(FREERTOS_MODULE_PATH)/freertos/FreeRTOS/Source/portable/GCC/$(PLATFORM_FREERTOS)
endif

# FIXME
ifeq ($(SOFTDEVICE_PRESENT),y)
INCLUDE_DIRS += $(TARGET_NRF5_SDK_PATH)/nrf5_sdk/external/freertos/portable/GCC/nrf52
INCLUDE_DIRS += $(TARGET_NRF5_SDK_PATH)/nrf5_sdk/external/freertos/portable/CMSIS/nrf52
endif
