
#  Makefile for maestro source code
#  Copyright (C) 2011-2015  Operations division of the Canadian Meteorological Centre
#                           Environment Canada
#
#  Maestro is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation,
#  version 2.1 of the License.
#
#  Maestro is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the
#  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA 02111-1307, USA.

MAKEFILE:=$(lastword $(MAKEFILE_LIST))

# common section definition
include ../config/config.mk

WERROR_FLAGS= -Wall -Werror=implicit-function-declaration -Werror=return-type -Werror=unused-but-set-variable -Werror=unused-variable
CFLAGS = -g -Wall
LIB=-lc -lcrypto 
LIBTH=-lpthread
CFLAGS1 = -g
CFLAGS2 = -lefence -g -I../inc -DREENTRANT -Wall -Wextra -Wno-unused -D__DEBUG -DIGNORE_EMPTY_TEXT_NODES
ROXML_OBJECTS = l2d2_roxml.o l2d2_roxml-internal.o l2d2_roxml-parse-engine.o
L2D2SOBJECTS  = l2d2_server.o l2d2_socket.o l2d2_Util.o l2d2_commun.o l2d2_lists.o $(ROXML_OBJECTS) SeqUtil.o SeqLoopsUtil.o SeqNameValues.o SeqNode.o SeqListNode.o
L2D2AOBJECTS  = l2d2_admin.o l2d2_socket.o l2d2_Util.o l2d2_commun.o l2d2_lists.o $(ROXML_OBJECTS)  SeqUtil.o SeqLoopsUtil.o SeqNameValues.o SeqNode.o SeqListNode.o
OBJECTS=SeqUtil.o SeqNode.o SeqListNode.o SeqNameValues.o SeqLoopsUtil.o SeqDatesUtil.o \
runcontrollib.o nodelogger.o maestro.o nodeinfo.o tictac.o expcatchup.o XmlUtils.o \
QueryServer.o SeqUtilServer.o l2d2_socket.o l2d2_commun.o ocmjinfo.o logreader.o $(ROXML_OBJECTS)
EXECUTABLES=nodelogger maestro nodeinfo tictac expcatchup getdef logreader mserver madmin mtest

#

all: prep 
	$(MAKE) -f $(MAKEFILE) prep $(EXECUTABLES) wrapper

wrapper: 
	rm -rf $(BINDIR)/wrappers
	mkdir -p $(BINDIR)/wrappers
	cd wrappers; for file in *; do \
	    cp $${file} $(BINDIR)/wrappers/maestro_${VERSION}.$${file}; \
	done; \
	cd ..

clean:
	rm -f *.o; rm -f $(EXECUTABLES); rm -rf $(SWDEST) .fo

prep:	
	if [ ! -d $(SWDEST) ] ; then \
	   echo "Creating $(SWDEST)" ; \
	   mkdir -p $(SWDEST); mkdir $(SWDEST)/include; mkdir $(SWDEST)/bin; \
	fi; \

#objects

objects:	$(OBJECTS)

runcontrollib.o:	runcontrollib.c runcontrollib.h nodelogger.h nodeinfo.h
	$(CC) -g -c runcontrollib.c

SeqNode.o:	SeqNode.c SeqNode.h
	$(CC) -g -c SeqNode.c;

SeqListNode.o:	SeqListNode.c
	$(CC) -g -c SeqListNode.c

SeqNameValues.o:	SeqNameValues.c
	$(CC) -g -c SeqNameValues.c

SeqLoopsUtil.o:	SeqLoopsUtil.c
	$(CC) -g -c SeqLoopsUtil.c

SeqDatesUtil.o:	SeqDatesUtil.c
	$(CC) -g -c  SeqDatesUtil.c

XmlUtils.o:	XmlUtils.c
	$(CC) -g -I $(INCDIR) -I $(XML_INCLUDE_DIR) -c XmlUtils.c

SeqUtil.o:	SeqUtil.c SeqUtil.h
	$(CC) -g -c SeqUtil.c 

SeqUtilServer.o: SeqUtilServer.c QueryServer.h
	$(CC) -g -c SeqUtilServer.c
 
