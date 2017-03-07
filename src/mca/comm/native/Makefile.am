#
# Copyright (c) 2016      Intel, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

headers = \
         comm_native.h

sources = \
          comm_native_component.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_scon_comm_native_DSO
lib =
lib_sources =
component = mca_comm_native.la
component_sources = $(headers) $(sources)
else
lib = libmca_comm_native.la
lib_sources = $(headers) $(sources)
component =
component_sources =
endif

mcacomponentdir = $(sconlibdir)
mcacomponent_LTLIBRARIES = $(component)
mca_comm_native_la_SOURCES = $(component_sources)
mca_comm_native_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(lib)
libmca_comm_native_la_SOURCES = $(lib_sources)
libmca_comm_native_la_LDFLAGS = -module -avoid-version

