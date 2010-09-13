# this file contains common config that is sourced by all makefiles
WHO_I_AM=$(shell whoami)
VERSION=1.0.3
MACHINE=$(shell uname -s)
ARCH=$(MACHINE)
SWDEST=$(shell pwd)/../bin/$(ARCH)
LIBDIR=$(SWDEST)/lib
INCDIR=$(SWDEST)/include
BINDIR=$(SWDEST)/bin
OBJECTS=SeqUtil.o SeqNode.o SeqListNode.o SeqNameValues.o SeqLoopsUtil.o SeqDatesUtil.o \
runcontrollib.o nodelogger.o maestro.o nodeinfo.o tictac.o
COMPONENTS=nodelogger maestro nodeinfo tictac
XTERN_LIB=$(ARMNLIB)/lib/$(MACHINE)
# platform specific definition
LIBNAME="runcontrol xml2 z rmn"
ifeq ($(MACHINE),Linux)
   MACH=op_linux
   SSM_MACH_ID=linux26-i686
   SSMPACKAGE=maestro_$(VERSION)_$(SSM_MACH_ID)
   XML_DIR=/home/binops/afsi/ssm/sw/tcl-tk_8.4.13.2_linux26-i686
else 
   ifeq ($(MACHINE),AIX)
      MACH=op_b
      SSM_MACH_ID=aix53-ppc-64
      SSMPACKAGE=maestro_$(VERSION)_$(SSM_MACH_ID)
      XML_DIR=/data/run_control/afsisul/software/aix
   endif
endif
