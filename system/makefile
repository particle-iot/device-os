
MODULE=system
SYSTEM_MODULE_PATH=.
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(MAIN_FLAVOR)

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
CFLAGS += -DSPARK_PLATFORM
endif
DEPENDENCIES = platform hal services communication wiring dynalib
TARGET_TYPE = a
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)

include ../build/arm-tlm.mk
