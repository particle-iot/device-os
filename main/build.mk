
# USRSRC is location of source files relative to SOURCE_PATH
# default is src in this module
USRSRC=src

# TARGET_FILE sets the file stem of the target. It's typically only specified when building applications.
ifdef TARGET_FILE
TARGET_FILE_NAME = $(TARGET_FILE)
endif

# TARGET_DIR is the externally specified target directory
ifdef TARGET_DIR
TARGET_PATH = $(TARGET_DIR)
endif


# determine where user sources are, relative to project root
ifdef APP
USRSRC = applications/$(APP)
# when TARGET_FILE is defined on the command line, 
TARGET_FILE_NAME ?= $(notdir $(APP))
TARGET_DIR_NAME ?= $(USRSRC)
endif

ifdef APPDIR
# APPDIR is where the sources are found
# if TARGET_DIR is not defined defaults to $(APPDIR)/target
# if TARGET_FILE_NAME is not defined, defaults to the name of the $(APPDIR)
USRSRC = 
SOURCE_PATH = $(APPDIR)
TARGET_FILE_NAME ?= $(notdir $(APPDIR))
TARGET_DIR_NAME = $(APPDIR)/target
# do not use $(BUILD_PATH) since the TARGET_DIR specifies fully where the output should go
TARGET_PATH ?= $(TARGET_DIR_NAME)
BUILD_PATH = $(TARGET_PATH)/obj
endif


ifdef TEST
USRSRC = tests/$(TEST)
INCLUDE_PLATFORM?=1
TARGET_FILE_NAME ?= $(notdir $(TEST))
TARGET_DIR_NAME ?= $(USRSRC)
include $(MODULE_PATH)/tests/tests.mk
-include $(MODULE_PATH)/$(USRSRC)/test.mk
endif

# user sources specified, so override application.cpp and pull in files from 
# the user source folder
INCLUDE_DIRS += $(SOURCE_PATH)/$(USRSRC)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
USRSRC_SLASH = $(and $(USRSRC),$(USRSRC)/)
CPPSRC += $(call target_files,$(USRSRC_SLASH),*.cpp)
CSRC += $(call target_files,$(USRSRC_SLASH),*.c)    


INCLUDE_DIRS += $(MODULE_PATH)/libraries

CFLAGS += -DSPARK_PLATFORM_NET=$(PLATFORM_NET)

