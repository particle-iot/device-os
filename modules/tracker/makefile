include modular.mk

GOALS := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)
.PHONY: $(GOALS) submake
$(GOALS): submake ;
submake:
	$(MAKE) -C .. $(MAKECMDGOALS)
