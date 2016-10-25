#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2009 High Performance Computing Center Stuttgart,
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2006-2016 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2012-2013 Los Alamos National Security, Inc.  All rights reserved.
# Copyright (c) 2013-2016 Intel, Inc. All rights reserved
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# Note that the -I directory must *exactly* match what was specified
# via AC_CONFIG_MACRO_DIR in configure.ac.
ACLOCAL_AMFLAGS = -I ./config

SUBDIRS = \
    util/keyval \
    mca/base \
    $(MCA_scon_FRAMEWORKS_SUBDIRS) \
    $(MCA_scon_FRAMEWORK_COMPONENT_STATIC_SUBDIRS) \
    . \
    $(MCA_scon_FRAMEWORK_COMPONENT_DSO_SUBDIRS)

DIST_SUBDIRS = \
    util/keyval \
    mca/base \
    $(MCA_scon_FRAMEWORKS_SUBDIRS) \
    $(MCA_scon_FRAMEWORK_COMPONENT_ALL_SUBDIRS)

headers =
sources =
nodist_headers =
EXTRA_DIST =
dist_scondata_DATA =

libscon_la_LIBADD = \
	mca/base/libscon_mca_base.la \
	$(MCA_scon_FRAMEWORK_LIBS)
libscon_la_DEPENDENCIES = $(libscon_la_LIBADD)

#if WANT_DSTORE
#include sm/Makefile.include
#include dstore/Makefile.include
#endif

if SCON_EMBEDDED_MODE
noinst_LTLIBRARIES = libscon.la
libscon_la_SOURCES = $(headers) $(sources)
libscon_la_LDFLAGS =

else

lib_LTLIBRARIES = libscon.la
libscon_la_SOURCES = $(headers) $(sources)
libscon_la_LDFLAGS = -version-info $(libscon_so_version)

endif !SCON_EMBEDDED_MODE

include class/Makefile.include
include buffer_ops/Makefile.am
include include/Makefile.include
include mca/Makefile.include
include util/Makefile.include
include runtime/Makefile.include

MAINTAINERCLEANFILES = Makefile.in config.h config.h.in
DISTCLEANFILES = Makefile
CLEANFILES = core.* *~
AM_CFLAGS = -Wall
