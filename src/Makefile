MAKEFILE:=$(lastword $(MAKEFILE_LIST))

# common section definition
include ../config/config.mk

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
EXECUTABLES=nodelogger maestro nodeinfo tictac expcatchup getdef logreader mserver madmin

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

nodeinfo.o:	nodeinfo.c nodeinfo.h runcontrollib.h
	$(CC) -g -I $(INCDIR) -I $(XML_INCLUDE_DIR) -c nodeinfo.c

logreader.o:	logreader.c logreader.h logreader_main.c
	$(CC) -g -c logreader.c

maestro.o:	maestro.c QueryServer.h maestro.h nodeinfo.h runcontrollib.h nodelogger.h tictac.h SeqUtil.h
	$(CC) -g -c maestro.c 

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

# Targets for command line executables:

tictac:	 tictac.o SeqUtil.o QueryServer.o l2d2_socket.o SeqUtilServer.o l2d2_commun.o tictac_main.c 
	$(CC) -g tictac.o SeqUtil.o QueryServer.o l2d2_socket.o SeqUtilServer.o l2d2_commun.o tictac_main.c -L$(XML_LIB_DIR) -lxml2 $(LIB) -o tictac
	cp tictac $(BINDIR)

nodelogger:	nodelogger.o SeqUtil.o l2d2_commun.o SeqUtilServer.o l2d2_socket.o QueryServer.o tictac.o nodeinfo.o SeqNode.o SeqLoopsUtil.o XmlUtils.o SeqNameValues.o SeqListNode.o SeqDatesUtil.o nodelogger_main.c
	$(CC) -g nodelogger.o tictac.o nodeinfo.o SeqNode.o SeqLoopsUtil.o XmlUtils.o SeqNameValues.o SeqListNode.o SeqDatesUtil.o SeqUtil.o l2d2_commun.o SeqUtilServer.o QueryServer.o l2d2_socket.o nodelogger_main.c -L$(XML_LIB_DIR) -lxml2 $(LIB) -o nodelogger
	cp nodelogger $(BINDIR)

logreader:	logreader.o SeqUtil.o SeqDatesUtil.o l2d2_commun.o logreader_main.c
	$(CC) -g logreader.o SeqUtil.o SeqDatesUtil.o l2d2_commun.o logreader_main.c $(LIB) -o logreader
	cp logreader $(BINDIR)

maestro:	maestro.o logreader.o nodelogger.o tictac.o nodeinfo.o SeqNode.o SeqLoopsUtil.o XmlUtils.o SeqNameValues.o SeqListNode.o SeqDatesUtil.o SeqUtil.o l2d2_commun.o SeqUtilServer.o QueryServer.o l2d2_socket.o runcontrollib.o ocmjinfo.o expcatchup.o maestro_main.c
	$(CC) -g maestro.o logreader.o nodelogger.o tictac.o nodeinfo.o SeqNode.o SeqLoopsUtil.o XmlUtils.o SeqNameValues.o SeqListNode.o SeqDatesUtil.o SeqUtil.o l2d2_commun.o SeqUtilServer.o QueryServer.o l2d2_socket.o runcontrollib.o ocmjinfo.o expcatchup.o maestro_main.c -L $(XML_LIB_DIR) -lxml2 $(LIB) -o maestro; \
	cp maestro $(BINDIR);

expcatchup:	expcatchup.o SeqUtil.o expcatchup_main.c XmlUtils.o l2d2_commun.o
	$(CC) -g expcatchup.o SeqUtil.o expcatchup_main.c XmlUtils.o l2d2_commun.o -L $(XML_LIB_DIR) -lxml2 $(LIB) -o expcatchup; \
	cp expcatchup $(BINDIR);

nodeinfo:	SeqUtil.o SeqNode.o XmlUtils.o SeqLoopsUtil.o l2d2_commun.o QueryServer.o l2d2_socket.o SeqListNode.o SeqDatesUtil.o SeqUtilServer.o tictac.o SeqNameValues.o nodeinfo.o nodeinfo_main.c
	$(CC) -g SeqUtil.o SeqNode.o XmlUtils.o SeqLoopsUtil.o l2d2_commun.o QueryServer.o l2d2_socket.o SeqListNode.o SeqDatesUtil.o SeqUtilServer.o tictac.o SeqNameValues.o nodeinfo.o nodeinfo_main.c -L $(XML_LIB_DIR) -lxml2 $(LIB) -o nodeinfo 
	cp nodeinfo $(BINDIR)

getdef:	getdef_main.o SeqUtil.o l2d2_commun.o
	$(CC) -g -c getdef_main.c 
	$(CC) -g getdef_main.o SeqUtil.o l2d2_commun.o $(LIB) -o getdef
	cp getdef $(BINDIR)

mserver: $(L2D2SOBJECTS)
	$(CC) $(L2D2SOBJECTS) $(LIB) $(LIBTH) -o mserver; \
	cp mserver $(BINDIR);

madmin: $(L2D2AOBJECTS)
	$(CC) $(L2D2AOBJECTS) $(LIB) $(LIBTH) -o madmin; \
	cp madmin $(BINDIR);

