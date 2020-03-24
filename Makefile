include Makefile.inc

PKG_CONFIG_PKGS = substrate
CFLAGS_EXTRA = -MMD -MF .dep/$*.d $(shell mysql_config --include) $(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
CFLAGS = -c $(OPTIM_FLAGS) -Wall -Wextra -pedantic -std=c++11 -I. $(CFLAGS_EXTRA) -o $@ $<
LIBS = $(shell mysql_config --libs) -lodbc -ldl -pthread $(shell pkg-config --libs $(PKG_CONFIG_PKGS))
LFLAGS = $(OPTIM_FLAGS) -shared $(O) $(LIBS) -o $@

SED = sed -e 's:@LIBDIR@:$(LIBDIR):g' -e 's:@PREFIX@:$(PREFIX):g' -e 's:@VERSION@:$(VER):g'

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib
PKGDIR = $(LIBDIR)/pkgconfig
INCDIR = $(PREFIX)/include/tmplORM
SER_INCDIR = $(INCDIR)/serializer

TYPE_INCDIR = $(PREFIX)/include/typestring
TYPE_H = typestring/typestring.hh

O = string.o mysql.o mssql.o tmplORM.types.o
H = mysql.hxx mssql.hxx tmplORM.hxx tmplORM.mysql.hxx tmplORM.mssql.hxx tmplORM.common.hxx tmplORM.types.hxx \
	tmplORM.extern.hxx conversions.hxx fixedVector.hxx string.hxx managedPtr.hxx
H_SERIALIZER = json.hxx fromJSON.hxx toJSON.hxx helpers.hxx
GCH = tmplORM.gch tmplORM.mysql.gch tmplORM.mssql.gch
VERMAJ = .0
VERMIN = $(VERMAJ).2
VERREV = $(VERMIN).0
VER = $(VERREV)
SO = libtmplORM.so
PC = tmplORM.pc

DEPS = .dep

default: all

all: $(DEPS) $(SO) $(GCH)

$(DEPS) $(LIBDIR) $(PKGDIR) $(INCDIR) $(SER_INCDIR) $(TYPE_INCDIR):
	$(call run-cmd,install_dir,$@)

install: all $(LIBDIR) $(PKGDIR) $(INCDIR) $(SER_INCDIR) $(TYPE_INCDIR) $(PC)
	$(call run-cmd,install_file,$(addsuffix $(VER),$(SO)),$(LIBDIR))
	$(call run-cmd,install_file,$(PC),$(PKGDIR))
	$(call run-cmd,install_file,$(H),$(INCDIR))
	$(call run-cmd,install_file,$(addprefix serializer/, $(H_SERIALIZER)),$(SER_INCDIR))
	$(call run-cmd,install_file,$(TYPE_H),$(TYPE_INCDIR))
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
	$(call run-cmd,cxx,$(CFLAGS))

%.gch: %.hxx
	$(call run-cmd,cxx,$(CFLAGS))

buildCheck: LIBS += $(shell pkg-config --cflags --libs rSON)
buildCheck: buildCheck.o
	$(call run-cmd,ccld,$(OPTIM_FLAGS) $(O) $(LIBS) -o $@ buildCheck.o)

clean:
	$(call run-cmd,rm,tmplORM,$(O) $(SO)* $(GCH) buildCheck.o)
	$(call run-cmd,rm,makedep,.dep/*.d)
	@$(MAKE) -C test clean

tests: all test
	@$(MAKE) -C test

check: all test
	@$(MAKE) -C test check

#mysql.o: CFLAGS_EXTRA += $(shell mysql_config --include)
.PHONY: default all clean tests check install
.SUFIXES: .cxx .hxx .o .gch
-include .dep/*.d
