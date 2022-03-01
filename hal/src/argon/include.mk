
# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project

INCLUDE_DIRS += $(TARGET_HAL_PATH)/src/$(PLATFORM_NAME)

ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/lwip/esp32
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/ncp/wifi
endif


include $(TARGET_HAL_PATH)/src/nRF52840/include.mk
