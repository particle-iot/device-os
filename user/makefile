MODULE=user
USER_MODULE_PATH=.

BUILD_PATH_EXT = $(USER_BUILD_PATH_EXT)

DEPENDENCIES = wiring system services communication hal

ifdef TEST
DEPENDENCIES += crypto
endif

include ../build/platform-id.mk

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
CFLAGS += -DSPARK_PLATFORM
endif

DEPENDENCIES += dynalib

WARNINGS_AS_ERRORS ?= n

TARGET_TYPE = a

include ../build/arm-tlm.mk
