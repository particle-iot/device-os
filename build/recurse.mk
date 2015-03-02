# Handles recursive make calls to dependent modules

# create a list of targets to clean from the list of dependencies
CLEAN_DEPENDENCIES=$(patsubst %,clean_%,$(MAKE_DEPENDENCIES))
	
$(MAKE_DEPENDENCIES):
	$(call,echo,'Making module $@')
	$(VERBOSE)$(MAKE) -C $(PROJECT_ROOT)/$@ $(SUBDIR_GOALS) $(MAKE_ARGS) $(MAKEOVERRIDES) submake=1

$(CLEAN_DEPENDENCIES):	
	$(VERBOSE)$(MAKE) -C $(PROJECT_ROOT)/$(patsubst clean_%,%,$@) clean $(MAKE_ARGS) $(MAKEOVERRIDES)  submake=1
		
# allow recursive invocation across dependencies to make
clean_deps: $(CLEAN_DEPENDENCIES)
make_deps: $(MAKE_DEPENDENCIES)
	
.PHONY: make_deps clean_deps $(MAKE_DEPENDENCIES) $(CLEAN_DEPENDENCIES)
	

