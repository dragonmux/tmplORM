include Makefile.inc

PKG_CONFIG_PKGS =
CFLAGS_EXTRA = $(shell mysql_config --include)
#$(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
DEFS = $(OPTIM_FLAGS) -Wall -Wextra -pedantic -std=c++11 $(CFLAGS_EXTRA)
CFLAGS = -c $(DEFS) -o $@ $<
DEPFLAGS = -E -MM $(DEFS) -o .dep/$*.d $<
LIBS = $(shell mysql_config --libs) -lodbc -ldl -pthread
#$(shell pkg-config --libs $(PKG_CONFIG_PKGS))
LFLAGS = $(OPTIM_FLAGS) -shared $(O) $(LIBS) -o $@
ifeq ($(strip $(FOR_TESTS)), 1)
	DEFS += -I. -Itest
endif

SED = sed -e 's:@LIBDIR@:$(LIBDIR):g' -e 's:@PREFIX@:$(PREFIX):g' -e 's:@VERSION@:$(VER):g'

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib
PKGDIR = $(LIBDIR)/pkgconfig
INCDIR = $(PREFIX)/include/tmplORM

O = string.o mysql.o mssql.o
H = mysql.hxx mssql.hxx tmplORM.hxx tmplORM.mysql.hxx tmplORM.mssql.hxx tmplORM.common.hxx tmplORM.types.hxx tmplORM.rSON.hxx tmplORM.extern.hxx
GCH = tmplORM.gch tmplORM.mysql.gch tmplORM.mssql.gch
VERMAJ = .0
VERMIN = $(VERMAJ).0
VERREV = $(VERMIN).1
VER = $(VERREV)
SO = libtmplORM.so
PC = tmplORM.pc

DEPS = .dep

default: all

all: $(DEPS) $(SO) $(GCH)

$(DEPS) $(LIBDIR) $(PKGDIR) $(INCDIR):
	$(call run-cmd,install_dir,$@)

install: all $(LIBDIR) $(PKGDIR) $(INCDIR) $(PC)
	$(call run-cmd,install_file,$(addsuffix $(VER),$(SO)),$(LIBDIR))
	$(call run-cmd,install_file,$(PC),$(PKGDIR))
	$(call run-cmd,install_file,$(H),$(INCDIR))
	$(call run-cmd,ln,$(LIBDIR)/$(SO)$(VERREV),$(LIBDIR)/$(SO)$(VERMIN))
	$(call run-cmd,ln,$(LIBDIR)/$(SO)$(VERMIN),$(LIBDIR)/$(SO)$(VERMAJ))
	$(call run-cmd,ln,$(LIBDIR)/$(SO)$(VERMAJ),$(LIBDIR)/$(SO))
	$(call ldconfig)

$(SO): $(O)
	$(call run-cmd,ccld,$(LFLAGS))
	$(call debug-strip,$@)
	$(call run-cmd,ln,$@,$@$(VER))

%.pc: %.pc.in
	$(call run-cmd,sed,$<,$@)

%.o: %.cxx | $(DEPS)
	$(call makedep,$(CXX),$(DEPFLAGS))
	$(call run-cmd,cxx,$(CFLAGS))

%.gch: %.hxx
	$(call run-cmd,cxx,$(CFLAGS))

buildCheck: buildCheck.o
	$(call run-cmd,ccld,$(OPTIM_FLAGS) $(O) $(LIBS) -o $@ buildCheck.o)

clean:
	$(call run-cmd,rm,tmplORM,$(O) $(SO)* $(GCH) buildCheck.o)
	$(call run-cmd,rm,makedep,.dep/*.d)

tests: all test
	@$(MAKE) -C test

check: all test
	@$(MAKE) -C test check

#mysql.o: CFLAGS_EXTRA += $(shell mysql_config --include)
.PHONY: default all clean tests check install
.SUFIXES: .cxx .hxx .o .gch
-include .dep/*.d
