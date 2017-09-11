#
# Define ARM tools
#


# Define the compiler/tools prefix
GCC_PREFIX ?= arm-none-eabi-

include $(COMMON_BUILD)/common-tools.mk

AR = $(GCC_ARM_PATH)$(GCC_PREFIX)gcc-ar

#
# default flags for targeting ARM
#

# C compiler flags
CFLAGS +=  -g3 -gdwarf-2 -Os -mcpu=cortex-m3 -mthumb -fomit-frame-pointer

# C++ specific flags
CPPFLAGS += -fno-exceptions -fno-rtti -fcheck-new

CONLYFLAGS +=

ASFLAGS +=  -g3 -gdwarf-2 -mcpu=cortex-m3 -mthumb -fomit-frame-pointer

LDFLAGS += -nostartfiles -Xlinker --gc-sections

ifeq ($(COMPILE_LTO),y)
CFLAGS += -flto
LDFLAGS += -flto -Os -fuse-linker-plugin
endif

# Check if the compiler version is the minimum required
quote="
lt=\<
dollar=$$
arm_gcc_version:=$(strip $(shell $(CC) -dumpversion))
expected_version:=5.3.1
#$(info result $(shell test $(quote)$(arm_gcc_version)$(quote) $(lt) $(quote)$(expected_version)$(quote);echo $$?))
ifeq ($(shell test $(quote)$(arm_gcc_version)$(quote) $(lt) $(quote)$(expected_version)$(quote); echo $$?),0)
     $(error "ARM gcc version $(expected_version) or later required, but found $(arm_gcc_version)")
endif


ifeq ($(shell test $(quote)$(arm_gcc_version)$(quote) $(lt) $(quote)4.9.0$(quote); echo $$?),0)
     NANO_SUFFIX=_s
endif
