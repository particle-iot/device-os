# Makefile that included by all modules - this defines the layout of the various modules

SHARED_MODULAR=$(PROJECT_ROOT)/modules/shared/rtl872x

MODULAR_FIRMWARE=y
# propagate to sub makes
MAKE_ARGS += MODULAR_FIRMWARE=y

# Ensure these defines are passed to all sub makefiles
GLOBAL_DEFINES += MODULAR_FIRMWARE=1

export PLATFORM_ID ?= 35