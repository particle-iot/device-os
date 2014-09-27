# makefile for ARM top level modules

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(patsubst %/,%,$(dir $(mkfile_path)))
	
include $(current_dir)/top-level-module.mk
include $(current_dir)/arm-tools.mk