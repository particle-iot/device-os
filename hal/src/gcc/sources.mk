HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template
HAL_SRC_GCC_PATH = $(TARGET_HAL_PATH)/src/gcc

templatedir=$(HAL_SRC_TEMPLATE_PATH)
overridedir=$(HAL_SRC_GCC_PATH)

# C source files included in this build.
# Use files from the template unless they are overridden by files in the 
# gcc folder. Also manually exclude some files that have changed from c->cpp.

CSRC += $(call target_files,$(templatedir)/,*.c)
CPPSRC += $(call target_files,$(templatedir)/,*.cpp)

# find the overridden list of files (without extension)
overrides = $(call rwildcard,$(overridedir)/,*.cpp)

# remove files from template that have the same basename as an overridden file
CSRC := $(filter-out $(addsuffix $(addprefix $(templatedir),$(overrides)),'.c), $(CSRC))
CPPSRC := $(filter-out $(addsuffix $(addprefix $(templatedir),$(overrides)),'.cpp), $(CPPSRC))

CSRC += $(call target_files,$(overridedir)/,*.c)
CPPSRC += $(call target_files,$(overridedir)/,*.cpp)

# ASM source files included in this build.
ASRC +=

CPPFLAGS += -DBOOST_ASIO_SEPARATE_COMPILATION