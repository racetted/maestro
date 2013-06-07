# common section definition
include ../config/config.mk

#CFLAGS = -optc=-g -librmn -defines=-DM$(MACH)=1
CFLAGS = -optc=-g -defines=-DM$(MACH)=1

all: prep log_server runcontrol

runcontrol: prep $(OBJECTS) log_server
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
	    cp $${file} $(BINDIR)/wrappers/maestro_$(VERSION).$${file}; \
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

SeqUtil.o:	SeqUtil.c
	$(CC) -g -c SeqUtil.c

nodelogger.o:	nodelogger.c nodelogger.h
	s.compile $(CFLAGS) -src nodelogger.c

tictac.o:	tictac.c tictac.h
	s.compile $(CFLAGS) -src tictac.c

expcatchup.o:	expcatchup.c expcatchup.h
	s.compile $(CFLAGS) -includes $(INCDIR) $(XML_INCLUDE_DIR) -libpath $(LIBDIR) $(XML_LIB_DIR) -src expcatchup.c

nodeinfo.o:	nodeinfo.c nodeinfo.h runcontrollib.h
	s.compile $(CFLAGS) -includes $(INCDIR) $(XML_INCLUDE_DIR) -libpath $(LIBDIR) $(XML_LIB_DIR)  -src nodeinfo.c

maestro.o:	maestro.c maestro.h nodeinfo.h runcontrollib.h nodelogger.h tictac.h
	echo CFLAGS=$(CFLAGS) ; \
	s.compile $(CFLAGS) -libappl maestro -src maestro.c

prep:
	if [ ! -d $(SWDEST) ] ; then \
	   echo "Creating $(SWDEST)" ; \
	   mkdir -p $(SWDEST); mkdir $(SWDEST)/lib;mkdir $(SWDEST)/include; mkdir $(SWDEST)/bin; \
	fi ; \
	eval $(COMPILER_SSM_CMD)

log_server: nodelogger_svr.c initvar.c LogServerUtil.c SeqUtil.c SeqListNode.c
	cc nodelogger_svr.c initvar.c LogServerUtil.c SeqUtil.c SeqListNode.c -o nodelogger_svr; \
	cp nodelogger_svr $(BINDIR) 

clean:
	rm -f *.o; rm -f $(COMPONENTS) nodelogger_svr
