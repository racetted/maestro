# this file contains common config that is sourced by all makefiles
WHO_I_AM=$(shell whoami)
VERSION=1.0.8
MACHINE=$(shell uname -s)
ARCH=$(MACHINE)
HARDWARE=$(shell uname -m)
SWDEST=$(shell pwd)/../bin/$(ARCH)
LIBDIR=$(SWDEST)/lib
INCDIR=$(SWDEST)/include
BINDIR=$(SWDEST)/bin
OBJECTS=SeqUtil.o SeqNode.o SeqListNode.o SeqNameValues.o SeqLoopsUtil.o SeqDatesUtil.o \
runcontrollib.o nodelogger.o maestro.o nodeinfo.o tictac.o expcatchup.o XmlUtils.o
COMPONENTS=nodelogger maestro nodeinfo tictac expcatchup
XTERN_LIB=$(ARMNLIB)/lib/$(BASE_ARCH)
# platform specific definition
LIBNAME="xml2 runcontrol z rmn"
XML_INCLUDE_DIR=/usr/include/libxml2
XML_LIB_DIR=/usr/lib
ifeq ($(MACHINE),Linux)
   MACH=op_linux
   SSM_MACH_ID=linux26-$(HARDWARE)
   SSMPACKAGE=maestro_$(VERSION)_$(SSM_MACH_ID)
   ifeq ($(HARDWARE),x86_64)
      LIBNAME="xml2 runcontrol z rmn_012"
   endif
else 
   ifeq ($(MACHINE),AIX)
      MACH=op_b
      SSM_MACH_ID=aix53-ppc-64
      SSMPACKAGE=maestro_$(VERSION)_$(SSM_MACH_ID)
      # only temp until it is insalled on /usr/lib
      #XML_INCLUDE_DIR=/data/run_control/afsisul/software/aix/include/libxml2/
      #XML_LIB_DIR=/data/run_control/afsisul/software/aix/lib
   endif
endif
