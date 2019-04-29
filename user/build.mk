
# SOURCE_PATH - the root of all sources. Defaults to the module dir
# USRSRC - relative path to SOURCE_PATH for the sources to build

# determine where user sources are, relative to project root

# propagate the APPLIBV1s to module libs only when building this user module
# (otherwise if we used APPLIBs directly in the module build each recursively
# built module would build the libs.)
MODULE_LIBSV1+=$(call remove_slash,$(APPLIBSV1))
MODULE_LIBSV2+=$(call remove_slash,$(APPLIBSV2))

ifdef APP
USER_MAKEFILE ?= $(APP).mk
# when TARGET_FILE is defined on the command line,
endif

ifdef APPDIR
# APPDIR is where the sources are found
# if TARGET_DIR is not defined defaults to $(APPDIR)/target
# if TARGET_FILE_NAME is not defined, defaults to the name of the $(APPDIR)
SOURCE_PATH = $(call remove_slash,$(APPDIR))
endif

ifdef TEST
INCLUDE_PLATFORM?=1
# Disable compiler warnings when deprecated APIs are used in test code
CFLAGS+=-DPARTICLE_USING_DEPRECATED_API
include $(MODULE_PATH)/tests/tests.mk
-include $(MODULE_PATH)/$(USRSRC)/test.mk
endif

# the root of the application
APPROOT := $(SOURCE_PATH)$(USRSRC)

ifneq ($(wildcard $(APPROOT)/project.properties),)
	ifneq ($(wildcard $(APPROOT)/src),)
	   APPLAYOUT=extended
	else
	   APPLAYOUT=simple
	endif
else
   APPLAYOUT=legacy
endif

ifeq ($(APPLAYOUT),extended)
# add vendored libraries to module libraries
MODULE_LIBSV2 += $(wildcard $(APPROOT)/lib/*)
SOURCE_PATH := $(APPROOT)/
USRSRC = src
endif

USRSRC_SLASH = $(and $(USRSRC),$(USRSRC)/)
USER_MAKEFILE ?= build.mk
usrmakefile = $(wildcard $(SOURCE_PATH)/$(USRSRC_SLASH)$(USER_MAKEFILE))
ifeq ("$(usrmakefile)","")
INCLUDE_DIRS += $(SOURCE_PATH)/$(USRSRC)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(USRSRC_SLASH),*.cpp)
CSRC += $(call target_files,$(USRSRC_SLASH),*.c)

APPSOURCES=$(call target_files,$(USRSRC_SLASH),*.cpp)
APPSOURCES+=$(call target_files,$(USRSRC_SLASH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(SOURCE_PATH)/$(USRSRC)")
endif
else
include $(usrmakefile)
endif

INCLUDE_DIRS += $(MODULE_PATH)/libraries

CFLAGS += -DSPARK_PLATFORM_NET=$(PLATFORM_NET)

BUILTINS_EXCLUDE = malloc free realloc
CFLAGS += $(addprefix -fno-builtin-,$(BUILTINS_EXCLUDE))

CFLAGS += $(EXTRA_CFLAGS)

# Use application source info regardless of release/debug build
CFLAGS += -DLOG_INCLUDE_SOURCE_INFO=1
LOG_MODULE_CATEGORY = app

CFLAGS += -DPARTICLE_USER_MODULE
