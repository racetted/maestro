# ATTENTION!
# YOU NEED TO USE THE GMAKE IN /users/dor/afsi/sul/ovbin/gmake for this
# Makefile to work, other versions installed are currently too old!
#
# common section definition
include ../config/config.mk

ssm:	profile.sh  ../bin/$(BASE_ARCH)
	echo "Creating SSM package $(SSMPACKAGE)..."
	if [ ! -d $(SSMPACKAGE) ]; then \
	   mkdir $(SSMPACKAGE); \
	fi;
	cp -r $(SWDEST)/* $(SSMPACKAGE); 
	cp ../Makefile $(SSMPACKAGE); 
	cp ../LICENSE.txt $(SSMPACKAGE); 
	cp -r ../config $(SSMPACKAGE); 
	mkdir -p $(SSMPACKAGE)/.ssm.d;
	mkdir -p $(SSMPACKAGE)/ssm;
	cp Makefile post-install $(SSMPACKAGE)/ssm; 
	cp -r post-install profile.sh $(SSMPACKAGE)/.ssm.d;
	mkdir -p $(SSMPACKAGE)/src;
	cp ../src/Makefile ../src/*.c ../src/*.h $(SSMPACKAGE)/src;
	echo "Package: maestro" > $(SSMPACKAGE)/.ssm.d/control; \
	echo "Version: ${VERSION}" >> $(SSMPACKAGE)/.ssm.d/control; \
	echo "Platform: ${ORDENV_PLAT}" >> $(SSMPACKAGE)/.ssm.d/control; \
	echo "Maintainer: ${WHO_I_AM}" >> $(SSMPACKAGE)/.ssm.d/control; \
	echo "BuildInfo:" >> $(SSMPACKAGE)/.ssm.d/control; \
	echo "Description: Unified Task Sequencer" >> $(SSMPACKAGE)/.ssm.d/control; \
	tar cvf - $(SSMPACKAGE) | gzip -> $(SSMPACKAGE).ssm

profile.sh: Makefile ../config/config.mk
	touch profile ;\
	echo "export SEQ_MAESTRO_VERSION=${VERSION}" >>$@ ;\
	cat profile >>$@

../bin/$(BASE_ARCH):
	$(MAKE) -C ../src all

clean:
	rm -rf $(SSMPACKAGE).ssm $(SSMPACKAGE) profile.sh

install: ssm
	if [ ! $(domain) ] ; then \
		echo "Define installation path with 'domain='" ; \
		exit 1 ; \
	fi ; \
	if [ ! -d $(domain) ] ; then \
	  ssm created -d $(domain) ; \
	else \
		if [ ! -d $(domain)/etc/ssm.d ] ; then \
			echo "domain: $(domain) must be an ssm domain or a non-existing directory" ; \
			exit 1 ; \
		fi ;\
	fi ; \
	if [ -d $(domain)/$(SSMPACKAGE) ] ; then \
	  $(MAKE) uninstall ; \
	fi ; \
	ssm install -d $(domain) -f $(SSMPACKAGE).ssm ; \
	ssm publish -d $(domain) -p $(SSMPACKAGE)

uninstall:
	if [ ! $(domain) ] ; then \
	  echo "Define installation path with 'domain='" ; \
	  exit 1 ; \
	fi ; \
	ssm unpublish -d $(domain) -p $(SSMPACKAGE) ; \
	ssm uninstall -d $(domain) -p $(SSMPACKAGE) ; \
