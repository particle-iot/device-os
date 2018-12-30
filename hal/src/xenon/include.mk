
# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project

INCLUDE_DIRS += $(TARGET_HAL_PATH)/src/$(PLATFORM_NAME)

include $(TARGET_HAL_PATH)/src/nRF52840/include.mk