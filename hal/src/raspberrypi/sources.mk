CPPFLAGS += -DBOOST_ASIO_SEPARATE_COMPILATION

# use the boost libraries
$(info "BOOST $(BOOST_ROOT)")
INCLUDE_DIRS += $(BOOST_ROOT)
INCLUDE_DIRS += $(BOOST_ROOT)/libs/asio/include

# Use the wiringPi library
INCLUDE_DIRS += $(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)/wiringPi

# # Compile the wiring Pi library
# WIRINGPI_DIR = $(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)/wiringPi
# WIRINGPI_LIB = $(WIRINGPI_DIR)/wiringPi/libwiringPi.a
# 
# # Compiler in the wiringPi is fixed to gcc so use a path override to use
# # the cross compiler
# WIRINGPI_XTOOLS_BIN = $(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)/x-tools-bin
# 
# $(WIRINGPI_LIB):
# 	$(VERBOSE)cd $(WIRINGPI_DIR)/wiringPi
# 	$(VERBOSE)$(MAKE) -j5 static -e PATH=$(WIRINGPI_XTOOLS_BIN):$(PATH)
# 
# These lines should go in include.mk if using wiringPi as a compiled library
# LIBS += wiringPi
# LIB_DIRS += $(WIRINGPI_LIB)


HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template
HAL_SRC_RASPBERRYPI_PATH = $(TARGET_HAL_PATH)/src/raspberrypi

templatedir=$(HAL_SRC_TEMPLATE_PATH)
overridedir=$(HAL_SRC_RASPBERRYPI_PATH)

# C source files included in this build.
# Use files from the template unless they are overridden by files in the
# raspberrypi folder. Also manually exclude some files that have changed from c->cpp.

CSRC += $(call target_files,$(templatedir)/,*.c)
CPPSRC += $(call target_files,$(templatedir)/,*.cpp)

# find the overridden list of files (without extension)
overrides_abs = $(call rwildcard,$(overridedir)/,*.cpp)
overrides_abs += $(call rwildcard,$(overridedir)/,*.c)
overrides = $(basename $(patsubst $(overridedir)/%,%,$(overrides_abs)))

remove_c = $(addsuffix .c,$(addprefix $(templatedir)/,$(overrides)))
remove_cpp = $(addsuffix .cpp,$(addprefix $(templatedir)/,$(overrides)))

# remove files from template that have the same basename as an overridden file
# e.g. if template contains core_hal.c, and raspberrypi contains
# core_hal.cpp, the raspberrypi module will override
CSRC := $(filter-out $(remove_c),$(CSRC))
CPPSRC := $(filter-out $(remove_cpp),$(CPPSRC))

CSRC += $(call target_files,$(overridedir)/,*.c)
CPPSRC += $(call target_files,$(overridedir)/,*.cpp)

# ASM source files included in this build.
ASRC +=

CPPFLAGS += -DBOOST_ASIO_SEPARATE_COMPILATION
CFLAGS += -DBOOST_NO_AUTO_PTR
CPPFLAGS += -std=gnu++11
