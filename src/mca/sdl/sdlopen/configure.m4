# -*- shell-script -*-
#
# Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2016      Research Organization for Information Science
#                         and Technology (RIST). All rights reserved.
#
# Copyright (c) 2017      Intel, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

AC_DEFUN([MCA_scon_sdl_sdlopen_PRIORITY], [80])

#
# Force this component to compile in static-only mode
#
AC_DEFUN([MCA_scon_sdl_sdlopen_COMPILE_MODE], [
    AC_MSG_CHECKING([for MCA component $1:$2 compile mode])
    $3="static"
    AC_MSG_RESULT([$$3])
])

# MCA_scon_sdl_sdlopen_POST_CONFIG()
# ---------------------------------
AC_DEFUN([MCA_scon_sdl_sdlopen_POST_CONFIG],[
    # If we won, then do all the rest of the setup
    AS_IF([test "$1" = "1"],
          [
           # Add some stuff to CPPFLAGS so that the rest of the source
           # tree can be built
           LDFLAGS="$LDFLAGS $scon_sdl_sdlopen_ADD_LDFLAGS"
           LIBS="$LIBS $scon_sdl_sdlopen_ADD_LIBS"
          ])
])dnl

# MCA_sdl_sdlopen_CONFIG([action-if-can-compile],
#                      [action-if-cant-compile])
# ------------------------------------------------
AC_DEFUN([MCA_scon_sdl_sdlopen_CONFIG],[
    AC_CONFIG_FILES([src/mca/sdl/sdlopen/Makefile])

    dnl This is effectively a back-door for SCON developers to
    dnl force the use of the libltdl sdl component.
    AC_ARG_ENABLE([dlopen],
        [AS_HELP_STRING([--disable-dlopen],
                        [Disable the "dlopen" PDL component (and probably force the use of the "libltdl" PDL component).])
        ])

    scon_sdl_sdlopen_happy=no
    AS_IF([test "$enable_dlopen" != "no"],
          [SCON_CHECK_PACKAGE([scon_sdl_sdlopen],
              [dlfcn.h],
              [dl],
              [dlopen],
              [],
              [],
              [],
              [scon_sdl_sdlopen_happy=yes],
              [scon_sdl_sdlopen_happy=no])
          ])

    AS_IF([test "$scon_sdl_sdlopen_happy" = "yes"],
          [scon_sdl_sdlopen_ADD_LIBS=$scon_sdl_sdlopen_LIBS
           $1],
          [$2])

    AC_SUBST(scon_sdl_sdlopen_LIBS)
])
