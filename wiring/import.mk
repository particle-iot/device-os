WIRING_MODULE_PATH ?= ../wiring
include $(call rwildcard,$(WIRING_MODULE_PATH)/,include.mk)

WIRING_LIB_DIR = $(BUILD_PATH_BASE)/wiring/$(ARCH)
WIRING_LIB_DEP = $(WIRING_LIB_DIR)/libwiring.a