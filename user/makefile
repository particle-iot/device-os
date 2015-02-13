MODULE=user
USER_MODULE_PATH=.

BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)

DEPENDENCIES = wiring system services communication hal

include ../build/platform-id.mk

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
ifneq ("$(PLATFORM_NET)","BCM9WCDUSI09")
CFLAGS += -DSPARK_PLATFORM
CFLAGS += -DFLASHEE_EEPROM
endif
endif

TARGET_TYPE = a

include ../build/arm-tlm.mk
