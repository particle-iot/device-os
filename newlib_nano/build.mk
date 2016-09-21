
include $(NEWLIBNANO_MODULE_PATH)/src/sources.mk

CFLAGS += -DHAVE_MMAP=0
ifeq ($(EMBEDDED_TARGET),1)
CFLAGS += -Wno-unused-but-set-variable
CFLAGS += -Wno-unused-variable
endif

