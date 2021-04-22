TARGET_AMBD_SDK_PATH = $(AMBD_SDK_MODULE_PATH)
TARGET_AMBD_SDK_SOC_PATH = $(TARGET_AMBD_SDK_PATH)/ambd_sdk/component/soc/realtek/amebad

INCLUDE_DIRS += $(TARGET_AMBD_SDK_PATH)
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)/cmsis
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)/fwlib/include
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)/app/monitor/include

# Hack of the century!
#ifneq (,$(findstring crypto,$(MAKE_DEPENDENCIES)))
#LIBS_EXT_END += $(TARGET_AMBD_SDK_EXTERNAL_PATH)/nrf_cc310/lib/cortex-m4/hard-float/libnrf_cc310_0.9.12.a
#LIBS_EXT_END += $(TARGET_AMBD_SDK_NFC_PATH)/t2t_lib/nfc_t2t_lib_gcc.a
#endif

