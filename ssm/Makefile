# ATTENTION!
# YOU NEED TO USE THE GMAKE IN /users/dor/afsi/sul/ovbin/gmake for this
# Makefile to work, other versions installed are currently too old!
#
# common section definition
include ../config/config.mk

ssm:	profile.sh 
	echo "Creating SSM package $(SSMPACKAGE)..."
	if [ ! -d $(SSMPACKAGE) ]; then \
	   mkdir $(SSMPACKAGE); \
	fi;
	cp -r $(SWDEST)/* $(SSMPACKAGE); 
	mkdir -p $(SSMPACKAGE)/.ssm.d;
	cp -r post-install profile.sh $(SSMPACKAGE)/.ssm.d;
	mkdir -p $(SSMPACKAGE)/src;
	cp ../src/*.c ../src/*.h $(SSMPACKAGE)/src;
	ssmprep --basePath $(shell pwd)/$(SSMPACKAGE) -p $(SSMPACKAGE) --editorPath /bin/echo;
	echo "Package: maestro" > $(SSMPACKAGE)/.ssm.d/control; \
	echo "Version: ${VERSION}" >> $(SSMPACKAGE)/.ssm.d/control; \
	echo "Platform: ${SSM_MACH_ID}" >> $(SSMPACKAGE)/.ssm.d/control; \
	echo "Maintainer: ${WHO_I_AM}" >> $(SSMPACKAGE)/.ssm.d/control; \
	echo "BuildInfo:" >> $(SSMPACKAGE)/.ssm.d/control; \
	echo "Description: Unified Task Sequencer" >> $(SSMPACKAGE)/.ssm.d/control; \
	tar cvf - $(SSMPACKAGE) | gzip -> $(SSMPACKAGE).ssm

profile.sh: Makefile ../config/config.mk
	touch profile ;\
	echo "export SEQ_WRAPPER=${SEQ_WRAPPER}" >$@ ;\
	echo "export SEQ_MAESTRO_VERSION=${VERSION}" >>$@ ;\
	cat profile >>$@

clean:
	rm -rf $(SSMPACKAGE).ssm $(SSMPACKAGE) profile.sh

