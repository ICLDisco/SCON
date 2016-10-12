#
# Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2010-2016 Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# Need this so that the base knows where to load dynamic components from
# (by default)

AM_CPPFLAGS = \
    $(LTDLINCL)

noinst_LTLIBRARIES = libscon_mca_base.la

dist_scondata_DATA = help-mca-base.txt help-mca-var.txt

# Source code files

headers = \
        base.h \
        scon_mca_base_component_repository.h \
        scon_mca_base_var.h \
        scon_mca_base_var_enum.h \
        scon_mca_base_var_group.h \
        scon_mca_base_vari.h \
        scon_mca_base_framework.h

# Library

libscon_mca_base_la_SOURCES = \
        $(headers) \
        scon_mca_base_close.c \
        scon_mca_base_component_compare.c \
        scon_mca_base_component_find.c \
        scon_mca_base_component_repository.c \
        scon_mca_base_components_open.c \
        scon_mca_base_components_close.c \
        scon_mca_base_components_select.c \
        scon_mca_base_list.c \
        scon_mca_base_open.c \
        scon_mca_base_var.c \
        scon_mca_base_var_enum.c \
        scon_mca_base_var_group.c \
        scon_mca_base_parse_paramfile.c \
        scon_mca_base_components_register.c \
        scon_mca_base_framework.c

# Conditionally install the header files

if WANT_INSTALL_HEADERS
scondir = $(sconincludedir)/$(subdir)
scon_HEADERS = $(headers)
endif
