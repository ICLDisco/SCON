#
# Copyright (c) 2017 Intel, Inc. All rights reserved
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

AM_CPPFLAGS = -I$(SCON_top_srcdir) -I$(SCON_top_srcdir)/src -I$(SCON_top_srcdir)/include

headers+ = test_common.h

noinst_PROGRAMS = test_init

test_init_SOURCES = $(headers) test_init.c
test_init_LDFLAGS = $(SCON_PKG_CONFIG_LDFLAGS)
test_init_LDADD = \
    $(SCON_top_builddir)/src/libscon.la
