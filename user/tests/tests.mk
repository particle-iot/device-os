# set up environment for compiling test apps
# for now, hard-wire the unit-test library

INCLUDE_DIRS += tests/libraries
CPPSRC += $(call target_files,tests/libraries/unit-test,*.cpp)

# Disable compiler warnings when deprecated APIs are used in test code
CFLAGS+=-DPARTICLE_USING_DEPRECATED_API
