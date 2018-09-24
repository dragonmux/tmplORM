include ../Makefile.inc

CFLAGS_EXTRA = -I.. -MMD -MF ../.dep/test/$*.d
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

TESTS = testString.so testMSSQL.so testMySQL.so testMSSQLMapper.so testMySQLMapper.so testConversions.so \
	testFixedVector.so testTypes.so testDateTime.so
O = conversions.o fixedVector.o tmplORM.types.o
GCH = models.gch
DEPS = ../.dep/test

quiet_cmd_crunchMake = -n
cmd_crunchMake = $(CRUNCHMAKE) $(O_FILE) $(2)
quiet_cmd_crunch = -n
cmd_crunch = $(CRUNCH) $(2)

default: all
all: $(GCH) $(DEPS) $(TESTS)

$(DEPS):
	$(call run-cmd,install_dir,$@)

%.gch: %.hxx | $(DEPS)
	$(call run-cmd,cxx,$(CFLAGS))

testString.so: O_FILE = ../string.o
testMySQL.so: O_FILE = ../mysql.o ../string.o ../tmplORM.types.o
testMSSQL.so: O_FILE = ../mssql.o ../string.o ../tmplORM.types.o
testMySQLMapper.so testMySQL.so: CRUNCHMAKE += $(shell mysql_config --include --libs)
testMSSQL.so: CRUNCHMAKE += -lodbc
testConversions.so: O_FILE = conversions.o
testFixedVector.so: O_FILE = fixedVector.o
testTypes.so: O_FILE = tmplORM.types.o ../string.o ../tmplORM.types.o
testDateTime.so: O_FILE = ../string.o
testMSSQL.so: constString.hxx
testConversions.so: conversions.o
testFixedVector.so: fixedVector.o
testTypes.so: tmplORM.types.o
%.so: $(O_FILE)
#$(TESTS): $(subst .so,.cxx,$@)
%.so: %.cxx
	$(call run-cmd,crunchMake,$(subst .so,.cxx,$@))

$(O): CFLAGS_EXTRA += -I. $(shell pkg-config --cflags crunch++)
%.o: %.cxx | $(DEPS)
	$(call run-cmd,cxx,$(CFLAGS))

check: all
	$(call run-cmd,crunch,$(subst .so,,$(TESTS)))

clean:
	$(call run-cmd,rm,tests,$(TESTS) $(GCH))
#	$(call run-cmd,rm,makedep,.dep/*.d)

.PHONY: default all check clean
.SUFIXES: .cxx .hxx .so .o
-include .dep/test/*.d
