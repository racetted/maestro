# common section definition
include ../config/config.mk

CFLAGS = -optc=-g -includes $(INCDIR) $(XML_DIR)/include/libxml2 \
	-librmn -defines=-DM$(MACH)=1

TEST_CFLAGS = -I$(XML_DIR)/include/libxml2 -L$(XML_DIR)/lib -lxml2

all: log_server runcontrol
runcontrol: prep $(OBJECTS) log_server
	rm -f $(LIBDIR)/libruncontrol.a
	ar rcv $(LIBDIR)/libruncontrol.a $(OBJECTS)
	cp *.h $(INCDIR)
	ARCH_EXTRA=""
	for component in $(COMPONENTS); do \
	   if test $(MACHINE) = "Linux"; then \
	      ARCH_EXTRA="-bidon c -main main_$${component}"; \
	   fi; \
	   r.compile $(CFLAGS) -libpath $(LIBDIR) $(XML_DIR)/lib -libappl $(LIBNAME) -src $${component}_main.c -o $${component} $${ARCH_EXTRA}; \
	   cp $${component} $(BINDIR) ; \
	done;

runcontrollib.o:	runcontrollib.c runcontrollib.h nodelogger.h nodeinfo.h
	r.compile $(CFLAGS) -src runcontrollib.c

SeqNode.o:	SeqNode.c SeqNode.h
	$(CC) -g -c SeqNode.c;

utils:	SeqListNode.o SeqNameValues.o SeqUtil.o

SeqListNode.o:	SeqListNode.c
	$(CC) -g -c SeqListNode.c

SeqNameValues.o:	SeqNameValues.c
	$(CC) -g -c SeqNameValues.c

SeqLoopsUtil.o:	SeqLoopsUtil.c
	$(CC) -g -c SeqLoopsUtil.c

SeqUtil.o:	SeqUtil.c
	$(CC) -g -c SeqUtil.c

nodelogger.o:	nodelogger.c nodelogger.h
	r.compile $(CFLAGS) -src nodelogger.c

tictac.o:	tictac.c tictac.h
	r.compile $(CFLAGS) -src tictac.c

nodeinfo.o:	nodeinfo.c nodeinfo.h runcontrollib.h
	r.compile $(CFLAGS) -src nodeinfo.c

maestro.o:	maestro.c maestro.h nodeinfo.h runcontrollib.h nodelogger.h tictac.h
	echo CFLAGS=$(CFLAGS) ; \
	r.compile $(CFLAGS) -libappl maestro -src maestro.c
prep:
	if [ ! -d $(SWDEST) ] ; then \
	   echo "Creating $(SWDEST)" ; \
	   mkdir -p $(SWDEST); mkdir $(SWDEST)/lib;mkdir $(SWDEST)/include; mkdir $(SWDEST)/bin; \
	fi

log_server: nodelogger_svr.c initvar.c LogServerUtil.c SeqUtil.c SeqListNode.c
	cc nodelogger_svr.c initvar.c LogServerUtil.c SeqUtil.c SeqListNode.c -o nodelogger_svr; \
	cp nodelogger_svr $(BINDIR) 

clean:
	rm -f *.o; rm -f $(COMPONENTS)
