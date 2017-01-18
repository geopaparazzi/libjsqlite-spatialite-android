all install uninstall clean: config.status
	$(MAKE) -C src $@

maintainer-clean-local:
	rm -f configure
	rm -f config.guess config.rpath config.sub
	rm -f libtool install-sh ltmain.sh

maintainer-clean: distclean
	$(MAKE) maintainer-clean-local

distclean-local:
	rm -Rf autom4te.cache
	rm -f config.log config.cache config.status

distclean: clean
	$(MAKE) distclean-local

configure: configure.ac
	./autogen.sh

config.status: configure
	./configure
