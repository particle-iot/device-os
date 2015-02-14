SYSTEM_PART2_MODULE_PATH ?= ../system-part2
include $(call rwildcard,$(SYSTEM_PART2_MODULE_PATH)/,include.mk)

