# Macros for controlling verbosity
# definition of verbose variable 'v' used to enable verbose output
# $(VERBOSE) expands to empty or @ to suppress echoing commands in recipes
# $(call,echo,text) conditionally outputs text if verbose is enabled

ifeq ("$(subst s,,$(MAKEFLAGS))","$(MAKEFLAGS)")
v=1
else
v=0
endif

ifeq ("$(v)","1")
ECHO=echo
VERBOSE=
VERBOSE_REDIRECT=
else
ECHO = true
VERBOSE=@
VERBOSE_REDIRECT= > /dev/null 2>&1
endif

echo=@$(ECHO) $1
