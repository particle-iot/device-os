
RM = rm -f
RMDIR = rm -f -r
MKDIR = mkdir -p

CC = $(GCC_PREFIX)gcc
CPP = $(GCC_PREFIX)g++
AR = $(GCC_PREFIX)ar
OBJCOPY = $(GCC_PREFIX)objcopy
OBJDUMP = $(GCC_PREFIX)objdump
SIZE = $(GCC_PREFIX)size
DFU = dfu-util
CURL = curl

CPPFLAGS += -std=gnu++11

