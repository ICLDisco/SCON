#
# Copyright (c) 2004-2010 The Trustees of Indiana University.
#                         All rights reserved.
# Copyright (c) 2014-2015 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2017 Intel, Inc. All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

sources = \
        sdl_sdlopen.h \
        sdl_sdlopen_component.c \
        sdl_sdlopen_module.c

# This component will only ever be built statically -- never as a DSO.

noinst_LTLIBRARIES = libmca_sdl_sdlopen.la

libmca_sdl_sdlopen_la_SOURCES = $(sources)
libmca_sdl_sdlopen_la_LDFLAGS = -module -avoid-version
libmca_sdl_sdlopen_la_LIBADD = $(scon_sdl_sdlopen_LIBS)
