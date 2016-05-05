HAL_MODULE_NAME=hal
HAL_MODULE_PATH?=$(PROJECT_ROOT)/$(HAL_MODULE_NAME)

HAL_BUILD_PATH_EXT=$(BUILD_TARGET_PLATFORM)$(HAL_TEST_FLAVOR)
HAL_LIB_DIR = $(BUILD_PATH_BASE)/$(HAL_MODULE_NAME)/$(HAL_BUILD_PATH_EXT)
HAL_LIB_DEP = $(HAL_LIB_DIR)/lib$(HAL_MODULE_NAME).a

include $(call rwildcard,$(HAL_MODULE_PATH)/inc/,include.mk)
include $(call rwildcard,$(HAL_MODULE_PATH)/shared/,include.mk)
include $(call rwildcard,$(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)/,include.mk)

