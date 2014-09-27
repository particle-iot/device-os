HAL_MODULE_PATH?=../hal
include $(call rwildcard,$(HAL_MODULE_PATH)/,include.mk)

LIB_DIRS += $(BUILD_PATH_BASE)/hal
LIBS += hal