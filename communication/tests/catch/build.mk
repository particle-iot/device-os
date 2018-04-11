
TARGET_COMMUNICATION_CATCH_TESTS_SRC_PATH = $(COMMUNICATION_CATCH_TESTS_MODULE_PATH)

$(info CPPSRC before $(CPPSRC))
CPPSRC += $(call target_files,$(TARGET_COMMUNICATION_CATCH_TESTS_SRC_PATH)/,*.cpp) # the final slash is necessary to ensure it doesn't pull in sources outside the catch folder, no time to investigate why
CSRC += $(call target_files,$(TARGET_COMMUNICATION_CATCH_TESTS_SRC_PATH)/,*.c)
$(info CPPSRC after $(CPPSRC))

CPPFLAGS += -std=gnu++11 -fexceptions -frtti
 
CPPFLAGS += -D_GLIBCXX_USE_C99  # needed for std::to_string


run: exe
	$(TARGET_BASE)$(EXECUTABLE_EXTENSION)

