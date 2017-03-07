# -*- shell-script -*-
#
# Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2013      Los Alamos National Security, LLC.  All rights reserved.
# Copyright (c) 2013-2016 Intel, Inc. All rights reserved
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# MCA_libpmix_CONFIG([action-if-found], [action-if-not-found])
# --------------------------------------------------------------------
AC_DEFUN([SCON_PMIX_CONFIG],[

   SCON_VAR_SCOPE_PUSH([scon_external_pmix_save_CPPFLAGS scon_external_pmix_save_LDFLAGS scon_external_pmix_save_LIBS])

    AC_ARG_WITH([pmix],
                [AC_HELP_STRING([--with-pmix(=DIR)],
                                [Build PMIx support.  DIR can take one of three values: "internal", "external", or a valid directory name.  "internal" (or no DIR value) forces SCON to use its internal copy of PMIx.  "external" forces SCON to use an external installation of PMIx.  Supplying a valid directory name also forces SCON to use an external installation of PMIx, and adds DIR/include, DIR/lib, and DIR/lib64 to the search path for headers and libraries. Note that SCON does not support --without-pmix.])])

    AS_IF([test "$with_pmix" = "no"],
          [AC_MSG_WARN([SCON requires PMIx support. It can be built])
           AC_MSG_WARN([with either its own internal copy of PMIx, or with])
           AC_MSG_WARN([an external copy that you supply.])
           AC_MSG_ERROR([Cannot continue])])

    AC_MSG_CHECKING([if user requested external PMIx support($with_pmix)])
    AS_IF([test -z "$with_pmix" || test "$with_pmix" = "yes" || test "$with_pmix" = "internal"],
          [AC_MSG_RESULT([no])
           scon_external_pmix_happy=no],

          [AC_MSG_RESULT([yes])
           # check for external pmix lib
           AS_IF([test "$with_pmix" = "external"],
                 [pmix_ext_install_dir=/usr],
                 [pmix_ext_install_dir=$with_pmix])

           # Make sure we have the headers and libs in the correct location
           SCON_CHECK_WITHDIR([external-pmix], [$pmix_ext_install_dir/include], [pmix.h])
           SCON_CHECK_WITHDIR([external-libpmix], [$pmix_ext_install_dir/lib], [libpmix.*])

           # check the version
           scon_external_pmix_save_CPPFLAGS=$CPPFLAGS
           scon_external_pmix_save_LDFLAGS=$LDFLAGS
           scon_external_pmix_save_LIBS=$LIB

           # if the pmix_version.h file does not exist, then
           # this must be from a pre-1.1.5 version
           AC_MSG_CHECKING([PMIx version])
           CPPFLAGS="-I$pmix_ext_install_dir/include $CPPFLAGS"
           AS_IF([test "x`ls $pmix_ext_install_dir/include/pmix_version.h 2> /dev/null`" = "x"],
                 [AC_MSG_RESULT([version file not found - assuming v1.1.4])
                  scon_external_pmix_version_found=1
                  scon_external_pmix_version=114],
                 [AC_MSG_RESULT([version file found])
                  scon_external_pmix_version_found=0])

           # if it does exist, then we need to parse it to find
           # the actual release series
           AS_IF([test "$scon_external_pmix_version_found" = "0"],
                 [AC_MSG_CHECKING([version 3x])
                  AC_PREPROC_IFELSE([AC_LANG_PROGRAM([
                                                      #include <pmix_version.h>
                                                      #if (PMIX_VERSION_MAJOR != 3L)
                                                      #error "not version 3"
                                                      #endif
                                                      ], [])],
                                    [AC_MSG_RESULT([found])
                                     scon_external_pmix_version=3x
                                     scon_external_pmix_version_found=1],
                                    [AC_MSG_RESULT([not found])])])

           AS_IF([test "$scon_external_pmix_version_found" = "0"],
                 [AC_MSG_CHECKING([version 2x])
                  AC_PREPROC_IFELSE([AC_LANG_PROGRAM([
                                                      #include <pmix_version.h>
                                                      #if (PMIX_VERSION_MAJOR != 2L)
                                                      #error "not version 2"
                                                      #endif
                                                      ], [])],
                                    [AC_MSG_RESULT([found])
                                     scon_external_pmix_version=2x
                                     scon_external_pmix_version_found=1],
                                    [AC_MSG_RESULT([not found])])])

           AS_IF([test "$scon_external_pmix_version_found" = "0"],
                 [AC_MSG_CHECKING([version 1x])
                  AC_PREPROC_IFELSE([AC_LANG_PROGRAM([
                                                      #include <pmix_version.h>
                                                      #if (PMIX_VERSION_MAJOR != 1L)
                                                      #error "not version 1"
                                                      #endif
                                                      ], [])],
                                    [AC_MSG_RESULT([found])
                                     scon_external_pmix_version=1x
                                     scon_external_pmix_version_found=1],
                                    [AC_MSG_RESULT([not found])])])

           AS_IF([test "x$scon_external_pmix_version" = "x"],
                 [AC_MSG_WARN([External PMIx support requested, but version])
                  AC_MSG_WARN([information of the external lib could not])
                  AC_MSG_WARN([be detected])
                  AC_MSG_ERROR([cannot continue])])

           CPPFLAGS=$opal_external_pmix_save_CPPFLAGS
           LDFLAGS=$opal_external_pmix_save_LDFLAGS
           LIBS=$opal_external_pmix_save_LIBS

           scon_external_pmix_CPPFLAGS="-I$pmix_ext_install_dir/include"
           scon_external_pmix_LDFLAGS=-L$pmix_ext_install_dir/lib
           scon_external_pmix_LIBS=-lpmix
           scon_external_pmix_happy=yes]
         #  CPPFLAGS="$CPPFLAGS $scon_external_pmix_CPPFLAGS"
         #  LDFLAGS="$LDFLAGS $scon_external_pmix_LIBS"
         #  LIBS="$LIBS $scon_external_pmix_LIBS"
           )

    #SCON_VAR_SCOPE_POP
])dnl
