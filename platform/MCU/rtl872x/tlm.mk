# Inject dependencies
DEPENDENCIES += third_party/ambd_sdk
MAKE_DEPENDENCIES += third_party/ambd_sdk

ifneq ("$(ARM_CPU)","cortex-m23")
DEPENDENCIES += third_party/littlefs third_party/miniz
MAKE_DEPENDENCIES += third_party/littlefs third_party/miniz
endif
