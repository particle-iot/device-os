# USRSRC is location of source files relative to SOURCE_PATH
# default is src in this module
USRSRC=src

# determine where user sources are, relative to project root
ifdef APP
USRSRC := applications/$(APP)
endif
ifdef APPDIR
USRSRC :=
USER_BUILD_DIR=$(patsubst ..,.,$(notdir $(APPDIR))$(call appendir,$(APP)))
endif

ifdef TEST
USRSRC := tests/$(TEST)
endif

USER_BUILD_DIR ?= $(if $(USRSRC),$(USRSRC),default)
USER_MODULE_PATH ?= $(PROJECT_ROOT)/user
USER_BUILD_PATH_EXT=$(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)/$(USER_BUILD_DIR)
USER_LIB_DIR = $(BUILD_PATH_BASE)/user/$(USER_BUILD_PATH_EXT)
USER_LIB_DEP = $(USER_LIB_DIR)/libuser.a

CFLAGS += -DINCLUDE_PLATFORM=1

# platforms.h
ifeq ($(PLATFORM_ID),3)
INCLUDE_DIRS += $(PROJECT_ROOT)/platform/shared/inc
endif

# gcc HAL is different for test driver and test subject
ifeq "$(SPARK_TEST_DRIVER)" "1"
USER_FLAVOR+=-driver
endif

include $(call rwildcard,$(USER_MODULE_PATH)/,include.mk)
