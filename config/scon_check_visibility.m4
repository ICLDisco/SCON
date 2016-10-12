# -*- shell-script -*-
#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2007 High Performance Computing Center Stuttgart,
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2006-2015 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2009-2011 Oracle and/or its affiliates.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# SCON_CHECK_VISIBILITY
# --------------------------------------------------------
AC_DEFUN([SCON_CHECK_VISIBILITY],[
    AC_REQUIRE([AC_PROG_GREP])

    # Check if the compiler has support for visibility, like some
    # versions of gcc, icc Sun Studio cc.
    AC_ARG_ENABLE(visibility,
        AC_HELP_STRING([--enable-visibility],
            [enable visibility feature of certain compilers/linkers (default: enabled)]))

    WANT_VISIBILITY=0
    scon_msg="whether to enable symbol visibility"

    if test "$enable_visibility" = "no"; then
        AC_MSG_CHECKING([$scon_msg])
        AC_MSG_RESULT([no (disabled)])
    else
        CFLAGS_orig=$CFLAGS

        scon_add=
        case "$scon_c_vendor" in
        sun)
            # Check using Sun Studio -xldscope=hidden flag
            scon_add=-xldscope=hidden
            CFLAGS="$SCON_CFLAGS_BEFORE_PICKY $scon_add -errwarn=%all"
            ;;

        *)
            # Check using -fvisibility=hidden
            scon_add=-fvisibility=hidden
            CFLAGS="$SCON_CFLAGS_BEFORE_PICKY $scon_add -Werror"
            ;;
        esac

        AC_MSG_CHECKING([if $CC supports $scon_add])
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[
            #include <stdio.h>
            __attribute__((visibility("default"))) int foo;
            ]],[[fprintf(stderr, "Hello, world\n");]])],
            [AS_IF([test -s conftest.err],
                   [$GREP -iq visibility conftest.err
                    # If we find "visibility" in the stderr, then
                    # assume it doesn't work
                    AS_IF([test "$?" = "0"], [scon_add=])])
            ], [scon_add=])
        AS_IF([test "$scon_add" = ""],
              [AC_MSG_RESULT([no])],
              [AC_MSG_RESULT([yes])])

        CFLAGS=$CFLAGS_orig
        SCON_VISIBILITY_CFLAGS=$scon_add

        if test "$scon_add" != "" ; then
            WANT_VISIBILITY=1
            CFLAGS="$CFLAGS $SCON_VISIBILITY_CFLAGS"
            AC_MSG_CHECKING([$scon_msg])
            AC_MSG_RESULT([yes (via $scon_add)])
        elif test "$enable_visibility" = "yes"; then
            AC_MSG_ERROR([Symbol visibility support requested but compiler does not seem to support it.  Aborting])
        else
            AC_MSG_CHECKING([$scon_msg])
            AC_MSG_RESULT([no (unsupported)])
        fi
        unset scon_add
    fi

    AC_DEFINE_UNQUOTED([SCON_C_HAVE_VISIBILITY], [$WANT_VISIBILITY],
            [Whether C compiler supports symbol visibility or not])
    AM_CONDITIONAL([WANT_HIDDEN],[test "$WANT_VISIBILITY" = "1"])
])
