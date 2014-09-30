
GCC_PREFIX=

include $(COMMON_BUILD)/common-tools.mk

#
# default flags for targeting ARM
#

# C compiler flags
CFLAGS +=  -g3 -m32

ASFLAGS +=  -g3


LDFLAGS += -Xlinker --gc-sections



