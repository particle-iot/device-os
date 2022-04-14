
# define a symbol MAKE_OS for the current operating system
# WINSOWS | LINUX | OSX

ifeq ($(OS),Windows_NT)
    MAKE_OS = WINDOWS
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        CPU=AMD64
    endif
    ifeq ($(PROCESSOR_ARCHITECTURE),x86)
        CPU=IA32
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        MAKE_OS=LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        MAKE_OS=OSX
    endif
endif
