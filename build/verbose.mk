# Macros for controlling verbosity
# definition of verbose variable 'v' used to enable verbose output
# $(VERBOSE) expands to empty or @ to suppress echoing commands in recipes
# $(call,echo,text) conditionally outputs text if verbose is enabled

ifdef v
ECHO=echo
VERBOSE=
VERBOSE_REDIRECT=
else
ECHO = true
VERBOSE=@
MAKE_ARGS += -s
VERBOSE_REDIRECT= > /dev/null
endif

echo=@$(ECHO) $1
