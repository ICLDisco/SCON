#
# Copyright (c) 2016      Intel, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

headers = \
         topology_binomial.h

sources = \
          topology_binomtree_component.c \
          topology_binomial.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_scon_topology_binomtree_DSO
lib =
lib_sources =
component = mca_topology_binomtree.la
component_sources = $(headers) $(sources)
else
lib = libmca_topology_binomtree.la
lib_sources = $(headers) $(sources)
component =
component_sources =
endif

mcacomponentdir = $(sconlibdir)
mcacomponent_LTLIBRARIES = $(component)
mca_topology_binomtree_la_SOURCES = $(component_sources)
mca_topology_binomtree_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(lib)
libmca_topology_binomtree_la_SOURCES = $(lib_sources)
libmca_topology_binomtree_la_LDFLAGS = -module -avoid-version

