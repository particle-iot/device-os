# This is specific to the photon. Include the WICED WIFI
WIFI_MODULE_WICED_LIB_FILES = $(HAL_LIB_COREV2)/resources.a $(HAL_LIB_DIR)/src/photon/resources.o
LINKER_DEPS += $(WIFI_MODULE_WICED_LIB_FILES)
SYSTEM_MODULE_PART1_DEPENDENCY=0,0,0
include $(SHARED_MODULAR)/part1_build.mk