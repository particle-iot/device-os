WIRING_MODULE_PATH ?= ../wiring
include $(call rwildcard,$(WIRING_MODULE_PATH)/,include.mk)

LIB_DIRS += $(BUILD_PATH_BASE)/wiring
LIBS += wiring