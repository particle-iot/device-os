# Handles recursive make calls to dependent modules

# create a list of targets to clean from the list of dependencies
CLEAN_DEPENDENCIES=$(patsubst %,clean_%,$(MAKE_DEPENDENCIES))

# these variables are defined internally and passed to submakes
export GLOBAL_DEFINES
export MODULAR_FIRMWARE

$(MAKE_DEPENDENCIES):
	$(call,echo,'Making module $@')
ifneq (,$(filter clean,$(SUBDIR_GOALS)))
	$(VERBOSE)$(MAKE) -C $(PROJECT_ROOT)/$@ clean
ifneq (,$(strip $(filter-out clean,$(SUBDIR_GOALS))))
	$(VERBOSE)$(MAKE) -C $(PROJECT_ROOT)/$@ $(filter-out clean,$(SUBDIR_GOALS))
endif
else
	$(VERBOSE)$(MAKE) -C $(PROJECT_ROOT)/$@ $(SUBDIR_GOALS)
endif

$(CLEAN_DEPENDENCIES):
	$(VERBOSE)$(MAKE) -C $(PROJECT_ROOT)/$(patsubst clean_%,%,$@) clean

# allow recursive invocation across dependencies to make
clean_deps: $(CLEAN_DEPENDENCIES)
make_deps: $(MAKE_DEPENDENCIES)

.PHONY: make_deps clean_deps $(MAKE_DEPENDENCIES) $(CLEAN_DEPENDENCIES)


