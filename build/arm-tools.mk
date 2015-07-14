#
# Define ARM tools
#


# Define the compiler/tools prefix
GCC_PREFIX ?= arm-none-eabi-

include $(COMMON_BUILD)/common-tools.mk


#
# default flags for targeting ARM
#

# C compiler flags
CFLAGS +=  -g3 -gdwarf-2 -Os -mcpu=cortex-m3 -mthumb

# C++ specific flags
CPPFLAGS += -fno-exceptions -fno-rtti -fcheck-new

CONLYFLAGS +=

ASFLAGS +=  -g3 -gdwarf-2 -mcpu=cortex-m3 -mthumb

LDFLAGS += -nostartfiles -Xlinker --gc-sections

ifeq ($(COMPILE_LTO),y)
CFLAGS += -flto
LDFLAGS += -flto -Os -fuse-linker-plugin
endif

arm_gcc_version:=$(shell $(CC) --version)
expected_version:=4.9.3 20150529
ifeq ("$(findstring $(expected_version),$(arm_gcc_version))","")
     $(error "ARM gcc version $(expected_version) required, but found $(arm_gcc_version)")
endif