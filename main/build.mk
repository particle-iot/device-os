
GLOBAL_DEFINES += MODULE_VERSION=$(MAIN_MODULE_VERSION)
GLOBAL_DEFINES += MODULE_FUNCTION=$(MODULE_FUNCTION_MONO_FIRMWARE)
GLOBAL_DEFINES += MODULE_DEPENDENCY=0,0,0
GLOBAL_DEFINES += MODULE_DEPENDENCY2=0,0,0

# TARGET_FILE sets the file stem of the target. It's typically only specified when building applications.
ifdef TARGET_FILE
TARGET_FILE_NAME = $(TARGET_FILE)
endif

# TARGET_DIR is the externally specified target directory
ifdef TARGET_DIR
TARGET_PATH = $(TARGET_DIR)
endif


ifdef APP
# when TARGET_FILE is defined on the command line,
TARGET_FILE_NAME ?= $(notdir $(APP))
TARGET_DIR_NAME ?= applications/$(APP)
endif

ifdef APPDIR
# APPDIR is where the sources are found
# if TARGET_DIR is not defined defaults to $(APPDIR)/target
# if TARGET_FILE_NAME is not defined, defaults to the name of the $(APPDIR)
TARGET_FILE_NAME ?= $(notdir $(APPDIR))
ifndef TARGET_DIR
TARGET_DIR_NAME = $(APPDIR)/target
# do not use $(BUILD_PATH) since the TARGET_DIR specifies fully where the output should go
TARGET_PATH = $(TARGET_DIR_NAME)
endif
BUILD_PATH = $(TARGET_PATH)/obj
endif

ifdef TEST
TARGET_FILE_NAME ?= $(notdir $(TEST))
TARGET_DIR_NAME ?= test/$(TEST)
endif
