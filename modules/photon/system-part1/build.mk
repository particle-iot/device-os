# This is specific to the photon. Include the WICED WIFI
WIFI_MODULE_WICED_LIB_FILES = $(HAL_LIB_COREV2)/resources.a $(HAL_LIB_DIR)/src/photon/resources.o $(HAL_LIB_COREV2)/FreeRTOS/STM32F2xx.a
# if !USE_MBEDTLS
# WIFI_MODULE_WICED_LIB_FILES += $(HAL_LIB_COREV2)/BESL.ARM_CM3.release.a
# WIFI_MODULE_WICED_LIB_FILES += $(HAL_LIB_COREV2)/Lib_crypto_open.a $(HAL_SRC_COREV2_PATH)/lib/Lib_micro_ecc.a
# WIFI_MODULE_WICED_LIB_FILES += $(HAL_LIB_COREV2)/Lib_base64.a
# endif
LINKER_DEPS += $(WIFI_MODULE_WICED_LIB_FILES)
include $(SHARED_MODULAR)/part1_build.mk
