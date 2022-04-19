# Uncomment the following to enable serial bitrate specific dfu/ymodem flasher in code
START_DFU_FLASHER_SERIAL_SPEED=14400
# Uncommenting this increase the size of the firmware image because of ymodem addition
START_YMODEM_FLASHER_SERIAL_SPEED=28800

include $(COMMON_BUILD)/version.mk

QUOTE='

ifeq ("$(DEBUG_BUILD)","y")
CFLAGS += -DDEBUG_BUILD
COMPILE_LTO ?= n
#
ifeq ("$(DEBUG_THREADING)","y")
CFLAGS += -DDEBUG_THREADING
endif
#
else
CFLAGS += -DRELEASE_BUILD
endif

ifdef SPARK_TEST_DRIVER
CFLAGS += -DSPARK_TEST_DRIVER=$(SPARK_TEST_DRIVER)
endif

# disable COMPILE_LTO when JTAG is enabled since it obfuscates the symbol mapping
# breaking step debugging
ifeq ($(USE_SWD),y)
COMPILE_LTO ?= n
endif

ifeq ($(USE_SWD_JTAG),y)
COMPILE_LTO ?= n
endif

WARNINGS_AS_ERRORS ?= y
ifeq ($(WARNINGS_AS_ERRORS),y)
CFLAGS += -Werror
CFLAGS += -Wextra
endif

# add include directories
CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS)) -I.
# Generate dependency files automatically.
CFLAGS += -MD -MP -MF $@.d
# Removed "-fdata-sections" as firmware doesn't work as expected
CFLAGS += -ffunction-sections -fdata-sections -Wall -Wno-switch -fmessage-length=0
CFLAGS += -Wno-error=deprecated-declarations -Wno-error=unused-parameter
# accepts any comment that contains something that matches (case insensitively) "falls?[ \t-]*thr(ough|u)" regular expression
# FIXME: we are using GCC 4.9 on CI for GCC platform which does not support implicit-fallthrough options and others
ifneq ($(PLATFORM_ID),3)
CFLAGS += -Wimplicit-fallthrough=2
CFLAGS += -Wno-expansion-to-defined -Wno-cast-function-type
endif
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-error=type-limits
CFLAGS += -fno-strict-aliasing
CFLAGS += -DSPARK=1 -DPARTICLE=1
CFLAGS += -Werror=return-type

CFLAGS += -Wundef

ifdef START_DFU_FLASHER_SERIAL_SPEED
CFLAGS += -DSTART_DFU_FLASHER_SERIAL_SPEED=$(START_DFU_FLASHER_SERIAL_SPEED)
endif
ifdef START_YMODEM_FLASHER_SERIAL_SPEED
CFLAGS += -DSTART_YMODEM_FLASHER_SERIAL_SPEED=$(START_YMODEM_FLASHER_SERIAL_SPEED)
endif

CONLYFLAGS += -Wno-pointer-sign

LDFLAGS += $(LIBS_EXT)
LDFLAGS += $(patsubst %,-L%,$(LIB_DIRS))

ifneq ($(PLATFORM_ID),3)
LDFLAGS += -L$(COMMON_BUILD)/arm/linker
WHOLE_ARCHIVE=y
else
ifneq ($(MAKE_OS),OSX)
WHOLE_ARCHIVE=y
endif
endif

WHOLE_ARCHIVE?=n
ifeq ($(WHOLE_ARCHIVE),y)
LDFLAGS += -Wl,--whole-archive $(patsubst %,-l%,$(LIBS)) -Wl,--no-whole-archive
else
LDFLAGS += $(patsubst %,-l%,$(LIBS))
endif

LDFLAGS += $(LIBS_EXT_END)

# Assembler flags
ASFLAGS += -x assembler-with-cpp -fmessage-length=0

ifeq (y,$(MODULAR_FIRMWARE))
MODULAR_EXT = -m
endif

COMPILE_LTO ?= n
ifeq (y,$(COMPILE_LTO))
LTO_EXT = -lto
CFLAGS += -DPARTICLE_COMPILE_LTO
endif

ifeq ("$(TARGET_TYPE)","a")
TARGET_FILE_PREFIX = lib
endif

# TARGET_FILE_NAME is the file name (minus extension) of the target produced
# TARGET_NAME is the final filename, including any prefix
TARGET_NAME ?= $(TARGET_FILE_PREFIX)$(TARGET_FILE_NAME)
TARGET_PATH ?= $(BUILD_PATH)/$(call sanitize,$(TARGET_DIR_NAME))

TARGET_BASE_DIR ?= $(TARGET_PATH)$(TARGET_SEP)
TARGET_BASE ?= $(TARGET_BASE_DIR)$(TARGET_NAME)
TARGET ?= $(TARGET_BASE).$(TARGET_TYPE)

# add BUILD_PATH_EXT with a preceeding slash if not empty.
BUILD_PATH ?= $(BUILD_PATH_BASE)/$(MODULE)$(and $(BUILD_PATH_EXT),/$(BUILD_PATH_EXT))

BUILD_TARGET_PLATFORM = platform-$(PLATFORM_ID)$(MODULAR_EXT)$(LTO_EXT)
BUILD_PATH_EXT ?= $(BUILD_TARGET_PLATFORM)

EXECUTABLE_EXTENSION=