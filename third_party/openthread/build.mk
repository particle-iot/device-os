TARGET_OPENTHREAD_SRC_PATH = $(OPENTHREAD_MODULE_PATH)/openthread

CPPSRC += $(call target_files,$(TARGET_OPENTHREAD_SRC_PATH)/src/core/,*.cpp)
CSRC += $(call target_files,$(TARGET_OPENTHREAD_SRC_PATH)/src/core/,*.c)

INCLUDE_DIRS += $(TARGET_OPENTHREAD_SRC_PATH)/include/openthread

CFLAGS += -DENABLE_DEBUG_LOG=0 -DENABLE_DEBUG_GPIO=0 -DENABLE_DEBUG_ASSERT=0

ifeq ($(PLATFORM_OPENTHREAD),nrf52840)
CFLAGS += -DNRF52840_AAAA=0 -DNRF52840_AABA=0
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_notification_swi.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_priority_drop_swi.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_request_swi.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_swi.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/raal/softdevice/nrf_raal_softdevice.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_ack_pending_bit.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_core.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_core_hooks.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_critical_section.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_debug.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_pib.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_revision.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_rssi.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_rx_buffer.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/fem/nrf_fem_control.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_filter.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_ack_timeout.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_csma_ca.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/platform/clock/nrf_802154_clock_sdk.c
CSRC += $(TARGET_OPENTHREAD_SRC_PATH)/third_party/NordicSemiconductor/drivers/radio/timer_scheduler/nrf_802154_timer_sched.c
endif
