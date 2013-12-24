all:
	$(MAKE) -C src all
	$(MAKE) -C ssm ssm

clean: 
	$(MAKE) -C src $@
	$(MAKE) -C ssm $@

distclean: clean
	rm -fr bin/$(BASE_ARCH) ; \
	find . -name "*~" -exec rm -f {} \;

install: all
	cd ssm ; $(MAKE) $@

uninstall:
	$(MAKE) -C ssm $@