CPPFLAGS += -DBOOST_ASIO_SEPARATE_COMPILATION

# use the boost libraries
$(info "BOOST $(BOOST_ROOT)")
INCLUDE_DIRS += $(BOOST_ROOT)
INCLUDE_DIRS += $(BOOST_ROOT)/libs/asio/include

HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template
HAL_SRC_MESH_VIRTUAL_PATH = $(TARGET_HAL_PATH)/src/mesh-virtual

templatedir=$(HAL_SRC_TEMPLATE_PATH)
overridedir=$(HAL_SRC_MESH_VIRTUAL_PATH)

# C source files included in this build.
# Use files from the template unless they are overridden by files in the
# mesh-virtual folder. Also manually exclude some files that have changed from c->cpp.

CSRC += $(call target_files,$(templatedir)/,*.c)
CPPSRC += $(call target_files,$(templatedir)/,*.cpp)

# find the overridden list of files (without extension)
overrides_abs = $(call rwildcard,$(overridedir)/,*.cpp)
overrides_abs += $(call rwildcard,$(overridedir)/,*.c)
overrides = $(basename $(patsubst $(overridedir)/%,%,$(overrides_abs)))

remove_c = $(addsuffix .c,$(addprefix $(templatedir)/,$(overrides)))
remove_cpp = $(addsuffix .cpp,$(addprefix $(templatedir)/,$(overrides)))

# remove files from template that have the same basename as an overridden file
# e.g. if template contains core_hal.c, and mesh-virtual contains core_hal.cpp, the mesh-virtual module
# will override
CSRC := $(filter-out $(remove_c),$(CSRC))
CPPSRC := $(filter-out $(remove_cpp),$(CPPSRC))

CSRC += $(call target_files,$(overridedir)/,*.c)
CPPSRC += $(call target_files,$(overridedir)/,*.cpp)

CSRC += $(call target_files,$(HAL_MODULE_PATH)/network/lwip/,*.c)
CPPSRC += $(call target_files,$(HAL_MODULE_PATH)/network/lwip/,*.cpp)

# ASM source files included in this build.
ASRC +=

CPPFLAGS += -DBOOST_ASIO_SEPARATE_COMPILATION
CFLAGS += -DBOOST_NO_AUTO_PTR
