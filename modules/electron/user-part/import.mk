USER_PART_MODULE_VERSION ?= 1
USER_PART_MODULE_PATH ?= $(PROJECT_ROOT)/modules/electron/user-part
include $(call rwildcard,$(USER_PART_MODULE_PATH)/,include.mk)


