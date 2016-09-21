#
# Define Linux on ARM tools
#

GCC_PREFIX?=arm-unknown-linux-gnueabi-

include $(COMMON_BUILD)/common-tools.mk

#
# default flags for targeting ARM
#

GCC_OPTIMIZE=3
ifeq ($(DEBUG_BUILD),y)
     GCC_OPTIMIZE=0
endif

# C compiler flags
CFLAGS +=  -g3 -O$(GCC_OPTIMIZE) -gdwarf-2
CFLAGS += -Wno-unused-local-typedefs
CPPFLAGS += -fno-rtti -fcheck-new
ASFLAGS +=  -g3

LDFLAGS +=  -pthread -Xlinker --gc-sections

