TARGET_AMBD_SDK_SRC_PATH = $(AMBD_SDK_MODULE_PATH)/ambd_sdk
TARGET_AMBD_SDK_SRC_SOC_PATH = $(TARGET_AMBD_SDK_PATH)/ambd_sdk/component/soc/realtek/amebad

#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_qdec.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_i2c.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_uart.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_captouch.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_pll.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_tim.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_bor.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_adc.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_sgpio.c
#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_gdma_memcpy.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_keyscan.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_ir.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_efuse.c
#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_ram_libc.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_comparator.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_rtc.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_flash_ram.c
#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_ipc_api.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_common/rtl8721d_wdg.c

CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_ota_ram.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_audio.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_sdio_host.c
#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_simulation.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_system.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_startup.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_usi_ssi.c
#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_psram.c
#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_sd.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_clk.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_sdio.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_ssi.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_usi_uart.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_usi_i2c.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_cpft.c
#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_app_start.c
#CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_pmc.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_lcdc.c
CSRC += $(TARGET_AMBD_SDK_SRC_SOC_PATH)/fwlib/ram_hp/rtl8721dhp_i2s.c


# C source files included in this build.
#CSRC += $(call target_files,$(TARGET_AMBD_SDK_NRFX_SRC_PATH)/drivers/src/,*.c)
#CSRC += $(call target_files,$(TARGET_AMBD_SDK_NRFX_SRC_PATH)/drivers/src/prs/,*.c)
#CSRC += $(call target_files,$(TARGET_AMBD_SDK_NRFX_SRC_PATH)/hal/,*.c)
#CSRC += $(call target_files,$(TARGET_AMBD_SDK_NRFX_SRC_PATH)/soc/,*.c)
#CSRC += $(TARGET_AMBD_SDK_NRFX_SRC_PATH)/mdk/system_nrf52840.c
#CSRC += $(TARGET_AMBD_SDK_INTEGRATION_NRFX_SRC_PATH)/legacy/nrf_drv_clock.c

