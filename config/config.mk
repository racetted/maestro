# this file contains common config that is sourced by all makefiles
WHO_I_AM=$(shell whoami)
VERSION=1.4.1
SEQ_WRAPPER=maestro_$(VERSION)
MACHINE=$(shell uname -s)
HARDWARE=$(shell uname -m | tr '_' '-')
SWDEST=$(shell pwd)/../bin/$(BASE_ARCH)
LIBDIR=$(SWDEST)/lib
INCDIR=$(SWDEST)/include
BINDIR=$(SWDEST)/bin
OBJECTS=SeqUtil.o SeqNode.o SeqListNode.o SeqNameValues.o SeqLoopsUtil.o SeqDatesUtil.o \
runcontrollib.o nodelogger.o maestro.o nodeinfo.o tictac.o expcatchup.o XmlUtils.o \
QueryServer.o SeqUtilServer.o l2d2_socket.o l2d2_commun.o ocmjinfo.o logreader.o
COMPONENTS=nodelogger maestro nodeinfo tictac expcatchup getdef logreader
XTERN_LIB=$(ARMNLIB)/lib/$(BASE_ARCH)
# platform specific definition
XML_INCLUDE_DIR=/usr/include/libxml2
XML_LIB_DIR=/usr/lib
LIBNAME="xml2 runcontrol rmn_014 crypto"

ifeq ($(MACHINE),Linux)
   MACH=op_linux
   SSM_MACH_ID=linux26-$(HARDWARE)
   COMPILER_SSM_CMD=". ssmuse-sh -d hpcs/201402/00/base -d hpcs/201402/00/intel13sp1 -d rpn/libs/4.0"
   ifeq ($(HARDWARE),i686)
      #overwrite for 32-bit platform
      LIBNAME="xml2 runcontrol rmn_013 crypto"
      COMPILER_SSM_CMD=". s.ssmuse.dot DEV/rmnlib-dev DEV/devtools DEV/pgi9xx DEV/legacy"
   endif
else 
   MACH=op_b
   COMPILER_SSM_CMD=". ssmuse-sh -d hpcs/13b/03/base -d rpn/libs/4.0 -d hpcs/ext/xlf_13.1.0.10"
   ifeq ($(BASE_ARCH),AIX-powerpc7)
       SSM_MACH_ID=aix61-ppc-64
   endif
endif

SSMPACKAGE=maestro_$(VERSION)_$(SSM_MACH_ID)
