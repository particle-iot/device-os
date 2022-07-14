#
# Define ARM tools
#

# Define the compiler/tools prefix
GCC_PREFIX ?= arm-none-eabi-

include $(COMMON_BUILD)/common-tools.mk

AR = $(GCC_ARM_PATH)$(GCC_PREFIX)gcc-ar

# Newlib has a bug in fake stdin/stdout/stderr implementation and leaks memory
# Stub printf/fprintf out
CFLAGS += -fno-builtin-puts -fno-builtin-printf -Wl,--wrap=puts -Wl,--wrap=printf

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

#enable the build-id GCC feature 
LDFLAGS += -Wl,--build-id
endif

#
# default flags for targeting ARM Cortex-M33
#
ifeq ("$(ARM_CPU)","cortex-m33")
CFLAGS += -g3 -gdwarf-2 -Os -mcpu=cortex-m33 -march=armv8-m.main+dsp -mthumb
CFLAGS += -mcmse -mfloat-abi=hard -mfpu=fpv5-sp-d16

# C++ specific flags
CPPFLAGS += -fno-exceptions -fno-rtti -fcheck-new

CONLYFLAGS +=

ASFLAGS +=  -g3 -gdwarf-2 -mcpu=cortex-m33 -march=armv8-m.main+dsp -mthumb
ASFLAGS += -mcmse -mfloat-abi=hard -mfpu=fpv5-sp-d16

LDFLAGS += -nostartfiles -Xlinker --gc-sections
endif

#
# default flags for targeting ARM Cortex-M23
#
ifeq ("$(ARM_CPU)","cortex-m23")
CFLAGS += -g3 -gdwarf-2 -Os -mcpu=cortex-m23 -march=armv8-m.base -mthumb -D__ARM_FEATURE_DSP=0

# C++ specific flags
CPPFLAGS += -fno-exceptions -fno-rtti -fcheck-new

CONLYFLAGS +=

ASFLAGS +=  -g3 -gdwarf-2 -mcpu=cortex-m23 -march=armv8-m.base -mthumb -D__ARM_FEATURE_DSP=0

LDFLAGS += -nostartfiles -Xlinker --gc-sections
endif

CFLAGS += -DARM_CPU_$(shell echo $(ARM_CPU) | tr '-' '_' | tr 'a-z' 'A-Z')

# NOTE: this does not enable LTO! This allows to build object files
# that can be linked with LTO enabled and disabled (https://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html)
# If LTO is disabled, LTO information is simply discarded. These parameters
# are only applied when compiling the sources. A separate setting during linking stage
# would control whether LTO is enabled or not.
#
# -fno-use-cxa-atexit makes sure that destructors for statically created C++ objects are never called,
# which saves us some flash space.
LTO_FLAGS = -flto -ffat-lto-objects -DPARTICLE_COMPILE_LTO_FAT
CPPFLAGS += $(LTO_FLAGS) -fno-use-cxa-atexit
CONLYFLAGS += $(LTO_FLAGS)
LDFLAGS += -fno-use-cxa-atexit

ifeq ($(COMPILE_LTO),y)
LDFLAGS += -flto -Os -fuse-linker-plugin
else
# Be explicit and disable LTO
LDFLAGS += -fno-lto
endif

# We are using newlib-nano for all the platforms
CFLAGS += --specs=nano.specs

# Check if the compiler version is the minimum required
version_to_number=$(shell v=$1; v=($${v//./ }); echo $$((v[0] * 10000 + v[1] * 100 + v[2])))
get_major_version=$(shell v=$1; v=($${v//./ }); echo $${v[0]})
arm_gcc_version_str:=$(shell $(CC) -dumpversion)
arm_gcc_version:=$(call version_to_number,$(arm_gcc_version_str))
expected_version_str:=10.2.1
ifeq ($(shell test $(arm_gcc_version) -lt $(call version_to_number,$(expected_version_str)); echo $$?),0)
     $(error "ARM gcc version $(expected_version_str) or later required, but found $(arm_gcc_version_str)")
endif

ifeq ($(shell test $(arm_gcc_version) -lt $(call version_to_number,4.9.0); echo $$?),0)
     NANO_SUFFIX=_s
endif

# GCC 8 linker is broken and doesn't support LENGTH(region) when defining another memory region within
# MEMORY block
ifeq ($(call get_major_version,$(arm_gcc_version_str)),8)
	LDFLAGS += -Wl,--defsym=GCC_LD_BROKEN=1
endif
