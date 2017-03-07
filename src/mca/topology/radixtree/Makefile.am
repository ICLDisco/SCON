#
# Copyright (c) 2016      Intel, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

headers = \
         topology_radix.h

sources = \
          topology_radix_component.c \
          topology_radix.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_scon_topology_radixtree_DSO
lib =
lib_sources =
component = mca_topology_radixtree.la
component_sources = $(headers) $(sources)
else
lib = libmca_topology_radixtree.la
lib_sources = $(headers) $(sources)
component =
component_sources =
endif

mcacomponentdir = $(sconlibdir)
mcacomponent_LTLIBRARIES = $(component)
mca_topology_radixtree_la_SOURCES = $(component_sources)
mca_topology_radixtree_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(lib)
libmca_topology_radixtree_la_SOURCES = $(lib_sources)
libmca_topology_radixtree_la_LDFLAGS = -module -avoid-version

