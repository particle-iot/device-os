MODULE=user
USER_MODULE_PATH=.

BUILD_PATH_EXT = $(USER_BUILD_PATH_EXT)

DEPENDENCIES = wiring system services communication hal

include ../build/platform-id.mk

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform dynalib
CFLAGS += -DSPARK_PLATFORM
endif

ifneq ("$(PLATFORM_NET)","BCM9WCDUSI09")
CFLAGS += -DFLASHEE_EEPROM
endif

WARNINGS_AS_ERRORS ?= n

TARGET_TYPE = a

include ../build/arm-tlm.mk
