# -*- shell-script -*-
#
# Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2013      Los Alamos National Security, LLC.  All rights reserved.
# Copyright (c) 2013-2017 Intel, Inc. All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# MCA_libevent_CONFIG([action-if-found], [action-if-not-found])
# --------------------------------------------------------------------
AC_DEFUN([SCON_LIBEVENT_CONFIG],[
    AC_ARG_WITH([libevent-header],
                [AC_HELP_STRING([--with-libevent-header=HEADER],
                                [The value that should be included in C files to include event.h])])

    AC_ARG_ENABLE([embedded-libevent],
                  [AC_HELP_STRING([--enable-embedded-libevent],
                                  [Enable use of locally embedded libevent])])

    AS_IF([test "$enable_embedded_libevent" = "yes"],
          [_SCON_LIBEVENT_EMBEDDED_MODE],
          [_SCON_LIBEVENT_EXTERNAL])

    AC_MSG_CHECKING([libevent header])
    AC_DEFINE_UNQUOTED([SCON_EVENT_HEADER], [$SCON_EVENT_HEADER],
                       [Location of event.h])
    AC_MSG_RESULT([$SCON_EVENT_HEADER])
    AC_MSG_CHECKING([libevent2/thread header])
    AC_DEFINE_UNQUOTED([SCON_EVENT2_THREAD_HEADER], [$SCON_EVENT2_THREAD_HEADER],
                       [Location of event2/thread.h])
    AC_MSG_RESULT([$SCON_EVENT2_THREAD_HEADER])
])

AC_DEFUN([_SCON_LIBEVENT_EMBEDDED_MODE],[
    AC_MSG_CHECKING([for libevent])
    AC_MSG_RESULT([assumed available (embedded mode)])

    SCON_EVENT_HEADER="$with_libevent_header"
    SCON_EVENT2_THREAD_HEADER="$with_libevent_header"

 ])

AC_DEFUN([_SCON_LIBEVENT_EXTERNAL],[
    SCON_VAR_SCOPE_PUSH([scon_event_dir scon_event_libdir])

    AC_ARG_WITH([libevent],
                [AC_HELP_STRING([--with-libevent=DIR],
                                [Search for libevent headers and libraries in DIR ])])

    # Bozo check
    AS_IF([test "$with_libevent" = "no"],
          [AC_MSG_WARN([It is not possible to configure SCON --without-libevent])
           AC_MSG_ERROR([Cannot continue])])

    AC_ARG_WITH([libevent-libdir],
                [AC_HELP_STRING([--with-libevent-libdir=DIR],
                                [Search for libevent libraries in DIR ])])

    AC_MSG_CHECKING([for libevent in])
    if test ! -z "$with_libevent" && test "$with_libevent" != "yes"; then
        scon_event_dir=$with_libevent
        if test -d $with_libevent/lib; then
            scon_event_libdir=$with_libevent/lib
        elif test -d $with_libevent/lib64; then
            scon_event_libdir=$with_libevent/lib64
        else
            AC_MSG_RESULT([Could not find $with_libevent/lib or $with_libevent/lib64])
            AC_MSG_ERROR([Can not continue])
        fi
        AC_MSG_RESULT([$scon_event_dir and $scon_event_libdir])
    else
        AC_MSG_RESULT([(default search paths)])
    fi
    AS_IF([test ! -z "$with_libevent_libdir" && "$with_libevent_libdir" != "yes"],
          [scon_event_libdir="$with_libevent_libdir"])

    SCON_CHECK_PACKAGE([scon_libevent],
                       [event.h],
                       [event],
                       [event_config_new],
                       [-levent -levent_pthreads],
                       [$scon_event_dir],
                       [$scon_event_libdir],
                       [],
                       [AC_MSG_WARN([LIBEVENT SUPPORT NOT FOUND])
                        AC_MSG_ERROR([CANNOT CONTINE])])

    # Ensure that this libevent has the symbol
    # "evthread_set_lock_callbacks", which will only exist if
    # libevent was configured with thread support.
    AC_CHECK_LIB([event], [evthread_set_lock_callbacks],
                 [],
                 [AC_MSG_WARN([External libevent does not have thread support])
                  AC_MSG_WARN([SCON requires libevent to be compiled with])
                  AC_MSG_WARN([thread support enabled])
                  AC_MSG_ERROR([Cannot continue])])
    AC_CHECK_LIB([event_pthreads], [evthread_use_pthreads],
                 [],
                 [AC_MSG_WARN([External libevent does not have thread support])
                  AC_MSG_WARN([SCON requires libevent to be compiled with])
                  AC_MSG_WARN([thread support enabled])
                  AC_MSG_ERROR([Cannot continue])])
    # Chck if this libevent has the symbol
    # "libevent_global_shutdown", which will only exist in
    # libevent version 2.1.1+
    AC_CHECK_FUNCS([libevent_global_shutdown],[], [])

    # Set output variables
    SCON_EVENT_HEADER="<event.h>"
    SCON_EVENT2_THREAD_HEADER="<event2/thread.h>"

    SCON_VAR_SCOPE_POP
])dnl
