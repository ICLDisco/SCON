#
# Copyright (c) 2017 Intel, Inc. All rights reserved
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

AM_CPPFLAGS = -I$(SCON_top_srcdir) -I$(SCON_top_srcdir)/src -I$(SCON_top_srcdir)/include

headers = test_common.h

noinst_PROGRAMS = test_init test_xcast test_send_recv

test_xcast_SOURCES = $(headers) test_xcast.c
test_xcast_LDFLAGS = $(SCON_PKG_CONFIG_LDFLAGS)
test_xcast_LDADD = \
    $(SCON_top_builddir)/src/libscon.la

test_init_SOURCES = $(headers) test_init.c
test_init_LDFLAGS = $(SCON_PKG_CONFIG_LDFLAGS)
test_init_LDADD = \
    $(SCON_top_builddir)/src/libscon.la

test_send_recv_SOURCES = $(headers) test_send_recv.c
test_send_recv_LDFLAGS = $(SCON_PKG_CONFIG_LDFLAGS)
test_send_recv_LDADD = \
    $(SCON_top_builddir)/src/libscon.la
