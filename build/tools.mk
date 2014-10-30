# Define the compiler/tools prefix
GCC_PREFIX ?= arm-none-eabi-

# Define tools
CC = $(GCC_PREFIX)gcc
CPP = $(GCC_PREFIX)g++
AR = $(GCC_PREFIX)ar
OBJCOPY = $(GCC_PREFIX)objcopy
SIZE = $(GCC_PREFIX)size
DFU = dfu-util
DFUSUFFIX = dfu-suffix
CURL = curl

RM = rm -f
RMDIR = rm -f -r
MKDIR = mkdir -p
