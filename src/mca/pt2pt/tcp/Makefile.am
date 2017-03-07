#
# Copyright (c) 2016      Intel, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

headers = \
          pt2pt_tcp_component.h \
          pt2pt_tcp.h \
          pt2pt_tcp_listener.h \
          pt2pt_tcp_common.h \
		  pt2pt_tcp_connection.h \
          pt2pt_tcp_sendrecv.h \
          pt2pt_tcp_hdr.h \
		  pt2pt_tcp_peer.h

sources = \
          pt2pt_tcp_component.c \
          pt2pt_tcp.c \
          pt2pt_tcp_listener.c \
          pt2pt_tcp_common.c \
          pt2pt_tcp_connection.c \
          pt2pt_tcp_sendrecv.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_scon_pt2pt_tcp_DSO
lib =
lib_sources =
component = mca_pt2pt_tcp.la
component_sources = $(headers) $(sources)
else
lib = libmca_pt2pt_tcp.la
lib_sources = $(headers) $(sources)
component =
component_sources =
endif

mcacomponentdir = $(sconlibdir)
mcacomponent_LTLIBRARIES = $(component)
mca_pt2pt_tcp_la_SOURCES = $(component_sources)
mca_pt2pt_tcp_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(lib)
libmca_pt2pt_tcp_la_SOURCES = $(lib_sources)
libmca_pt2pt_tcp_la_LDFLAGS = -module -avoid-version

