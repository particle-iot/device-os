USER_MODULE_PATH ?= $(PROJECT_ROOT)/user
include $(call rwildcard,$(USER_MODULE_PATH)/,include.mk)

USER_LIB_DIR = $(BUILD_PATH_BASE)/user/$(BUILD_TARGET_PLATFORM)
USER_LIB_DEP = $(USER_LIB_DIR)/libuser.a
	
CFLAGS += -DINCLUDE_PLATFORM=1

# gcc HAL is different for test driver and test subject
ifeq "$(SPARK_TEST_DRIVER)" "1"
USER_FLAVOR+=-driver
endif

