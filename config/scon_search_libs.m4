dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2013-2014 Cisco Systems, Inc.  All rights reserved.
dnl Copyright (c) 2014-2016      Intel, Inc. All rights reserved.
dnl $COPYRIGHT$
dnl
dnl Additional copyrights may follow
dnl
dnl $HEADER$
dnl

# SCON SEARCH_LIBS_CORE(func, list-of-libraries,
#                       action-if-found, action-if-not-found,
#                       other-libraries)
#
# Wrapper around AC SEARCH_LIBS.  If a library ends up being added to
# $LIBS, then also add it to the wrapper LIBS list (so that it is
# added to the link command line for the static link case).
#
# NOTE: COMPONENTS SHOULD NOT USE THIS MACRO!  Components should use
# SCON_SEARCH_LIBS_COMPONENT.  The reason why is because this macro
# calls SCON_WRAPPER_FLAGS_ADD -- see big comment in
# scon_setup_wrappers.m4 for an explanation of why this is bad).
AC_DEFUN([SCON_SEARCH_LIBS_CORE],[
    AC_SEARCH_LIBS([$1], [$2],
        [scon_have_$1=1
         $3],
        [scon_have_$1=0
         $4], [$5])

    AC_DEFINE_UNQUOTED([SCON_HAVE_]m4_toupper($1), [$scon_have_$1],
         [whether $1 is found and available])

])dnl
