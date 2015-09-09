# this file contains common config that is sourced by all makefiles
WHO_I_AM=$(shell whoami)
VERSION=1.4.3
SEQ_WRAPPER=maestro_$(VERSION)
MACHINE=$(shell uname -s)
HARDWARE=$(shell uname -m | tr '_' '-')
SWDEST=$(shell pwd)/../bin/$(ORDENV_PLAT)
LIBDIR=$(SWDEST)/lib
INCDIR=$(SWDEST)/include
BINDIR=$(SWDEST)/bin
XML_INCLUDE_DIR=/usr/include/libxml2
XML_LIB_DIR=/usr/lib

# platform specific definition
ifeq ($(MACHINE),Linux)
   CC = cc
else 
   CC = cc
endif

SSMPACKAGE=maestro_$(VERSION)_$(ORDENV_PLAT)
