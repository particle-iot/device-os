USER_PART_MODULE_PATH ?= ../user-part
include $(call rwildcard,$(USER_PART_MODULE_PATH)/,include.mk)


