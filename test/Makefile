include ../Makefile.inc

CFLAGS_EXTRA = -I..
CFLAGS = -c $(OPTIM_FLAGS) -Wall -Wextra -pedantic -std=c++11 $(CFLAGS_EXTRA) -o $@ $<

TEST = 
GCH = models.gch

default: all

all: $(GCH) $(TEST)

%.gch: %.hxx
	$(call run-cmd,cxx,$(CFLAGS))

clean:
	$(call run-cmd,rm,tmplORM,$(TEST) $(GCH))
#	$(call run-cmd,rm,makedep,.dep/*.d)

.PHONY: default all clean
.SUFIXES: .cxx .hxx
#-include .dep/*.d
