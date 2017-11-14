include ../Makefile.inc

CFLAGS_EXTRA = -I..
CFLAGS = -c $(OPTIM_FLAGS) -Wall -Wextra -pedantic -std=c++11 $(CFLAGS_EXTRA) -o $@ $<

CRUNCHMAKE = crunchMake -I.. -z defs
# $(shell pkg-config --cflags --libs crunch++ zlib)
ifeq ($(BUILD_VERBOSE), 0)
	CRUNCHMAKE += -q
endif
ifeq ($(strip $(COVERAGE)), 1)
	CRUNCHMAKE += -lgcov
endif
CRUNCH = crunch++

TESTS = testString.so testMySQL.so testMSSQLMapper.so testMySQLMapper.so testConversions.so testFixedVector.so
O = conversions.o fixedVector.o
GCH = models.gch
DEPS = ../.dep/test

quiet_cmd_crunchMake = -n
cmd_crunchMake = $(CRUNCHMAKE) $(addprefix -o,$(O_FILE)) $(2)
quiet_cmd_crunch = -n
cmd_crunch = $(CRUNCH) $(2)

default: all
all: $(GCH) $(DEPS) $(TESTS)

$(DEPS):
	$(call run-cmd,install_dir,$@)

%.gch: %.hxx
	$(call run-cmd,cxx,$(CFLAGS))

testString.so: O_FILE = ../string.o
testMySQL.so: O_FILE = ../mysql.o ../string.o
#testMSSQL.so: O_FILE = ../mssql.o
testMySQLMapper.so testMySQL.so: CRUNCHMAKE += $(shell mysql_config --include --libmysqld-libs)
testConversions.so: O_FILE = conversions.o
testFixedVector.so: O_FILE = fixedVector.o
testConversions.so: conversions.o testConversions.cxx
testFixedVector.so: fixedVector.o testFixedVector.cxx
#$(TESTS): $(subst .so,.cxx,$@)
%.so: %.cxx
	$(call run-cmd,crunchMake,$(subst .so,.cxx,$@))

$(O): $(subst .o,.cxx,$@) | $(DEPS)
	$(Q)$(MAKE) FOR_TESTS=1 -C .. test/$@

check: all
	$(call run-cmd,crunch,$(subst .so,,$(TESTS)))

clean:
	$(call run-cmd,rm,tests,$(TESTS) $(GCH))
#	$(call run-cmd,rm,makedep,.dep/*.d)

.PHONY: default all check clean
.SUFIXES: .cxx .hxx .so .o
-include .dep/test/*.d
