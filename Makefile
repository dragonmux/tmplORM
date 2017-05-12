include Makefile.inc

PKG_CONFIG_PKGS =
CFLAGS_EXTRA = $(shell mysql_config --include)
#$(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
DEFS = $(OPTIM_FLAGS) -Wall -Wextra -pedantic -std=c++11 $(CFLAGS_EXTRA)
CFLAGS = -c $(DEFS) -o $@ $<
DEPFLAGS = -E -MM $(DEFS) -o .dep/$*.d $<
LIBS = $(shell mysql_config --libs) -lodbc
#$(shell pkg-config --libs $(PKG_CONFIG_PKGS))
LFLAGS = $(OPTIM_FLAGS) -shared $(O) $(LIBS) -o $@

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib

O = string.o mysql.o mssql.o
GCH = tmplORM.gch tmplORM.mysql.gch tmplORM.mssql.gch
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

buildCheck: buildCheck.o
	$(call run-cmd,ccld,$(OPTIM_FLAGS) $(O) $(LIBS) -o $@ buildCheck.o)

clean:
	$(call run-cmd,rm,tmplORM,$(O) $(SO) $(GCH))
	$(call run-cmd,rm,makedep,.dep/*.d)

#mysql.o: CFLAGS_EXTRA += $(shell mysql_config --include)
.PHONY: default all clean test
.SUFIXES: .cxx .hxx
-include .dep/*.d
