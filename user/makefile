MODULE=user
USER_MODULE_PATH=.

BUILD_PATH_EXT = $(USER_BUILD_PATH_EXT)

DEPENDENCIES = wiring system services communication hal

ifdef TEST
DEPENDENCIES += crypto
endif

include ../build/platform-id.mk

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform dynalib
CFLAGS += -DSPARK_PLATFORM
endif

# We also have HAS_SERIAL_FLASH for P1 platforms that essentially means the same thing as FLASHEE_EEPROM
# The difference is that HAS_SERIAL_FLASH is not defined when PLATFORM is the CORE.
# Consider refactoring in the future.
ifeq ("$(PLATFORM_NET)","BCM9WCDUSI14")
CFLAGS += -DFLASHEE_EEPROM
endif
ifeq ("$(PLATFORM_NET)","CC3000")
CFLAGS += -DFLASHEE_EEPROM
endif

WARNINGS_AS_ERRORS ?= n

TARGET_TYPE = a

include ../build/arm-tlm.mk
