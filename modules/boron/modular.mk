# Makefile that included by all modules - this defines the layout of the various modules

SHARED_MODULAR=$(PROJECT_ROOT)/modules/shared/nRF52840

MODULAR_FIRMWARE=y
# propagate to sub makes
MAKE_ARGS += MODULAR_FIRMWARE=y
USER_FIRMWARE_IMAGE_SIZE=0x40000
USER_FIRMWARE_IMAGE_LOCATION=0xB4000

# Ensure these defines are passed to all sub makefiles
GLOBAL_DEFINES += USER_FIRMWARE_IMAGE_SIZE=$(USER_FIRMWARE_IMAGE_SIZE)
GLOBAL_DEFINES += USER_FIRMWARE_IMAGE_LOCATION=$(USER_FIRMWARE_IMAGE_LOCATION)
GLOBAL_DEFINES += MODULAR_FIRMWARE=1

export PLATFORM_ID ?= 13
