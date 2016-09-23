include Makefile.inc

PKG_CONFIG_PKGS = 
CFLAGS_EXTRA = $(shell mysql_config --include)
#$(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
DEFS = $(OPTIM_FLAGS) -Wall -Wextra -pedantic -std=c++11 $(CFLAGS_EXTRA)
CFLAGS = -c $(DEFS) -o $@ $<
DEPFLAGS = -E -MM $(DEFS) -o .dep/$*.d $<
LIBS = $(shell mysql_config --libs)
#$(shell pkg-config --libs $(PKG_CONFIG_PKGS))
LFLAGS = $(OPTIM_FLAGS) -shared $(O) $(LIBS) -o $@

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib

O = string.o mysql.o
GCH = tmplORM.gch
SO = libtmplORM.so

DEPS = .dep

default: all

all: $(DEPS) $(SO) $(GCH)

$(DEPS):
	$(call run-cmd,install_dir,$@)

$(SO): $(O)
	$(call run-cmd,ccld,$(LFLAGS))
	$(call debug-strip,$(SO))

%.o: %.cxx $(DEPS)
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

%.gch: %.hxx
	$(call run-cmd,cxx,$(CFLAGS))

clean:
	$(call run-cmd,rm,tmplORM,$(O) $(SO))
	$(call run-cmd,rm,makedep,.dep/*.d)

.PHONY: default all clean
.SUFIXES: .cxx .hxx
-include .dep/*.d
