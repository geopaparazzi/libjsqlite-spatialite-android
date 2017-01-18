ACLOCAL_AMFLAGS = -I m4

SUBDIRS = headers src test tools

EXTRA_DIST = mainpage.doxy Android_R4.3.0.mk rasterlite2-4.3.0.mk \
	Makefile-static-MinGW

AUTOMAKE_OPTIONS = dist-zip

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rasterlite2.pc

coverage-init:
	lcov --directory src --capture --initial --output-file rasterlite2_cov.info

coverage::
	lcov --rc lcov_branch_coverage=1 --directory src --output-file rasterlite2_cov.info --capture
	genhtml --rc lcov_branch_coverage=1 -o covresults rasterlite2_cov.info

MOSTLYCLEANFILES = rasterlite2_cov.info 
