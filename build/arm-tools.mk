#
# Define ARM tools
#

include $(COMMON_BUILD)/common-tools.mk

# Define the compiler/tools prefix
GCC_PREFIX = arm-none-eabi-

CC = $(GCC_PREFIX)gcc
CPP = $(GCC_PREFIX)g++
AR = $(GCC_PREFIX)ar
OBJCOPY = $(GCC_PREFIX)objcopy
SIZE = $(GCC_PREFIX)size
DFU = dfu-util
CURL = curl


#
# default flags for targeting ARM
#

# C compiler flags
CFLAGS +=  -g3 -gdwarf-2 -Os -mcpu=cortex-m3 -mthumb 

# C++ specific flags
CPPFLAGS += -fno-exceptions -fno-rtti -std=gnu++11

ASFLAGS +=  -g3 -gdwarf-2 -mcpu=cortex-m3 -mthumb 

LDFLAGS += -nostartfiles -Xlinker --gc-sections


