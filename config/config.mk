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
   CC = icc
   COMPILER_SSM_CMD=". ssmuse-sh -d hpcs/201402/02/base -d hpcs/201402/02/intel13sp1u2"
   ifeq ($(HARDWARE),i686)
     #overwrite for 32-bit platform
      COMPILER_SSM_CMD=". s.ssmuse.dot DEV/rmnlib-dev DEV/devtools DEV/pgi9xx DEV/legacy"
   endif
else 
   CC = cc
   COMPILER_SSM_CMD=". ssmuse-sh -d hpcs/201402/02/base -d hpcs/ext/xlf_13.1.0.10"
endif

SSMPACKAGE=maestro_$(VERSION)_$(ORDENV_PLAT)
