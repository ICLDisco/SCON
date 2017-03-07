# -*- shell-script -*-
# Copyright (c) 2016      Intel, Inc. All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# MCA_pt2pt_tcp_CONFIG([action-if-found], [action-if-not-found])
# -----------------------------------------------------------
AC_DEFUN([MCA_scon_pt2pt_tcp_CONFIG],[
    AC_CONFIG_FILES([src/mca/pt2pt/tcp/Makefile])

    # check for sockaddr_in (a good sign we have TCP)
    AC_CHECK_TYPES([struct sockaddr_in],
                   [pt2pt_tcp_happy="yes"],
                   [pt2pt_tcp_happy="no"],
                   [AC_INCLUDES_DEFAULT
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif])

    AS_IF([test "$pt2pt_tcp_happy" = "yes"], [$1], [$2])
])dnl
