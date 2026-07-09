MAKE_DEPENDENCIES += third_party/libiperf

# Shared iperf server wrapper
CPPSRC += $(call target_files,tests/libraries/iperf,*.cpp)
