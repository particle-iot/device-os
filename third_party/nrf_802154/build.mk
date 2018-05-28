TARGET_NRF_802154_SRC_PATH = $(NRF_802154_MODULE_PATH)/nrf_802154

CFLAGS += -DENABLE_DEBUG_LOG=1 -DENABLE_DEBUG_GPIO=0 -DENABLE_DEBUG_ASSERT=0
CFLAGS += -DNRF52840_AAAA=0 -DNRF52840_AABA=0

CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_ack_pending_bit.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_core.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_core_hooks.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_critical_section.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_debug.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_pib.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_revision.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_rx_buffer.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_rssi.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/fem/nrf_fem_control.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/mac_features/nrf_802154_ack_timeout.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/mac_features/nrf_802154_csma_ca.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/mac_features/nrf_802154_filter.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/platform/clock/nrf_802154_clock_sdk.c
#CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/platform/timer/nrf_802154_timer_nodrv.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_notification_swi.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_priority_drop_swi.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_request_swi.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/nrf_802154_swi.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/raal/softdevice/nrf_raal_softdevice.c
CSRC += $(TARGET_NRF_802154_SRC_PATH)/src/timer_scheduler/nrf_802154_timer_sched.c
