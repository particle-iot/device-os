# this is included in the top-level-module makefile to provide
# HAL-specific defines

# softAP is uses RSA functions from the communication lib

DEPENDENCIES += communication newlib_nano third_party/miniz
MAKE_DEPENDENCIES += third_party/miniz
