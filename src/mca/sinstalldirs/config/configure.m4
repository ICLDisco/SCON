# -*- shell-script -*-
#
# Copyright (c) 2006      Los Alamos National Security, LLC.  All rights
#                         reserved.
# Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2016      Intel, Inc. All rights reserved
# Copyright (c) 2016      Research Organization for Information Science
#                         and Technology (RIST). All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

AC_DEFUN([MCA_scon_sinstalldirs_config_PRIORITY], [0])

AC_DEFUN([MCA_scon_sinstalldirs_config_COMPILE_MODE], [
    AC_MSG_CHECKING([for MCA component $1:$2 compile mode])
    $3="static"
    AC_MSG_RESULT([$$3])
])


# MCA_sinstalldirs_config_CONFIG(action-if-can-compile,
#                        [action-if-cant-compile])
# ------------------------------------------------
AC_DEFUN([MCA_scon_sinstalldirs_config_CONFIG],[
    AC_CONFIG_FILES([src/mca/sinstalldirs/config/Makefile
                     src/mca/sinstalldirs/config/sinstall_dirs.h])
])

