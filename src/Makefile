# common section definition
include ../config/config.mk

CFLAGS = -optc=-g -defines=-DM$(MACH)=1

all: prep runcontrol mserver madmin


L2D2SERVER = mserver
L2D2ADMIN  = madmin

LIB=-lc -lm -lcrypto 
LIBTH=-lpthread
CFLAGS1 = -g
CFLAGS2 = -lefence -g -I../inc -DUNITTEST -DREENTRANT -DLINUX -Wall -Wextra -Wno-unused -D__DEBUG -DIGNORE_EMPTY_TEXT_NODES
ROXML_OBJECTS = l2d2_roxml.o l2d2_roxml-internal.o l2d2_roxml-parse-engine.o
L2D2SOBJECTS  = l2d2_server.o l2d2_socket.o l2d2_Util.o l2d2_commun.o $(ROXML_OBJECTS) 
L2D2AOBJECTS  = l2d2_admin.o l2d2_socket.o l2d2_Util.o l2d2_commun.o $(ROXML_OBJECTS) 
 
runcontrol: prep $(OBJECTS) mserver
	rm -f $(LIBDIR)/libruncontrol.a
	ar rcv $(LIBDIR)/libruncontrol.a $(OBJECTS)
	cp *.h $(INCDIR)
	ARCH_EXTRA=""
	for component in $(COMPONENTS); do \
	   if test $(MACHINE) = "Linux"; then \
	      ARCH_EXTRA="-bidon c -main main_$${component}"; \
	   fi; \
	   s.compile $(CFLAGS) -includes $(INCDIR) $(XML_INCLUDE_DIR) -libpath $(XML_LIB_DIR) $(LIBDIR) $(XTERN_LIB) -libappl $(LIBNAME) -src $${component}_main.c -o $${component} $${ARCH_EXTRA}; \
	   cp $${component} $(BINDIR) ; \
	done;
	mkdir -p $(BINDIR)/wrappers;
	cd wrappers; for file in *; do \
	    cp $${file} $(BINDIR)/wrappers/maestro.$${file}; \
	done; \
	cd ..

runcontrollib.o:	runcontrollib.c runcontrollib.h nodelogger.h nodeinfo.h
	s.compile $(CFLAGS) -includes $(INCDIR) -libpath $(XTERN_LIB) -src runcontrollib.c

SeqNode.o:	SeqNode.c SeqNode.h
	$(CC) -g -c SeqNode.c;

utils:	SeqListNode.o SeqNameValues.o SeqUtil.o

SeqListNode.o:	SeqListNode.c
	$(CC) -g -c SeqListNode.c

SeqNameValues.o:	SeqNameValues.c
	$(CC) -g -c SeqNameValues.c

SeqLoopsUtil.o:	SeqLoopsUtil.c
	$(CC) -g -c SeqLoopsUtil.c

SeqDatesUtil.o:	SeqDatesUtil.c
	s.compile $(CFLAGS) -src SeqDatesUtil.c

XmlUtils.o: XmlUtils.c
	s.compile $(CFLAGS) -includes $(INCDIR) $(XML_INCLUDE_DIR) -libpath $(LIBDIR) $(XML_LIB_DIR) -src XmlUtils.c

SeqUtil.o:	SeqUtil.c SeqUtil.h
	$(CC) -g -c SeqUtil.c


SeqUtilServer.o: SeqUtilServer.c QueryServer.h
	$(CC) -g -c SeqUtilServer.c
 
nodelogger.o:	nodelogger.c nodelogger.h SeqUtil.c l2d2_commun.c l2d2_Util.c l2d2_socket.c
	s.compile $(CFLAGS) -src nodelogger.c

tictac.o:	tictac.c tictac.h
	s.compile $(CFLAGS) -src tictac.c

QueryServer.o: QueryServer.c QueryServer.h
	s.compile  $(CFLAGS) -src QueryServer.c
  
l2d2_socket.o: l2d2_socket.h l2d2_socket.c 
	s.compile  $(CFLAGS) -src l2d2_socket.c

expcatchup.o:	expcatchup.c expcatchup.h
	s.compile $(CFLAGS) -includes $(INCDIR) $(XML_INCLUDE_DIR) -libpath $(LIBDIR) $(XML_LIB_DIR) -src expcatchup.c

nodeinfo.o:	nodeinfo.c nodeinfo.h runcontrollib.h
	s.compile $(CFLAGS) -includes $(INCDIR) $(XML_INCLUDE_DIR) -libpath $(LIBDIR) $(XML_LIB_DIR)  -src nodeinfo.c

maestro.o:	maestro.c QueryServer.h maestro.h nodeinfo.h runcontrollib.h nodelogger.h tictac.h SeqUtil.h
	echo CFLAGS=$(CFLAGS) ; \
	s.compile $(CFLAGS) -libappl maestro -src maestro.c

prep:
	if [ ! -d $(SWDEST) ] ; then \
	   echo "Creating $(SWDEST)" ; \
	   mkdir -p $(SWDEST); mkdir $(SWDEST)/lib;mkdir $(SWDEST)/include; mkdir $(SWDEST)/bin; \
	fi ; \
	eval $(COMPILER_SSM_CMD)


ocmjinfo.o:	ocmjinfo.c ocmjinfo.h 
	s.compile $(CFLAGS) -src ocmjinfo.c

l2d2_Util.o: l2d2_Util.c l2d2_Util.h
	$(CC) $(CFLAGS1) -c l2d2_Util.c
 
l2d2_commun.o: l2d2_commun.c l2d2_commun.h
	$(CC) $(CFLAGS1) -c l2d2_commun.c
 
l2d2_admin.o: l2d2_admin.c
	$(CC) $(CFLAGS1) -c l2d2_admin.c
 
l2d2_server.o: l2d2_server.c l2d2_server.h
	$(CC) $(CFLAGS1) -c l2d2_server.c
 
l2d2_roxml.o: l2d2_roxml.c
	$(CC)  $(CFLAGS2) -c l2d2_roxml.c
 
l2d2_roxml-internal.o: l2d2_roxml-internal.c
	$(CC)  $(CFLAGS2) -c l2d2_roxml-internal.c
 
l2d2_roxml-parse-engine.o: l2d2_roxml-parse-engine.c
	$(CC)  $(CFLAGS2) -c l2d2_roxml-parse-engine.c
 
mserver: l2d2_server.o l2d2_Util.o l2d2_socket.o l2d2_commun.o l2d2_roxml.o l2d2_roxml-internal.o l2d2_roxml-parse-engine.o
	$(CC) $(L2D2SOBJECTS) $(LIB) $(LIBTH) -o $(L2D2SERVER); \
	cp $(L2D2SERVER) $(BINDIR);
 
madmin: l2d2_admin.o l2d2_socket.o l2d2_Util.o l2d2_commun.o l2d2_roxml.o l2d2_roxml-internal.o l2d2_roxml-parse-engine.o
	$(CC) $(L2D2AOBJECTS) $(LIB) $(LIBTH) -o $(L2D2ADMIN)
	cp $(L2D2ADMIN) $(BINDIR);

clean:
	rm -f *.o; rm -f $(COMPONENTS) nodelogger_svr $(L2D2ADMIN) $(L2D2SERVER); rm -rf $(SWDEST)

