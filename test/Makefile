include ../Makefile.inc

CFLAGS_EXTRA = -I..
CFLAGS = -c $(OPTIM_FLAGS) -Wall -Wextra -pedantic -std=c++11 $(CFLAGS_EXTRA) -o $@ $<

CRUNCHMAKE = crunchMake -I..
# $(shell pkg-config --cflags --libs crunch++ zlib)
ifeq ($(BUILD_VERBOSE), 0)
	CRUNCHMAKE += -q
endif
CRUNCH = crunch++

TESTS = testMSSQLMapper.so testMySQLMapper.so
GCH = models.gch

quiet_cmd_crunchMake = -n
cmd_crunchMake = $(CRUNCHMAKE) $(addprefix -o,$(O_FILE)) $(2)
quiet_cmd_crunch = -n
cmd_crunch = $(CRUNCH) $(2)

default: all

all: $(GCH) $(TESTS)

%.gch: %.hxx
	$(call run-cmd,cxx,$(CFLAGS))

#testMySQL.so: O_FILE = ../mysql.o
#testMSSQL.so: O_FILE = ../mssql.o
#testMySQL.so
testMySQLMapper.so: CRUNCHMAKE += $(shell mysql_config --cflags --libs)
$(TESTS): $(subst .so,.cxx,$@)
	$(call run-cmd,crunchMake,$(subst .so,.cxx,$@))

check: all
	$(call run-cmd,crunch,$(subst .so,,$(TESTS)))

clean:
	$(call run-cmd,rm,tests,$(TESTS) $(GCH))
#	$(call run-cmd,rm,makedep,.dep/*.d)

.PHONY: default all check clean
.SUFIXES: .cxx .hxx .so .o
#-include .dep/*.d
