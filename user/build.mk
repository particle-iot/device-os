
# SOURCE_PATH - the root of all sources. Defaults to the module dir
# USRSRC - relative path to SOURCE_PATH for the sources to build

# determine where user sources are, relative to project root
ifdef APP
USER_MAKEFILE ?= $(APP).mk
# when TARGET_FILE is defined on the command line,
endif

ifdef APPDIR
# APPDIR is where the sources are found
# if TARGET_DIR is not defined defaults to $(APPDIR)/target
# if TARGET_FILE_NAME is not defined, defaults to the name of the $(APPDIR)
SOURCE_PATH = $(APPDIR)
endif


ifdef TEST
INCLUDE_PLATFORM?=1
include $(MODULE_PATH)/tests/tests.mk
-include $(MODULE_PATH)/$(USRSRC)/test.mk
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
CPPFLAGS += -std=gnu++11

BUILTINS_EXCLUDE = malloc free realloc
CFLAGS += $(addprefix -fno-builtin-,$(BUILTINS_EXCLUDE))

CFLAGS += $(EXTRA_CFLAGS)

ifeq ("$(PLATFORM_NET)", "CC3000")
# Stick to some POSIX-conforming API to disable BSD extensions
CPPFLAGS += -D_POSIX_C_SOURCE=200809
endif

# Use application source info regardless of release/debug build
CFLAGS += -DLOG_INCLUDE_SOURCE_INFO
LOG_MODULE_CATEGORY = app
