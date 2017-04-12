dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
dnl Copyright (c) 2014-2017 Intel, Inc. All rights reserved.
dnl Copyright (c) 2014      Research Organization for Information Science
dnl                         and Technology (RIST). All rights reserved.
dnl
dnl $COPYRIGHT$
dnl
dnl Additional copyrights may follow
dnl
dnl $HEADER$
dnl

# SCON_CHECK_OS_FLAVOR_SPECIFIC()
# ----------------------------------------------------
# Helper macro from SCON-CHECK-OS-FLAVORS(), below.
# $1 = macro to look for
# $2 = suffix of env variable to set with results
AC_DEFUN([SCON_CHECK_OS_FLAVOR_SPECIFIC],
[
    AC_MSG_CHECKING([$1])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
     [[#ifndef $1
      error: this isnt $1
      #endif
     ]])],
                      [scon_found_$2=yes],
                      [scon_found_$2=no])
    AC_MSG_RESULT([$scon_found_$2])
])dnl

# SCON_CHECK_OS_FLAVORS()
# ----------------------------------------------------
# Try to figure out the various OS flavors out there.
#
AC_DEFUN([SCON_CHECK_OS_FLAVORS],
[
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__NetBSD__], [netbsd])
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__FreeBSD__], [freebsd])
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__OpenBSD__], [openbsd])
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__DragonFly__], [dragonfly])
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__386BSD__], [386bsd])
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__bsdi__], [bsdi])
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__APPLE__], [apple])
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__linux__], [linux])
    SCON_CHECK_OS_FLAVOR_SPECIFIC([__sun__], [sun])
    AS_IF([test "$scon_found_sun" = "no"],
          SCON_CHECK_OS_FLAVOR_SPECIFIC([__sun], [sun]))

    AS_IF([test "$scon_found_sun" = "yes"],
          [scon_have_solaris=1
           CFLAGS="$CFLAGS -D_REENTRANT"
           CPPFLAGS="$CPPFLAGS -D_REENTRANT"],
          [scon_have_solaris=0])
    AC_DEFINE_UNQUOTED([SCON_HAVE_SOLARIS],
                       [$scon_have_solaris],
                       [Whether or not we have solaris])

    # check for sockaddr_in (a good sign we have TCP)
    AC_CHECK_HEADERS([netdb.h netinet/in.h netinet/tcp.h])
    AC_CHECK_TYPES([struct sockaddr_in],
                   [scon_found_sockaddr=yes],
                   [scon_found_sockaddr=no],
                   [AC_INCLUDES_DEFAULT
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif])
])dnl