QueryServer.o: QueryServer.c QueryServer.h
	$(CC) -g -c QueryServer.c
  
l2d2_socket.o: l2d2_socket.h l2d2_socket.c 
	$(CC) -g -c l2d2_socket.c

expcatchup.o:	expcatchup.c expcatchup.h
	$(CC) -g -I $(INCDIR) -I $(XML_INCLUDE_DIR) -c expcatchup.c

FlowVisitor.o:  FlowVisitor.c FlowVisitor.h nodeinfo.c nodeinfo.h ResourceVisitor.h ResourceVisitor.c
	$(CC) -g $(WERROR_FLAGS) -I $(INCDIR) -I $(XML_INCLUDE_DIR) -c $<

ResourceVisitor.o: ResourceVisitor.c ResourceVisitor.h nodeinfo.c nodeinfo.h
	$(CC) -g $(WERROR_FLAGS) -I $(INCDIR) -I $(XML_INCLUDE_DIR) -c $<

nodeinfo.o:	nodeinfo.c nodeinfo.h FlowVisitor.h runcontrollib.h ResourceVisitor.c ResourceVisitor.h
	$(CC) -g $(WERROR_FLAGS) -I $(INCDIR) -I $(XML_INCLUDE_DIR) -c $<

logreader.o:	logreader.c logreader.h logreader_main.c
	$(CC) -g -c logreader.c

maestro.o:	maestro.c QueryServer.h maestro.h nodeinfo.h runcontrollib.h nodelogger.h tictac.h SeqUtil.h
	$(CC) -g -c maestro.c -I $(XML_INCLUDE_DIR)

ocmjinfo.o:	ocmjinfo.c ocmjinfo.h 
	$(CC) -g -c ocmjinfo.c

l2d2_lists.o: l2d2_lists.h l2d2_lists.c
	$(CC) -c l2d2_lists.c

l2d2_Util.o: l2d2_Util.c l2d2_Util.h
	$(CC) -c l2d2_Util.c
 
l2d2_commun.o: l2d2_commun.c l2d2_commun.h
	$(CC) -c l2d2_commun.c
 
l2d2_admin.o: l2d2_admin.c
	$(CC) -c l2d2_admin.c
 
l2d2_server.o: l2d2_server.c l2d2_server.h
	$(CC) -c l2d2_server.c
 
l2d2_roxml.o: l2d2_roxml.c
	$(CC)  $(CFLAGS2) -c l2d2_roxml.c
 
l2d2_roxml-internal.o: l2d2_roxml-internal.c
	$(CC)  $(CFLAGS2) -c l2d2_roxml-internal.c
 
l2d2_roxml-parse-engine.o: l2d2_roxml-parse-engine.c
	$(CC)  $(CFLAGS2) -c l2d2_roxml-parse-engine.c
 
nodelogger.o:	nodelogger.c nodelogger.h SeqUtil.c l2d2_commun.c l2d2_Util.c l2d2_socket.c
	$(CC) -g -c nodelogger.c 

tictac.o:	tictac.c tictac.h
	$(CC) -g -c tictac.c

getopt_long.o:	getopt_long.c getopt.h
	$(CC) -c getopt_long.c

# Targets for command line executables:
#
# For reference (I always forget them):
# $^ is the whole prerequisite list
# $@ is the target name
# $< the name of the first prerequisite

TICTAC_OBJECTS = tictac.o SeqUtil.o QueryServer.o l2d2_socket.o SeqUtilServer.o \
	l2d2_commun.o SeqListNode.o getopt_long.o

tictac:	tictac_main.c $(TICTAC_OBJECTS)
	$(CC) -g $^ -L$(XML_LIB_DIR) -lxml2 $(LIB) -o tictac
	cp tictac $(BINDIR)

NODELOGGER_OBJECTS = nodelogger.o SeqUtil.o l2d2_commun.o SeqUtilServer.o \
	l2d2_socket.o QueryServer.o tictac.o nodeinfo.o SeqNode.o SeqLoopsUtil.o \
	XmlUtils.o SeqNameValues.o SeqListNode.o SeqDatesUtil.o getopt_long.o \
	FlowVisitor.o ResourceVisitor.o

