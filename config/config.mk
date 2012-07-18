# this file contains common config that is sourced by all makefiles
WHO_I_AM=$(shell whoami)
VERSION=1.2.0
SEQ_WRAPPER=maestro_1.0.0
MACHINE=$(shell uname -s)
HARDWARE=$(shell uname -m | tr '_' '-')
SWDEST=$(shell pwd)/../bin/$(BASE_ARCH)
LIBDIR=$(SWDEST)/lib
INCDIR=$(SWDEST)/include
BINDIR=$(SWDEST)/bin
OBJECTS=SeqUtil.o SeqNode.o SeqListNode.o SeqNameValues.o SeqLoopsUtil.o SeqDatesUtil.o \
runcontrollib.o nodelogger.o maestro.o nodeinfo.o tictac.o expcatchup.o XmlUtils.o
COMPONENTS=nodelogger maestro nodeinfo tictac expcatchup getdef
XTERN_LIB=$(ARMNLIB)/lib/$(BASE_ARCH)
# platform specific definition
LIBNAME="xml2 runcontrol z rmn_012"
XML_INCLUDE_DIR=/usr/include/libxml2
XML_LIB_DIR=/usr/lib
ifeq ($(MACHINE),Linux)
   MACH=op_linux
   SSM_MACH_ID=linux26-$(HARDWARE)
else 
   ifeq ($(MACHINE),AIX)
       MACH=op_b
       SSM_MACH_ID=aix53-ppc-64
   endif
   ifeq ($(BASE_ARCH),AIX-powerpc7) 
       SSM_MACH_ID=aix61-ppc-64
   endif
endif

SSMPACKAGE=maestro_$(VERSION)_$(SSM_MACH_ID)
