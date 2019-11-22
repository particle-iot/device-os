#
# Define ARM tools
#


# Define the compiler/tools prefix
GCC_PREFIX ?= arm-none-eabi-

include $(COMMON_BUILD)/common-tools.mk

AR = $(GCC_ARM_PATH)$(GCC_PREFIX)gcc-ar

#
# default flags for targeting ARM Cortex-M3
#
ifeq ("$(ARM_CPU)","cortex-m3")
# C compiler flags
CFLAGS +=  -g3 -gdwarf-2 -Os -mcpu=cortex-m3 -mthumb

# C++ specific flags
CPPFLAGS += -fno-exceptions -fno-rtti -fcheck-new

CONLYFLAGS +=

ASFLAGS +=  -g3 -gdwarf-2 -mcpu=cortex-m3 -mthumb

LDFLAGS += -nostartfiles -Xlinker --gc-sections
endif

#
# default flags for targeting ARM Cortex-M4
#
ifeq ("$(ARM_CPU)","cortex-m4")
CFLAGS += -g3 -gdwarf-2 -Os -mcpu=cortex-m4 -mthumb -mabi=aapcs
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16

# C++ specific flags
CPPFLAGS += -fno-exceptions -fno-rtti -fcheck-new

CONLYFLAGS +=

ASFLAGS +=  -g3 -gdwarf-2 -mcpu=cortex-m4 -mthumb -mabi=aapcs
ASFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16

LDFLAGS += -nostartfiles -Xlinker --gc-sections
endif

ifeq ($(COMPILE_LTO),y)
CFLAGS += -flto
LDFLAGS += -flto -Os -fuse-linker-plugin
endif

# Check if the compiler version is the minimum required
quote="
lt=\<
dollar=$$
arm_gcc_version_str:=$(strip $(shell $(CC) -dumpversion))
expected_version:=5.3.1
#$(info result $(shell test $(quote)$(arm_gcc_version_str)$(quote) $(lt) $(quote)$(expected_version)$(quote);echo $$?))
ifeq ($(shell test $(quote)$(arm_gcc_version_str)$(quote) $(lt) $(quote)$(expected_version)$(quote); echo $$?),0)
     $(error "ARM gcc version $(expected_version) or later required, but found $(arm_gcc_version_str)")
endif

arm_gcc_version_major:=$(word 1,$(subst ., ,$(arm_gcc_version_str)))
arm_gcc_version_minor:=$(word 2,$(subst ., ,$(arm_gcc_version_str)))
arm_gcc_version_patch:=$(word 3,$(subst ., ,$(arm_gcc_version_str)))
arm_gcc_version:=$(shell echo $(($(arm_gcc_version_major) * 10000 + $(arm_gcc_version_minor) * 100 + $(arm_gcc_version_patch))))

ifeq ($(shell test $(quote)$(arm_gcc_version_str)$(quote) $(lt) $(quote)4.9.0$(quote); echo $$?),0)
     NANO_SUFFIX=_s
endif

# GCC 8 linker is broken and doesn't support LENGTH(region) when defining another memory region within
# MEMORY block
ifeq ($(arm_gcc_version_major),8)
	LDFLAGS += -Wl,--defsym=GCC_LD_BROKEN=1
endif
