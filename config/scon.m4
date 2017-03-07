dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
dnl                         University Research and Technology
dnl                         Corporation.  All rights reserved.
dnl Copyright (c) 2004-2005 The University of Tennessee and The University
dnl                         of Tennessee Research Foundation.  All rights
dnl                         reserved.
dnl Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
dnl                         University of Stuttgart.  All rights reserved.
dnl Copyright (c) 2004-2005 The Regents of the University of California.
dnl                         All rights reserved.
dnl Copyright (c) 2006-2016 Cisco Systems, Inc.  All rights reserved.
dnl Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
dnl Copyright (c) 2009      IBM Corporation.  All rights reserved.
dnl Copyright (c) 2009      Los Alamos National Security, LLC.  All rights
dnl                         reserved.
dnl Copyright (c) 2009-2011 Oak Ridge National Labs.  All rights reserved.
dnl Copyright (c) 2011-2013 NVIDIA Corporation.  All rights reserved.
dnl Copyright (c) 2013-2015 Intel, Inc. All rights reserved
dnl Copyright (c) 2015      Research Organization for Information Science
dnl                         and Technology (RIST). All rights reserved.
dnl Copyright (c) 2016      Mellanox Technologies, Inc.
dnl                         All rights reserved.
dnl
dnl $COPYRIGHT$
dnl
dnl Additional copyrights may follow
dnl
dnl $HEADER$
dnl

