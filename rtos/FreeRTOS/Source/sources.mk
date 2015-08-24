# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_FREERTOS_SRC_PATH = $(TARGET_FREERTOS_PATH)

# C source files included in this build.
CSRC += $(TARGET_FREERTOS_SRC_PATH)/portable/GCC/ARM_CM3/port.c

# CSRC += $(TARGET_FREERTOS_SRC_PATH)/portable/MemMang/heap_1.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/portable/MemMang/heap_3.c

CSRC += $(TARGET_FREERTOS_SRC_PATH)/croutine.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/event_groups.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/list.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/queue.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/tasks.c
CSRC += $(TARGET_FREERTOS_SRC_PATH)/timers.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

