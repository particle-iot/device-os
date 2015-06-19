# this is included in the top-level-module makefile to provide
# HAL-specific defines

# softAP is uses RSA functions from the communication lib

DEPENDENCIES += communication

CPPFLAGS += -std=gnu++11