nodelogger: nodelogger_main.c $(NODELOGGER_OBJECTS)
	$(CC) -g $^ -I $(XML_INCLUDE_DIR) -L$(XML_LIB_DIR) -lxml2 $(LIB) -I$(INCDIR) -o nodelogger
	cp nodelogger $(BINDIR)

logreader:	logreader.o SeqUtil.o SeqDatesUtil.o l2d2_commun.o SeqListNode.o logreader_main.c getopt_long.o
	$(CC) -g logreader.o SeqUtil.o SeqDatesUtil.o l2d2_commun.o SeqListNode.o getopt_long.o logreader_main.c $(LIB) -o logreader
	cp logreader $(BINDIR)

MAESTRO_OBJECTS = maestro.o logreader.o nodelogger.o tictac.o nodeinfo.o \
	SeqNode.o SeqLoopsUtil.o XmlUtils.o SeqNameValues.o SeqListNode.o SeqDatesUtil.o \
	SeqUtil.o l2d2_commun.o SeqUtilServer.o QueryServer.o l2d2_socket.o \
	runcontrollib.o ocmjinfo.o expcatchup.o getopt_long.o ResourceVisitor.o \
	FlowVisitor.o

maestro: maestro_main.c $(MAESTRO_OBJECTS)
	$(CC) -g $^ -I $(INCDIR) -L $(XML_LIB_DIR) -lxml2 $(LIB) -o maestro; \
	cp maestro $(BINDIR);

expcatchup:	expcatchup.o getopt_long.o SeqUtil.o expcatchup_main.c XmlUtils.o SeqListNode.o l2d2_commun.o
	$(CC) -g getopt_long.o expcatchup.o SeqUtil.o expcatchup_main.c XmlUtils.o SeqListNode.o l2d2_commun.o -L $(XML_LIB_DIR) -lxml2 $(LIB) -o expcatchup; \
	cp expcatchup $(BINDIR);

NODEINFO_OBJECTS = SeqUtil.o SeqNode.o XmlUtils.o SeqLoopsUtil.o l2d2_commun.o \
	QueryServer.o l2d2_socket.o SeqListNode.o SeqDatesUtil.o SeqUtilServer.o \
	tictac.o SeqNameValues.o nodeinfo.o getopt_long.o FlowVisitor.o \
	ResourceVisitor.o

nodeinfo: nodeinfo_main.c $(NODEINFO_OBJECTS)
	$(CC) -g $^ -I $(XML_INCLUDE_DIR) -L $(XML_LIB_DIR) -lxml2 $(LIB) -o nodeinfo 
	cp nodeinfo $(BINDIR)

getdef:	getdef_main.o SeqUtil.o SeqListNode.o l2d2_commun.o getopt_long.o
	$(CC) -g -c getdef_main.c 
	$(CC) -g getdef_main.o SeqUtil.o SeqListNode.o l2d2_commun.o getopt_long.o $(LIB) -o getdef
	cp getdef $(BINDIR)

mserver: $(L2D2SOBJECTS)
	$(CC) $(L2D2SOBJECTS) $(LIB) $(LIBTH) -o mserver; \
	cp mserver $(BINDIR);

madmin: $(L2D2AOBJECTS)
	$(CC) $(L2D2AOBJECTS) $(LIB) $(LIBTH) -o madmin; \
	cp madmin $(BINDIR);


TEST_OBJECTS = SeqUtil.o SeqNode.o XmlUtils.o SeqLoopsUtil.o l2d2_commun.o \
	QueryServer.o l2d2_socket.o SeqListNode.o SeqDatesUtil.o SeqUtilServer.o \
	tictac.o SeqNameValues.o nodeinfo.o getopt_long.o FlowVisitor.o \
	ResourceVisitor.o

mtest:	mtest_main.c $(TEST_OBJECTS)
	$(CC) -g $(WERROR_FLAGS) $^ -I $(XML_INCLUDE_DIR) -L $(XML_LIB_DIR) -lxml2 $(LIB) -o $@
	cp $@ $(BINDIR)
