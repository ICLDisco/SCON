# -*- shell-script -*-
#
# Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
#
# Copyright (c) 2017      Intel, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

AC_DEFUN([MCA_scon_sdl_slibltdl_PRIORITY], [50])

#
# Force this component to compile in static-only mode
#
AC_DEFUN([MCA_scon_sdl_slibltdl_COMPILE_MODE], [
    AC_MSG_CHECKING([for MCA component $1:$2 compile mode])
    $3="static"
    AC_MSG_RESULT([$$3])
])

# MCA_scon_sdl_slibltdl_POST_CONFIG()
# ---------------------------------
AC_DEFUN([MCA_scon_sdl_slibltdl_POST_CONFIG],[
    # If we won, then do all the rest of the setup
    AS_IF([test "$1" = "1"],
          [
           # Add some stuff to CPPFLAGS so that the rest of the source
           # tree can be built
           LDFLAGS="$LDFLAGS $scon_sdl_slibltdl_ADD_LDFLAGS"
           LIBS="$LIBS $scon_sdl_slibltdl_ADD_LIBS"
          ])
])dnl

# MCA_dl_slibltdl_CONFIG([action-if-can-compile],
#                       [action-if-cant-compile])
# ------------------------------------------------
AC_DEFUN([MCA_scon_sdl_slibltdl_CONFIG],[
    SCON_VAR_SCOPE_PUSH([CPPFLAGS_save LDFLAGS_save LIBS_save])
    AC_CONFIG_FILES([src/mca/sdl/slibltdl/Makefile])

    # Add --with options
    AC_ARG_WITH([slibltdl],
        [AC_HELP_STRING([--with-libltdl(=DIR)],
             [Build libltdl support, optionally adding DIR/include, DIR/lib, and DIR/lib64 to the search path for headers and libraries])])
    AC_ARG_WITH([libltdl-libdir],
       [AC_HELP_STRING([--with-libltdl-libdir=DIR],
             [Search for libltdl libraries in DIR])])

    # Sanity check the --with values
    SCON_CHECK_WITHDIR([slibltdl], [$with_libltdl],
                       [include/ltdl.h])
    SCON_CHECK_WITHDIR([slibltdl-libdir], [$with_libltdl_libdir],
                       [libltdl.*])

    # Defaults
    scon_check_slibltdl_dir_msg="compiler default"
    scon_check_slibltdl_libdir_msg="linker default"

    # Save directory names if supplied
    AS_IF([test ! -z "$with_libltdl" && test "$with_libltdl" != "yes"],
          [scon_check_slibltdl_dir=$with_libltdl
           scon_check_slibltdl_dir_msg="$scon_check_slibltdl_dir (from --with-libltdl)"])
    AS_IF([test ! -z "$with_libltdl_libdir" && test "$with_libltdl_libdir" != "yes"],
          [scon_check_slibltdl_libdir=$with_libltdl_libdir
           scon_check_slibltdl_libdir_msg="$scon_check_slibltdl_libdir (from --with-libltdl-libdir)"])

    scon_sdl_slibltdl_happy=no
    AS_IF([test "$with_slibltdl" != "no"],
          [AC_MSG_CHECKING([for libltdl dir])
           AC_MSG_RESULT([$scon_check_slibltdl_dir_msg])
           AC_MSG_CHECKING([for libltdl library dir])
           AC_MSG_RESULT([$scon_check_slibltdl_libdir_msg])

           SCON_CHECK_PACKAGE([scon_sdl_slibltdl],
                  [ltdl.h],
                  [ltdl],
                  [lt_dlopen],
                  [],
                  [$scon_check_slibltdl_dir],
                  [$scon_check_slibltdl_libdir],
                  [scon_sdl_slibltdl_happy=yes],
                  [scon_sdl_slibltdl_happy=no])
              ])

    # If we have slibltdl, do we have lt_dladvise?
    scon_sdl_slibltdl_have_lt_dladvise=0
    AS_IF([test "$scon_sdl_slibltdl_happy" = "yes"],
          [CPPFLAGS_save=$CPPFLAGS
           LDFLAGS_save=$LDFLAGS
           LIBS_save=$LIBS

           CPPFLAGS="$scon_sdl_slibltdl_CPPFLAGS $CPPFLAGS"
           LDFLAGS="$scon_sdl_slibltdl_LDFLAGS $LDFLAGS"
           LIBS="$scon_sdl_slibltdl_LIBS $LIBS"
           AC_CHECK_FUNC([lt_dladvise_init],
                         [scon_sdl_slibltdl_have_lt_dladvise=1])
           CPPFLAGS=$CPPFLAGS_save
           LDFLAGS=$LDFLAGS_save
           LIBS=$LIBS_save
          ])
    AC_DEFINE_UNQUOTED(SCON_PDL_PLIBLTDL_HAVE_LT_DLADVISE,
        [$scon_sdl_slibltdl_have_lt_dladvise],
        [Whether we have lt_dladvise or not])

    AS_IF([test "$scon_sdl_slibltdl_happy" = "yes"],
          [scon_sdl_slibltdl_ADD_CPPFLAGS=$scon_sdl_slibltdl_CPPFLAGS
           scon_sdl_slibltdl_ADD_LDFLAGS=$scon_sdl_slibltdl_LDFLAGS
           scon_sdl_slibltdl_ADD_LIBS=$scon_sdl_slibltdl_LIBS
           $1],
          [AS_IF([test ! -z "$with_libltdl" && \
                  test "$with_libltdl" != "no"],
                 [AC_MSG_WARN([libltdl support requested (via --with-libltdl) but not found.])
                  AC_MSG_ERROR([Cannot continue.])])
           $2])

    AC_SUBST(scon_sdl_slibltdl_CPPFLAGS)
    AC_SUBST(scon_sdl_slibltdl_LDFLAGS)
    AC_SUBST(scon_sdl_slibltdl_LIBS)

    SCON_VAR_SCOPE_POP
])
