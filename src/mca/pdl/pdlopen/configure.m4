# -*- shell-script -*-
#
# Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
#
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

AC_DEFUN([MCA_scon_pdl_pdlopen_PRIORITY], [80])

#
# Force this component to compile in static-only mode
#
AC_DEFUN([MCA_scon_pdl_pdlopen_COMPILE_MODE], [
    AC_MSG_CHECKING([for MCA component $1:$2 compile mode])
    $3="static"
    AC_MSG_RESULT([$$3])
])

# MCA_pdl_pdlopen_CONFIG([action-if-can-compile],
#                      [action-if-cant-compile])
# ------------------------------------------------
AC_DEFUN([MCA_scon_pdl_pdlopen_CONFIG],[
    AC_CONFIG_FILES([src/mca/pdl/pdlopen/Makefile])

    dnl This is effectively a back-door for SCON developers to
    dnl force the use of the libltdl pdl component.
    AC_ARG_ENABLE([pdl-dlopen],
        [AS_HELP_STRING([--disable-pdl-dlopen],
                        [Disable the "dlopen" PDL component (and probably force the use of the "libltdl" PDL component).  This option should really only be used by SCON developers.  You are probably actually looking for the "--disable-dlopen" option, which disables all dlopen-like functionality from SCON.])
        ])

    scon_pdl_pdlopen_happy=no
    AS_IF([test "$enable_pdl_dlopen" != "no"],
          [SCON_CHECK_PACKAGE([scon_pdl_pdlopen],
              [dlfcn.h],
              [dl],
              [dlopen],
              [],
              [],
              [],
              [scon_pdl_pdlopen_happy=yes],
              [scon_pdl_pdlopen_happy=no])
          ])

    AS_IF([test "$scon_pdl_pdlopen_happy" = "yes"],
          [scon_pdl_pdlopen_ADD_LIBS=$scon_pdl_pdlopen_LIBS
           $1],
          [$2])

    AC_SUBST(scon_pdl_pdlopen_LIBS)
])
