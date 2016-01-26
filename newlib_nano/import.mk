# this imports the various paths provided by this module into another module
# note that MODULE is not set to this module but the importing module

NEWLIBNANO_MODULE_NAME = newlib_nano
NEWLIBNANO_MODULE_PATH ?= $(PROJECT_ROOT)/$(NEWLIBNANO_MODULE_NAME)
NEWLIBNANO_BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)

NEWLIBNANO_LIB_DIR = $(BUILD_PATH_BASE)/$(NEWLIBNANO_MODULE_NAME)/$(NEWLIBNANO_BUILD_PATH_EXT)
NEWLIBNANO_LIB_DEP = $(NEWLIBNANO_LIB_DIR)/lib$(NEWLIBNANO_MODULE_NAME).a

LIB_DIRS += $(NEWLIBNANO_LIB_DIR)

#$(info "version $(arm_gcc_version)")
ifeq ("$(firstword $(arm_gcc_version))", "4.8.4")
NEWLIB_TWEAK_SPECS = $(NEWLIBNANO_MODULE_PATH)/src/custom-nano-4.8.4.specs
else
NEWLIB_TWEAK_SPECS = $(NEWLIBNANO_MODULE_PATH)/src/custom-nano.specs
endif