TARGET_NRF5_SDK_SRC_PATH = $(NRF5_SDK_MODULE_PATH)/nrf5_sdk
TARGET_NRF5_SDK_NRFX_SRC_PATH = $(TARGET_NRF5_SDK_SRC_PATH)/modules/nrfx

# C source files included in this build.
CSRC += $(call target_files,$(TARGET_NRF5_SDK_NRFX_SRC_PATH)/drivers/src/,*.c)
CSRC += $(call target_files,$(TARGET_NRF5_SDK_NRFX_SRC_PATH)/drivers/src/prs/,*.c)
CSRC += $(call target_files,$(TARGET_NRF5_SDK_NRFX_SRC_PATH)/hal/,*.c)
CSRC += $(call target_files,$(TARGET_NRF5_SDK_NRFX_SRC_PATH)/soc/,*.c)
CSRC += $(TARGET_NRF5_SDK_NRFX_SRC_PATH)/mdk/system_nrf52840.c
