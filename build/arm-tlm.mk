# makefile for ARM top level modules

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

include $(current_dir)/top-level-module.mk


ifeq ("$(ARCH)","arm")
include $(current_dir)/arm-tools.mk
else
ifeq ("$(ARCH)","gcc")
include $(current_dir)/gcc-tools.mk
else
$(error "Unknown architecture '$(ARCH)'");
endif
endif

include $(COMMON_BUILD)/lang-std.mk

include $(COMMON_BUILD)/module.mk