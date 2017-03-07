dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2010-2015 Cisco Systems, Inc.  All rights reserved.
dnl Copyright (c) 2016      Intel, Inc. All right reserved
dnl Copyright (c) 2016      Research Organization for Information Science
dnl                         and Technology (RIST). All rights reserved.
dnl $COPYRIGHT$
dnl
dnl Additional copyrights may follow
dnl
dnl $HEADER$
dnl

dnl There will only be one component used in this framework, and it will
dnl be selected at configure time by priority.  Components must set
dnl their priorities in their configure.m4 file.

dnl We only want one winning component (vs. STOP_AT_FIRST_PRIORITY,
dnl which will allow all components of the same priority who succeed to
dnl win)
m4_define(MCA_scon_sdl_CONFIGURE_MODE, STOP_AT_FIRST)

AC_DEFUN([MCA_scon_sdl_CONFIG],[
    SCON_HAVE_SDL_SUPPORT=0

    # If --disable-dlopen was used, then have all the components fail
    # (we still need to configure them all so that things like "make
    # dist" work", but we just want the MCA system to (artificially)
    # conclude that it can't build any of the components.
    AS_IF([test $SCON_ENABLE_DLOPEN_SUPPORT -eq 0],
          [want_sdl=0], [want_sdl=1])

    MCA_CONFIGURE_FRAMEWORK([sdl], [$want_sdl])

    # If we found no suitable static sdl component and dlopen support
    # was not specifically disabled, this is an error.
    AS_IF([test "$MCA_scon_sdl_STATIC_COMPONENTS" = "" && \
           test $SCON_ENABLE_DLOPEN_SUPPORT -ne 0],
          [AC_MSG_WARN([Did not find a suitable static scon sdl component])
           AC_MSG_WARN([You might need to install libltld (and its headers) or])
           AC_MSG_WARN([specify --disable-dlopen to configure.])
           AC_MSG_ERROR([Cannot continue])])

    # If we have a winning component (which, per above, will only
    # happen if --disable-dlopen was *not* specified), do some more
    # logic.
    AS_IF([test "$MCA_scon_sdl_STATIC_COMPONENTS" != ""],
       [ # We had a winner -- w00t!

        SCON_HAVE_SDL_SUPPORT=1
        # If we added any -L flags to ADD_LDFLAGS, then we (might)
        # need to add those directories to LD_LIBRARY_PATH.
        # Otherwise, if we try to AC RUN_IFELSE anything here in
        # configure, it might die because it can't find the libraries
        # we just linked against.
        SCON_VAR_SCOPE_PUSH([scon_sdl_base_found_l scon_sdl_base_token scon_sdl_base_tmp scon_sdl_base_dir])
        scon_sdl_base_found_l=0
        eval "scon_sdl_base_tmp=\$scon_sdl_${scon_sdl_winner}_ADD_LIBS"
        for scon_sdl_base_token in $scon_sdl_base_tmp; do
            case $scon_sdl_base_token in
            -l*) scon_sdl_base_found_l=1 ;;
            esac
        done
        AS_IF([test $scon_sdl_base_found_l -eq 1],
              [eval "scon_sdl_base_tmp=\$scon_sdl_${scon_sdl_winner}_ADD_LDFLAGS"
               for scon_sdl_base_token in $scon_sdl_base_tmp; do
                   case $scon_sdl_base_token in
                   -L*)
                       scon_sdl_base_dir=`echo $scon_sdl_base_token | cut -c3-`
                       export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$scon_sdl_base_dir
                       AC_MSG_WARN([Adding to LD_LIBRARY_PATH: $scon_sdl_base_dir])
                       ;;
                   esac
               done])
        SCON_VAR_SCOPE_POP
    ])

    AC_DEFINE_UNQUOTED([SCON_HAVE_SDL_SUPPORT], [$SCON_HAVE_SDL_SUPPORT],
                       [Whether the SCON SDL framework is functional or not])
])
