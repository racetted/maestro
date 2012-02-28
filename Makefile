##OBJECT: runcontrol-1.0
#
##LIBRARIES: standard "c"
#
##MODULES:  runcontrol-1.0
#
##MACHINE:  op_f
#
##

# ATTENTION!
# YOU NEED TO USE THE GMAKE IN /users/dor/afsi/sul/ovbin/gmake for this
# Makefile to work, other versions installed are currently too old!
#
# common section definition
include config/config.mk

all: install
install: clean
	cd src; $(MAKE); cd ..
	cd ssm; $(MAKE) ssm; cd ..
clean:
	cd src; $(MAKE) clean; cd ..
	cd ssm; $(MAKE) clean; cd ..
