TARGET_FREERTOS_SRC_PATH = $(FREERTOS_MODULE_PATH)/freertos/FreeRTOS/Source

# FreeRTOS
CSRC += $(TARGET_FREERTOS_SRC_PATH)/croutine.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/event_groups.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/list.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/queue.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/stream_buffer.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/tasks.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/timers.c

# FreeRTOS port
ifneq ("$(PLATFORM_FREERTOS)","")
TARGET_FREERTOS_PORT_PATH = $(TARGET_FREERTOS_SRC_PATH)/portable/GCC/$(PLATFORM_FREERTOS)
endif

# FIXME
ifeq ($(SOFTDEVICE_PRESENT),y)
# We are building these in nrf5_sdk
# CSRC += $(TARGET_NRF5_SDK_PATH)/nrf5_sdk/external/freertos/portable/GCC/nrf52/port.c
# CSRC += $(TARGET_NRF5_SDK_PATH)/nrf5_sdk/external/freertos/portable/CMSIS/nrf52/port_cmsis.c
# CSRC += $(TARGET_NRF5_SDK_PATH)/nrf5_sdk/external/freertos/portable/CMSIS/nrf52/port_cmsis_systick.c
else
CSRC += $(call target_files,$(TARGET_FREERTOS_PORT_PATH)/,*.c)
endif
