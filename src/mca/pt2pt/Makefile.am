#
# Copyright (c) 2015-2017     Intel, Inc. All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#
# main library setup
noinst_LTLIBRARIES = libmca_pt2pt.la
libmca_pt2pt_la_SOURCES =

# pkgdata setup
dist_scondata_DATA =

# local files
headers = pt2pt.h
libmca_pt2pt_la_SOURCES += $(headers)

# Conditionally install the header files
if WANT_INSTALL_HEADERS
scondir = $(sconincludedir)/$(subdir)
nobase_scon_HEADERS = $(headers)
endif

include base/Makefile.am

distclean-local:
	rm -f base/static-components.h
