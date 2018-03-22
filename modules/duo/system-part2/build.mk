
CPPSRC += $(call target_files,$(PROJECT_ROOT)/modules/photon/system-part2/src,*.cpp)
CSRC += $(call target_files,$(PROJECT_ROOT)/modules/photon/system-part2/src,*.c)

include ../../shared/stm32f2xx/part2_build.mk