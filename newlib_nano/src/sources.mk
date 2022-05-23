NEWLIBNANO_SRC_COMMON_PATH = $(NEWLIBNANO_MODULE_PATH)/src

# Use FreeRTOS heap_4 and a set of newlib wrappers
CPPSRC += $(NEWLIBNANO_SRC_COMMON_PATH)/malloc.cpp
