HAL_SRC_GCC_PATH = $(TARGET_HAL_PATH)/src/gcc

HAL_SRC_TEMPLATE_PATH = $(TARGET_HAL_PATH)/src/template

basedir = HAL_SRC_TEMPLATE_PATH
overridedir = HAL_SRC_GCC_PATH

# C source files included in this build.
# Use files from the template unless they are overridden by files in the 
# gcc folder. Also manually exclude some files that have changed from c->cpp.

CSRC += $(call target_files,$(basedir)/,*.c)
CPPSRC += $(call target_files,$(basedir)/,*.cpp)

# find the overridden list of files (without extension)
overrides = $(basename $(call,rwildcard,$(override),*.c*))

# remove files from template that have the same basename as an overridden file
CSRC := $(filter-out $(addsuffix $(addprefix $(basedir),$(overrides)),'.c), $(CSRC))
CPPSRC := $(filter-out $(addsuffix $(addprefix $(basedir),$(overrides)),'.cpp), $(CPPSRC))

CSRC += $(call target_files,$(override)/,*.c)
CPPSRC += $(call target_files,$(override)/,*.cpp)

$(info source $(CSRC) $(CPPSRC))

# ASM source files included in this build.
ASRC +=

