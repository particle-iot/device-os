
# Add include to all objects built for this target
INCLUDE_DIRS += $(WIRING_MODULE_PATH)/inc

ifeq ("$(PLATFORM_ID)","88")
INCLUDE_DIRS += $(TARGET_HAL_PATH)/src/$(PLATFORM_NAME)/libraries/btstack/port
INCLUDE_DIRS += $(TARGET_HAL_PATH)/src/$(PLATFORM_NAME)/libraries/btstack/src
INCLUDE_DIRS += $(TARGET_HAL_PATH)/src/$(PLATFORM_NAME)/libraries/btstack/src/ble
INCLUDE_DIRS += $(TARGET_HAL_PATH)/src/$(PLATFORM_NAME)/libraries/btstack/src/classic
endif