AC_DEFUN([SCON_SETUP_CORE],[

    AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])
    AC_REQUIRE([AC_CANONICAL_TARGET])
    AC_REQUIRE([AC_PROG_CC])

    # If no prefix was defined, set a good value
    m4_ifval([$1],
             [m4_define([scon_config_prefix],[$1/])],
             [m4_define([scon_config_prefix], [])])

    # Get scon's absolute top builddir (which may not be the same as
    # the real $top_builddir)
    SCON_startdir=`pwd`
    if test x"scon_config_prefix" != "x" && test ! -d "scon_config_prefix"; then
        mkdir -p "scon_config_prefix"
    fi
    if test x"scon_config_prefix" != "x"; then
        cd "scon_config_prefix"
    fi
    SCON_top_builddir=`pwd`
    AC_SUBST(SCON_top_builddir)

    # Get scon's absolute top srcdir (which may not be the same as the
    # real $top_srcdir.  First, go back to the startdir incase the
    # $srcdir is relative.

    cd "$SCON_startdir"
    cd "$srcdir"/scon_config_prefix
    SCON_top_srcdir="`pwd`"
    AC_SUBST(SCON_top_srcdir)

    # Go back to where we started
    cd "$SCON_startdir"

    AC_MSG_NOTICE([scon builddir: $SCON_top_builddir])
    AC_MSG_NOTICE([scon srcdir: $SCON_top_srcdir])
    if test "$SCON_top_builddir" != "$SCON_top_srcdir"; then
        AC_MSG_NOTICE([Detected VPATH build])
    fi

    # Get the version of scon that we are installing
    AC_MSG_CHECKING([for scon version])
    SCON_VERSION="`$SCON_top_srcdir/config/scon_get_version.sh $SCON_top_srcdir/VERSION`"
    if test "$?" != "0"; then
        AC_MSG_ERROR([Cannot continue])
    fi
    AC_MSG_RESULT([$SCON_VERSION])
    AC_SUBST(SCON_VERSION)
    AC_DEFINE_UNQUOTED([SCON_VERSION], ["$SCON_VERSION"],
                       [The library version is always available, contrary to VERSION])

    SCON_RELEASE_DATE="`$SCON_top_srcdir/config/scon_get_version.sh $SCON_top_srcdir/VERSION --release-date`"
    AC_SUBST(SCON_RELEASE_DATE)

    # Save the breakdown the version information
    AC_MSG_CHECKING([for scon major version])
    SCON_MAJOR_VERSION="`$SCON_top_srcdir/config/scon_get_version.sh $SCON_top_srcdir/VERSION --major`"
    if test "$?" != "0"; then
        AC_MSG_ERROR([Cannot continue])
    fi
    AC_SUBST(SCON_MAJOR_VERSION)
    AC_DEFINE_UNQUOTED([SCON_MAJOR_VERSION], [$SCON_MAJOR_VERSION],
                       [The library major version is always available, contrary to VERSION])

    AC_MSG_CHECKING([for scon minor version])
    SCON_MINOR_VERSION="`$SCON_top_srcdir/config/scon_get_version.sh $SCON_top_srcdir/VERSION --minor`"
    if test "$?" != "0"; then
        AC_MSG_ERROR([Cannot continue])
    fi
    AC_SUBST(SCON_MINOR_VERSION)
    AC_DEFINE_UNQUOTED([SCON_MINOR_VERSION], [$SCON_MINOR_VERSION],
                       [The library minor version is always available, contrary to VERSION])

    AC_MSG_CHECKING([for scon release version])
    SCON_RELEASE_VERSION="`$SCON_top_srcdir/config/scon_get_version.sh $SCON_top_srcdir/VERSION --release`"
    if test "$?" != "0"; then
        AC_MSG_ERROR([Cannot continue])
    fi
    AC_SUBST(SCON_RELEASE_VERSION)
    AC_DEFINE_UNQUOTED([SCON_RELEASE_VERSION], [$SCON_RELEASE_VERSION],
                       [The library release version is always available, contrary to VERSION])

    # Debug mode?
    AC_MSG_CHECKING([if want scon maintainer support])
    scon_debug=
    AS_IF([test "$scon_debug" = "" && test "$enable_debug" = "yes"],
          [scon_debug=1
           scon_debug_msg="enabled"])
    AS_IF([test "$scon_debug" = ""],
          [scon_debug=0
           scon_debug_msg="disabled"])
    # Grr; we use #ifndef for SCON_DEBUG!  :-(
    AH_TEMPLATE(SCON_ENABLE_DEBUG, [Whether we are in debugging mode or not])
    AS_IF([test "$scon_debug" = "1"], [AC_DEFINE([SCON_ENABLE_DEBUG])])
    AC_MSG_RESULT([$scon_debug_msg])

    AC_MSG_CHECKING([for scon directory prefix])
    AC_MSG_RESULT(m4_ifval([$1], scon_config_prefix, [(none)]))

    # Note that private/config.h *MUST* be listed first so that it
    # becomes the "main" config header file.  Any AC-CONFIG-HEADERS
    # after that (scon/config.h) will only have selective #defines
    # replaced, not the entire file.
    AC_CONFIG_HEADERS(scon_config_prefix[src/include/scon_config.h])

    # GCC specifics.
    if test "x$GCC" = "xyes"; then
        SCON_GCC_CFLAGS="-Wall -Wmissing-prototypes -Wundef"
        SCON_GCC_CFLAGS="$SCON_GCC_CFLAGS -Wpointer-arith -Wcast-align"
    fi

    ############################################################################
    # Check for compilers and preprocessors
    ############################################################################
    scon_show_title "Compiler and preprocessor tests"

    #
    # Check for some types
    #

    AC_CHECK_TYPES(int8_t)
    AC_CHECK_TYPES(uint8_t)
    AC_CHECK_TYPES(int16_t)
    AC_CHECK_TYPES(uint16_t)
    AC_CHECK_TYPES(int32_t)
    AC_CHECK_TYPES(uint32_t)
    AC_CHECK_TYPES(int64_t)
    AC_CHECK_TYPES(uint64_t)
    AC_CHECK_TYPES(long long)

    AC_CHECK_TYPES(intptr_t)
    AC_CHECK_TYPES(uintptr_t)
    AC_CHECK_TYPES(ptrdiff_t)

    #
    # Check for type sizes
    #

    AC_CHECK_SIZEOF(_Bool)
    AC_CHECK_SIZEOF(char)
    AC_CHECK_SIZEOF(short)
    AC_CHECK_SIZEOF(int)
    AC_CHECK_SIZEOF(long)
    if test "$ac_cv_type_long_long" = yes; then
        AC_CHECK_SIZEOF(long long)
    fi
    AC_CHECK_SIZEOF(float)
    AC_CHECK_SIZEOF(double)

    AC_CHECK_SIZEOF(void *)
    AC_CHECK_SIZEOF(size_t)
    if test "$ac_cv_type_ssize_t" = yes ; then
        AC_CHECK_SIZEOF(ssize_t)
    fi
    if test "$ac_cv_type_ptrdiff_t" = yes; then
        AC_CHECK_SIZEOF(ptrdiff_t)
    fi
    AC_CHECK_SIZEOF(wchar_t)

    AC_CHECK_SIZEOF(pid_t)

    #
    # Check for type alignments
    #

    SCON_C_GET_ALIGNMENT(bool, SCON_ALIGNMENT_BOOL)
    SCON_C_GET_ALIGNMENT(int8_t, SCON_ALIGNMENT_INT8)
    SCON_C_GET_ALIGNMENT(int16_t, SCON_ALIGNMENT_INT16)
    SCON_C_GET_ALIGNMENT(int32_t, SCON_ALIGNMENT_INT32)
    SCON_C_GET_ALIGNMENT(int64_t, SCON_ALIGNMENT_INT64)
    SCON_C_GET_ALIGNMENT(char, SCON_ALIGNMENT_CHAR)
    SCON_C_GET_ALIGNMENT(short, SCON_ALIGNMENT_SHORT)
    SCON_C_GET_ALIGNMENT(wchar_t, SCON_ALIGNMENT_WCHAR)
    SCON_C_GET_ALIGNMENT(int, SCON_ALIGNMENT_INT)
    SCON_C_GET_ALIGNMENT(long, SCON_ALIGNMENT_LONG)
    if test "$ac_cv_type_long_long" = yes; then
        SCON_C_GET_ALIGNMENT(long long, SCON_ALIGNMENT_LONG_LONG)
    fi
    SCON_C_GET_ALIGNMENT(float, SCON_ALIGNMENT_FLOAT)
    SCON_C_GET_ALIGNMENT(double, SCON_ALIGNMENT_DOUBLE)
    if test "$ac_cv_type_long_double" = yes; then
        SCON_C_GET_ALIGNMENT(long double, SCON_ALIGNMENT_LONG_DOUBLE)
    fi
    SCON_C_GET_ALIGNMENT(void *, SCON_ALIGNMENT_VOID_P)
    SCON_C_GET_ALIGNMENT(size_t, SCON_ALIGNMENT_SIZE_T)


    #
    # Does the C compiler native support "bool"? (i.e., without
    # <stdbool.h> or any other help)
    #

    SCON_VAR_SCOPE_PUSH([MSG])
    AC_MSG_CHECKING(for C bool type)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
                                          AC_INCLUDES_DEFAULT],
                                       [[bool bar, foo = true; bar = foo;]])],
                      [SCON_NEED_C_BOOL=0 MSG=yes],[SCON_NEED_C_BOOL=1 MSG=no])
    AC_DEFINE_UNQUOTED(SCON_NEED_C_BOOL, $SCON_NEED_C_BOOL,
                       [Whether the C compiler supports "bool" without any other help (such as <stdbool.h>)])
    AC_MSG_RESULT([$MSG])
    AC_CHECK_SIZEOF(_Bool)
    SCON_VAR_SCOPE_POP

    #
    # Check for other compiler characteristics
    #

    SCON_VAR_SCOPE_PUSH([SCON_CFLAGS_save])
    if test "$GCC" = "yes"; then

        # gcc 2.96 will emit oodles of warnings if you use "inline" with
        # -pedantic (which we do in developer builds).  However,
        # "__inline__" is ok.  So we have to force gcc to select the
        # right one.  If you use -pedantic, the AC_C_INLINE test will fail
        # (because it names a function foo() -- without the (void)).  So
        # we turn off all the picky flags, turn on -ansi mode (which is
        # implied by -pedantic), and set warnings to be errors.  Hence,
        # this does the following (for 2.96):
        #
        # - causes the check for "inline" to emit a warning, which then
        # fails
        # - checks for __inline__, which then emits no error, and works
        #
        # This also works nicely for gcc 3.x because "inline" will work on
        # the first check, and all is fine.  :-)

        SCON_CFLAGS_save=$CFLAGS
        CFLAGS="$SCON_CFLAGS_BEFORE_PICKY -Werror -ansi"
    fi
    AC_C_INLINE
    if test "$GCC" = "yes"; then
        CFLAGS=$SCON_CFLAGS_save
    fi
    SCON_VAR_SCOPE_POP

    if test "x$CC" = "xicc"; then
        SCON_CHECK_ICC_VARARGS
    fi


    ##################################
    # Only after setting up
    # C do we check compiler attributes.
    ##################################

    scon_show_subtitle "Compiler characteristics"

    SCON_CHECK_ATTRIBUTES
    SCON_CHECK_COMPILER_VERSION_ID

    ##################################
    # Header files
    ##################################

    scon_show_title "Header file tests"

    AC_CHECK_HEADERS([arpa/inet.h \
                      fcntl.h ifaddrs.h inttypes.h libgen.h \
                      net/if.h net/uio.h netinet/in.h \
                      stdint.h stddef.h \
                      stdlib.h string.h strings.h \
                      sys/ioctl.h sys/param.h \
                      sys/select.h sys/socket.h sys/sockio.h \
                      stdarg.h sys/stat.h sys/time.h \
                      sys/types.h sys/un.h sys/uio.h \
                      sys/wait.h syslog.h \
                      time.h unistd.h dirent.h \
                      crt_externs.h signal.h \
                      ioLib.h sockLib.h hostLib.h limits.h \
                      sys/statfs.h sys/statvfs.h])

    # Note that sometimes we have <stdbool.h>, but it doesn't work (e.g.,
    # have both Portland and GNU installed; using pgcc will find GNU's
    # <stdbool.h>, which all it does -- by standard -- is define "bool" to
    # "_Bool" [see
    # http://sconw.opengroup.org/onlinepubs/009695399/basedefs/stdbool.h.html],
    # and Portland has no idea what to do with _Bool).

    # So first figure out if we have <stdbool.h> (i.e., check the value of
    # the macro HAVE_STDBOOL_H from the result of AC_CHECK_HEADERS,
    # above).  If we do have it, then check to see if it actually works.
    # Define SCON_USE_STDBOOL_H as approrpaite.
    AC_CHECK_HEADERS([stdbool.h], [have_stdbool_h=1], [have_stdbool_h=0])
    AC_MSG_CHECKING([if <stdbool.h> works])
    if test "$have_stdbool_h" = "1"; then
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT[
                                                   #if HAVE_STDBOOL_H
                                                   #include <stdbool.h>
                                                   #endif
                                               ]],
                                           [[bool bar, foo = true; bar = foo;]])],
                          [SCON_USE_STDBOOL_H=1 MSG=yes],[SCON_USE_STDBOOL_H=0 MSG=no])
    else
        SCON_USE_STDBOOL_H=0
        MSG="no (don't have <stdbool.h>)"
    fi
    AC_DEFINE_UNQUOTED(SCON_USE_STDBOOL_H, $SCON_USE_STDBOOL_H,
                       [Whether to use <stdbool.h> or not])
    AC_MSG_RESULT([$MSG])

    # checkpoint results
    AC_CACHE_SAVE

    ##################################
    # Types
    ##################################

    scon_show_title "Type tests"

    AC_CHECK_TYPES([socklen_t, struct sockaddr_in, struct sockaddr_un,
                    struct sockaddr_in6, struct sockaddr_storage],
                   [], [], [AC_INCLUDES_DEFAULT
                            #if HAVE_SYS_SOCKET_H
                            #include <sys/socket.h>
                            #endif
                            #if HAVE_SYS_UN_H
                            #include <sys/un.h>
                            #endif
                            #ifdef HAVE_NETINET_IN_H
                            #include <netinet/in.h>
                            #endif
                           ])

    AC_CHECK_DECLS([AF_UNSPEC, PF_UNSPEC, AF_INET6, PF_INET6],
                   [], [], [AC_INCLUDES_DEFAULT
                            #if HAVE_SYS_SOCKET_H
                            #include <sys/socket.h>
                            #endif
                            #ifdef HAVE_NETINET_IN_H
                            #include <netinet/in.h>
                            #endif
                           ])

    # SA_RESTART in signal.h
    SCON_VAR_SCOPE_PUSH([MSG2])
    AC_MSG_CHECKING([if SA_RESTART defined in signal.h])
                        AC_EGREP_CPP(yes, [
                                            #include <signal.h>
                                            #ifdef SA_RESTART
                                            yes
                                            #endif
                                        ], [MSG2=yes VALUE=1], [MSG2=no VALUE=0])
    AC_DEFINE_UNQUOTED(SCON_HAVE_SA_RESTART, $VALUE,
                       [Whether we have SA_RESTART in <signal.h> or not])
    AC_MSG_RESULT([$MSG2])
    SCON_VAR_SCOPE_POP

    AC_CHECK_MEMBERS([struct sockaddr.sa_len], [], [], [
                         #include <sys/types.h>
                         #if HAVE_SYS_SOCKET_H
                         #include <sys/socket.h>
                         #endif
                     ])

    AC_CHECK_MEMBERS([struct dirent.d_type], [], [], [
                         #include <sys/types.h>
                         #include <dirent.h>])

    AC_CHECK_MEMBERS([siginfo_t.si_fd],,,[#include <signal.h>])
    AC_CHECK_MEMBERS([siginfo_t.si_band],,,[#include <signal.h>])

    #
    # Checks for struct member names in struct statfs
    #
    AC_CHECK_MEMBERS([struct statfs.f_type], [], [], [
                         AC_INCLUDES_DEFAULT
                         #ifdef HAVE_SYS_VFS_H
                         #include <sys/vfs.h>
                         #endif
                         #ifdef HAVE_SYS_STATFS_H
                         #include <sys/statfs.h>
                         #endif
                     ])

    AC_CHECK_MEMBERS([struct statfs.f_fstypename], [], [], [
                         AC_INCLUDES_DEFAULT
                         #ifdef HAVE_SYS_PARAM_H
                         #include <sys/param.h>
                         #endif
                         #ifdef HAVE_SYS_MOUNT_H
                         #include <sys/mount.h>
                         #endif
                         #ifdef HAVE_SYS_VFS_H
                         #include <sys/vfs.h>
                         #endif
                         #ifdef HAVE_SYS_STATFS_H
                         #include <sys/statfs.h>
                         #endif
                     ])

    #
    # Checks for struct member names in struct statvfs
    #
    AC_CHECK_MEMBERS([struct statvfs.f_basetype], [], [], [
                         AC_INCLUDES_DEFAULT
                         #ifdef HAVE_SYS_STATVFS_H
                         #include <sys/statvfs.h>
                         #endif
                     ])

    AC_CHECK_MEMBERS([struct statvfs.f_fstypename], [], [], [
                         AC_INCLUDES_DEFAULT
                         #ifdef HAVE_SYS_STATVFS_H
                         #include <sys/statvfs.h>
                         #endif
                     ])

    AC_CHECK_MEMBERS([struct ucred.uid, struct ucred.cr_uid, struct sockpeercred.uid],
                     [], [],
                     [#include <sys/types.h>
                      #include <sys/socket.h> ])

    #
    # Check for ptrdiff type.  Yes, there are platforms where
    # sizeof(void*) != sizeof(long) (64 bit Windows, apparently).
    #
    AC_MSG_CHECKING([for pointer diff type])
    if test $ac_cv_type_ptrdiff_t = yes ; then
        scon_ptrdiff_t="ptrdiff_t"
        scon_ptrdiff_size=$ac_cv_sizeof_ptrdiff_t
    elif test $ac_cv_sizeof_void_p -eq $ac_cv_sizeof_long ; then
        scon_ptrdiff_t="long"
        scon_ptrdiff_size=$ac_cv_sizeof_long
    elif test $ac_cv_type_long_long = yes && test $ac_cv_sizeof_void_p -eq $ac_cv_sizeof_long_long ; then
        scon_ptrdiff_t="long long"
        scon_ptrdiff_size=$ac_cv_sizeof_long_long
        #else
        #    AC_MSG_ERROR([Could not find datatype to emulate ptrdiff_t.  Cannot continue])
    fi
    AC_DEFINE_UNQUOTED([SCON_PTRDIFF_TYPE], [$scon_ptrdiff_t],
                       [type to use for ptrdiff_t])
    AC_MSG_RESULT([$scon_ptrdiff_t (size: $scon_ptrdiff_size)])

    ##################################
    # Linker characteristics
    ##################################

    AC_MSG_CHECKING([the linker for support for the -fini option])
    SCON_VAR_SCOPE_PUSH([LDFLAGS_save])
    LDFLAGS_save=$LDFLAGS
    LDFLAGS="$LDFLAGS_save -Wl,-fini -Wl,finalize"
    AC_TRY_LINK([void finalize (void) {}], [], [AC_MSG_RESULT([yes])
            scon_ld_have_fini=1], [AC_MSG_RESULT([no])
            scon_ld_have_fini=0])
    LDFLAGS=$LDFLAGS_save
    SCON_VAR_SCOPE_POP

    scon_destructor_use_fini=0
    scon_no_destructor=0
    if test x$scon_cv___attribute__destructor = x0 ; then
        if test x$scon_ld_have_fini = x1 ; then
            scon_destructor_use_fini=1
        else
            scon_no_destructor=1;
        fi
    fi

    AC_DEFINE_UNQUOTED(SCON_NO_LIB_DESTRUCTOR, [$scon_no_destructor],
        [Whether libraries can be configured with destructor functions])
    AM_CONDITIONAL(SCON_DESTRUCTOR_USE_FINI, [test x$scon_destructor_use_fini = x1])

    ##################################
    # Libraries
    ##################################

    scon_show_title "Library and Function tests"

    SCON_SEARCH_LIBS_CORE([socket], [socket])

    # IRIX and CentOS have dirname in -lgen, usually in libc
    SCON_SEARCH_LIBS_CORE([dirname], [gen])

    # Darwin doesn't need -lm, as it's a symlink to libSystem.dylib
    SCON_SEARCH_LIBS_CORE([ceil], [m])

    AC_CHECK_FUNCS([asprintf snprintf vasprintf vsnprintf strsignal socketpair strncpy_s usleep statfs statvfs getpeereid strnlen])

    # On some hosts, htonl is a define, so the AC_CHECK_FUNC will get
    # confused.  On others, it's in the standard library, but stubbed with
    # the magic glibc foo as not implemented.  and on other systems, it's
    # just not there.  This covers all cases.
    AC_CACHE_CHECK([for htonl define],
                   [scon_cv_htonl_define],
                   [AC_PREPROC_IFELSE([AC_LANG_PROGRAM([
                                                          #ifdef HAVE_SYS_TYPES_H
                                                          #include <sys/types.h>
                                                          #endif
                                                          #ifdef HAVE_NETINET_IN_H
                                                          #include <netinet/in.h>
                                                          #endif
                                                          #ifdef HAVE_ARPA_INET_H
                                                          #include <arpa/inet.h>
                                                          #endif],[
                                                          #ifndef ntohl
                                                          #error "ntohl not defined"
                                                          #endif
                                                      ])], [scon_cv_htonl_define=yes], [scon_cv_htonl_define=no])])
    AC_CHECK_FUNC([htonl], [scon_have_htonl=yes], [scon_have_htonl=no])
    AS_IF([test "$scon_cv_htonl_define" = "yes" || test "$scon_have_htonl" = "yes"],
          [AC_DEFINE_UNQUOTED([HAVE_UNIX_BYTESWAP], [1],
                              [whether unix byteswap routines -- htonl, htons, nothl, ntohs -- are available])])

    # check pandoc separately so we can setup an AM_CONDITIONAL off it
    AC_CHECK_PROG([scon_have_pandoc], [pandoc], [yes], [no])
    AM_CONDITIONAL([SCON_HAVE_PANDOC], [test "x$scon_have_pandoc" = "xyes"])

    #
    # Make sure we can copy va_lists (need check declared, not linkable)
    #

    AC_CHECK_DECL(va_copy, SCON_HAVE_VA_COPY=1, SCON_HAVE_VA_COPY=0,
                  [#include <stdarg.h>])
    AC_DEFINE_UNQUOTED(SCON_HAVE_VA_COPY, $SCON_HAVE_VA_COPY,
                       [Whether we have va_copy or not])

    AC_CHECK_DECL(__va_copy, SCON_HAVE_UNDERSCORE_VA_COPY=1,
                  SCON_HAVE_UNDERSCORE_VA_COPY=0, [#include <stdarg.h>])
    AC_DEFINE_UNQUOTED(SCON_HAVE_UNDERSCORE_VA_COPY, $SCON_HAVE_UNDERSCORE_VA_COPY,
                       [Whether we have __va_copy or not])

    AC_CHECK_DECLS(__func__)

    # checkpoint results
    AC_CACHE_SAVE

    ##################################
    # System-specific tests
    ##################################

    scon_show_title "System-specific tests"

    AC_C_BIGENDIAN
    SCON_CHECK_BROKEN_QSORT

    ##################################
    # Visibility
    ##################################

    # Check the visibility declspec at the end to avoid problem with
    # the previous tests that are not necessarily prepared for
    # the visibility feature.
    scon_show_title "Symbol visibility feature"

    SCON_CHECK_VISIBILITY

    ##################################
    # Libevent
    ##################################
    scon_show_title "Libevent"

    SCON_LIBEVENT_CONFIG

    ##################################
    # PMIx
    ##################################
    scon_show_title "PMIx"

    SCON_PMIX_CONFIG

    ##################################
    # MCA
    ##################################

    scon_show_title "Modular Component Architecture (MCA) setup"

    AC_MSG_CHECKING([for subdir args])
    SCON_CONFIG_SUBDIR_ARGS([scon_subdir_args])
    AC_MSG_RESULT([$scon_subdir_args])

    SCON_MCA

    ############################################################################
    # final compiler config
    ############################################################################

    scon_show_subtitle "Set path-related compiler flags"

    #
    # This is needed for VPATH builds, so that it will -I the appropriate
    # include directory.  We delayed doing it until now just so that
    # '-I$(top_srcdir)' doesn't show up in any of the configure output --
    # purely aesthetic.
    #
    # Because scon_config.h is created by AC_CONFIG_HEADERS, we
    # don't need to -I the builddir for scon/include. However, if we
    # are VPATH building, we do need to include the source directories.
    #
    if test "$SCON_top_builddir" != "$SCON_top_srcdir"; then
        # Note the embedded m4 directives here -- we must embed them
        # rather than have successive assignments to these shell
        # variables, lest the $(foo) names try to get evaluated here.
        # Yuck!
        CPPFLAGS='-I$(SCON_top_builddir) -I$(SCON_top_srcdir) -I$(SCON_top_srcdir)/src -I$(SCON_top_builddir)/include -I$(SCON_top_srcdir)/include'" $CPPFLAGS"
    else
        CPPFLAGS='-I$(SCON_top_srcdir) -I$(SCON_top_srcdir)/src -I$(SCON_top_srcdir)/include'" $CPPFLAGS"
    fi

    # scondatadir, sconlibdir, and sconinclude are essentially the same as
    # pkg*dir, but will always be */scon.
    scondatadir='${datadir}/scon'
    sconlibdir='${libdir}/scon'
    sconincludedir='${includedir}/scon'
    AC_SUBST(scondatadir)
    AC_SUBST(sconlibdir)
    AC_SUBST(sconincludedir)

    ############################################################################
    # final output
    ############################################################################

    scon_show_subtitle "Final output"

    AC_CONFIG_FILES(
        scon_config_prefix[Makefile]
        scon_config_prefix[config/Makefile]
        scon_config_prefix[include/Makefile]
        scon_config_prefix[src/Makefile]
        scon_config_prefix[src/util/keyval/Makefile]
        scon_config_prefix[src/mca/base/Makefile]
        scon_config_prefix[src/test/Makefile]
        )

    # Success
    $2
])dnl

AC_DEFUN([SCON_DEFINE_ARGS],[
    # Embedded mode, or standalone?
    AC_MSG_CHECKING([if embedded mode is enabled])
    AC_ARG_ENABLE([embedded-mode],
        [AC_HELP_STRING([--enable-embedded-mode],
                [Using --enable-embedded-mode causes SCON to skip a few configure checks and install nothing.  It should only be used when building SCON within the scope of a larger package.])])
    AS_IF([test ! -z "$enable_embedded_mode" && test "$enable_embedded_mode" = "yes"],
          [scon_mode=embedded
           AC_MSG_RESULT([yes])],
          [scon_mode=standalone
           AC_MSG_RESULT([no])])

    # Rename symbols?
    AC_ARG_WITH([scon-symbol-rename],
                AC_HELP_STRING([--with-scon-symbol-rename=FILE],
                               [Provide an include file that contains directives to rename SCON symbols]))
    AS_IF([test ! -z "$with_scon_symbol_rename" && test "$with_scon_symbol_rename" != "yes"],
          [scon_symbol_rename="$with_scon_symbol_rename"],
          [scon_symbol_rename=\"src/include/rename.h\"])
    AC_DEFINE_UNQUOTED(SCON_SYMBOL_RENAME, [$scon_symbol_rename],
                       [The scon symbol rename include directive])

    # Install tests and examples?
    AC_MSG_CHECKING([if tests and examples are to be installed])
    AC_ARG_WITH([tests-examples],
        [AC_HELP_STRING([--with-tests-examples],
                [Whether or not to install the tests and example programs.])])
    AS_IF([test ! -z "$with_tests_examples" && test "$with_tests_examples" = "no"],
          [scon_tests=no
           AC_MSG_RESULT([no])],
          [scon_tests=yes
           AC_MSG_RESULT([yes])])

#
# Is this a developer copy?
#

if test -d .git; then
    SCON_DEVEL=1
else
    SCON_DEVEL=0
fi


#
# Developer picky compiler options
#

AC_MSG_CHECKING([if want developer-level compiler pickyness])
AC_ARG_ENABLE(picky,
    AC_HELP_STRING([--enable-picky],
                   [enable developer-level compiler pickyness when building SCON (default: disabled)]))
if test "$enable_picky" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_PICKY_COMPILER=1
else
    AC_MSG_RESULT([no])
    WANT_PICKY_COMPILER=0
fi
#################### Early development override ####################
if test "$WANT_PICKY_COMPILER" = "0" && test -z "$enable_picky" && test "$SCON_DEVEL" = "1"; then
    WANT_PICKY_COMPILER=1
    echo "--> developer override: enable picky compiler by default"
fi
#################### Early development override ####################

#
# Developer debugging
#

AC_MSG_CHECKING([if want developer-level debugging code])
AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug],
                   [enable developer-level debugging code (not for general SCON users!) (default: disabled)]))
if test "$enable_debug" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_DEBUG=1
else
    AC_MSG_RESULT([no])
    WANT_DEBUG=0
fi
#################### Early development override ####################
if test "$WANT_DEBUG" = "0" && test -z "$enable_debug" && test "$SCON_DEVEL" = "1"; then
    WANT_DEBUG=1
    echo "--> developer override: enable debugging code by default"
fi
#################### Early development override ####################
if test "$WANT_DEBUG" = "0"; then
    CFLAGS="-DNDEBUG $CFLAGS"
    CXXFLAGS="-DNDEBUG $CXXFLAGS"
fi
AC_DEFINE_UNQUOTED(SCON_ENABLE_DEBUG, $WANT_DEBUG,
                   [Whether we want developer-level debugging code or not])

AC_ARG_ENABLE(debug-symbols,
              AC_HELP_STRING([--disable-debug-symbols],
                             [Disable adding compiler flags to enable debugging symbols if --enable-debug is specified.  For non-debugging builds, this flag has no effect.]))

#
# Do we want to install the internal devel headers?
#
AC_MSG_CHECKING([if want to install project-internal header files])
AC_ARG_WITH(devel-headers,
    AC_HELP_STRING([--with-devel-headers],
                   [normal SCON users/applications do not need this (scon.h and friends are ALWAYS installed).  Developer headers are only necessary for authors doing deeper integration (default: disabled).]))
if test "$with_devel_headers" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_INSTALL_HEADERS=1
else
    AC_MSG_RESULT([no])
    WANT_INSTALL_HEADERS=0
fi
AM_CONDITIONAL(WANT_INSTALL_HEADERS, test "$WANT_INSTALL_HEADERS" = 1)

#
# Support per-user config files?
#
AC_ARG_ENABLE([per-user-config-files],
   [AC_HELP_STRING([--enable-per-user-config-files],
      [Disable per-user configuration files, to save disk accesses during job start-up.  This is likely desirable for large jobs.  Note that this can also be acheived by environment variables at run-time.  (default: enabled)])])
if test "$enable_per_user_config_files" = "no" ; then
  result=0
else
  result=1
fi
AC_DEFINE_UNQUOTED([SCON_WANT_HOME_CONFIG_FILES], [$result],
     [Enable per-user config files])

#
# Do we want the pretty-print stack trace feature?
#

AC_MSG_CHECKING([if want pretty-print stacktrace])
AC_ARG_ENABLE([pretty-print-stacktrace],
              [AC_HELP_STRING([--enable-pretty-print-stacktrace],
                              [Pretty print stacktrace on process signal (default: enabled)])])
if test "$enable_pretty_print_stacktrace" = "no" ; then
    AC_MSG_RESULT([no])
    WANT_PRETTY_PRINT_STACKTRACE=0
else
    AC_MSG_RESULT([yes])
    WANT_PRETTY_PRINT_STACKTRACE=1
fi
AC_DEFINE_UNQUOTED([SCON_WANT_PRETTY_PRINT_STACKTRACE],
                   [$WANT_PRETTY_PRINT_STACKTRACE],
                   [if want pretty-print stack trace feature])

#
# Do we want the shared memory datastore usage?
#

AC_MSG_CHECKING([if want special dstore usage])
AC_ARG_ENABLE([dstore],
              [AC_HELP_STRING([--enable-dstore],
                              [Using special datastore (default: disabled)])])
if test "$enable_dstore" = "yes" ; then
    AC_MSG_RESULT([yes])
    WANT_DSTORE=1
else
    AC_MSG_RESULT([no])
    WANT_DSTORE=0
fi
AC_DEFINE_UNQUOTED([SCON_ENABLE_DSTORE],
                   [$WANT_DSTORE],
                   [if want special dstore feature])
AM_CONDITIONAL([WANT_DSTORE],[test "x$enable_dstore" = "xyes"])

#
# Ident string
#
AC_MSG_CHECKING([if want ident string])
AC_ARG_WITH([ident-string],
            [AC_HELP_STRING([--with-ident-string=STRING],
                            [Embed an ident string into SCON object files])])
if test "$with_ident_string" = "" || test "$with_ident_string" = "no"; then
    with_ident_string="%VERSION%"
fi
# This is complicated, because $SCON_VERSION may have spaces in it.
# So put the whole sed expr in single quotes -- i.e., directly
# substitute %VERSION% for (not expanded) $SCON_VERSION.
with_ident_string="`echo $with_ident_string | sed -e 's/%VERSION%/$SCON_VERSION/'`"

# Now eval an echo of that so that the "$SCON_VERSION" token is
# replaced with its value.  Enclose the whole thing in "" so that it
# ends up as 1 token.
with_ident_string="`eval echo $with_ident_string`"

AC_DEFINE_UNQUOTED([SCON_IDENT_STRING], ["$with_ident_string"],
                   [ident string for SCON])
AC_MSG_RESULT([$with_ident_string])

#
# Timing support
#
AC_MSG_CHECKING([if want developer-level timing support])
AC_ARG_ENABLE(timing,
              AC_HELP_STRING([--enable-timing],
                             [enable developer-level timing code (default: disabled)]))
if test "$enable_timing" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_TIMING=1
else
    AC_MSG_RESULT([no])
    WANT_TIMING=0
fi

AC_DEFINE_UNQUOTED([SCON_ENABLE_TIMING], [$WANT_TIMING],
                   [Whether we want developer-level timing support or not])

#
# Install header files
#
AC_MSG_CHECKING([if want to head developer-level header files])
AC_ARG_WITH(devel-headers,
              AC_HELP_STRING([--with-devel-headers],
                             [also install developer-level header files (only for internal SCON developers, default: disabled)]))
if test "$with_devel_headers" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_INSTALL_HEADERS=1
else
    AC_MSG_RESULT([no])
    WANT_INSTALL_HEADERS=0
fi

AM_CONDITIONAL([WANT_INSTALL_HEADERS], [test $WANT_INSTALL_HEADERS -eq 1])
])dnl

# This must be a standalone routine so that it can be called both by
# SCON_INIT and an external caller (if SCON_INIT is not invoked).
AC_DEFUN([SCON_DO_AM_CONDITIONALS],[
    AS_IF([test "$scon_did_am_conditionals" != "yes"],[
        AM_CONDITIONAL([SCON_EMBEDDED_MODE], [test "x$scon_mode" = "xembedded"])
        AM_CONDITIONAL([SCON_COMPILE_TIMING], [test "$WANT_TIMING" = "1"])
        AM_CONDITIONAL([SCON_WANT_MUNGE], [test "$scon_munge_support" = "1"])
        AM_CONDITIONAL([SCON_WANT_SASL], [test "$scon_sasl_support" = "1"])
    ])
    scon_did_am_conditionals=yes
])dnl

