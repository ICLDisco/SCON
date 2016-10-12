#
# Copyright (c) 2015      Intel, Inc. All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

sources = \
	common_orte.h \
	common_orte_component.c

#	scon_common_orte_ops.c

# Make the output library in this directory, and name it either
# mca_scon_<type>_<name>.la (for DSO builds) or lib_mca_scon_<type>_<name>.la
# (for static builds).

if MCA_BUILD_scon_common_orte_DSO
lib =
lib_sources =
component = mca_scon_common_orte.la
component_sources = $(headers) $(sources)
else
lib = libmca_scon_common_orte.la
lib_sources = $(headers) $(sources)
component =
component_sources =
endif

mcacomponentdir = $(sconlibdir)
mcacomponent_LTLIBRARIES = $(component)
mca_scon_common_orte_la_SOURCES = $(component_sources)
mca_scon_common_orte_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(lib)
libmca_scon_common_orte_la_SOURCES = $(lib_sources)
libmca_scon_common_orte_la_LDFLAGS = -module -avoid-version
