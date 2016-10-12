dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
dnl Copyright (c) 2015      Intel, Inc. All rights reserved
dnl $COPYRIGHT$
dnl
dnl Additional copyrights may follow
dnl
dnl $HEADER$
dnl
dnl defines:
dnl   SCON_$1_USE_PRAGMA_IDENT
dnl   SCON_$1_USE_IDENT
dnl   SCON_$1_USE_CONST_CHAR_IDENT
dnl

# SCON_CHECK_IDENT(compiler-env, compiler-flags,
# file-suffix, lang) Try to compile a source file containing
# a #pragma ident, and determine whether the ident was
# inserted into the resulting object file
# -----------------------------------------------------------
AC_DEFUN([SCON_CHECK_IDENT], [
    AC_MSG_CHECKING([for $4 ident string support])

    scon_pragma_ident_happy=0
    scon_ident_happy=0
    scon_static_const_char_happy=0
    _SCON_CHECK_IDENT(
        [$1], [$2], [$3],
        [[#]pragma ident], [],
        [scon_pragma_ident_happy=1
         scon_message="[#]pragma ident"],
        _SCON_CHECK_IDENT(
            [$1], [$2], [$3],
            [[#]ident], [],
            [scon_ident_happy=1
             scon_message="[#]ident"],
            _SCON_CHECK_IDENT(
                [$1], [$2], [$3],
                [[#]pragma comment(exestr, ], [)],
                [scon_pragma_comment_happy=1
                 scon_message="[#]pragma comment"],
                [scon_static_const_char_happy=1
                 scon_message="static const char[[]]"])))

    AC_DEFINE_UNQUOTED([SCON_$1_USE_PRAGMA_IDENT],
        [$scon_pragma_ident_happy], [Use #pragma ident strings for $4 files])
    AC_DEFINE_UNQUOTED([SCON_$1_USE_IDENT],
        [$scon_ident_happy], [Use #ident strings for $4 files])
    AC_DEFINE_UNQUOTED([SCON_$1_USE_PRAGMA_COMMENT],
        [$scon_pragma_comment_happy], [Use #pragma comment for $4 files])
    AC_DEFINE_UNQUOTED([SCON_$1_USE_CONST_CHAR_IDENT],
        [$scon_static_const_char_happy], [Use static const char[] strings for $4 files])

    AC_MSG_RESULT([$scon_message])

    unset scon_pragma_ident_happy scon_ident_happy scon_static_const_char_happy scon_message
])

# _SCON_CHECK_IDENT(compiler-env, compiler-flags,
# file-suffix, header_prefix, header_suffix, action-if-success, action-if-fail)
# Try to compile a source file containing a #-style ident,
# and determine whether the ident was inserted into the
# resulting object file
# -----------------------------------------------------------
AC_DEFUN([_SCON_CHECK_IDENT], [
    eval scon_compiler="\$$1"
    eval scon_flags="\$$2"

    scon_ident="string_not_coincidentally_inserted_by_the_compiler"
    cat > conftest.$3 <<EOF
$4 "$scon_ident" $5
int main(int argc, char** argv);
int main(int argc, char** argv) { return 0; }
EOF

    # "strings" won't always return the ident string.  objdump isn't
    # universal (e.g., OS X doesn't have it), and ...other
    # complications.  So just try to "grep" for the string in the
    # resulting object file.  If the ident is found in "strings" or
    # the grep succeeds, rule that we have this flavor of ident.

    echo "configure:__oline__: $1" >&5
    scon_output=`$scon_compiler $scon_flags -c conftest.$3 -o conftest.${OBJEXT} 2>&1 1>/dev/null`
    scon_status=$?
    AS_IF([test $scon_status = 0],
          [test -z "$scon_output"
           scon_status=$?])
    SCON_LOG_MSG([\$? = $scon_status], 1)
    AS_IF([test $scon_status = 0 && test -f conftest.${OBJEXT}],
          [scon_output="`strings -a conftest.${OBJEXT} | grep $scon_ident`"
           grep $scon_ident conftest.${OBJEXT} 2>&1 1>/dev/null
           scon_status=$?
           AS_IF([test "$scon_output" != "" || test "$scon_status" = "0"],
                 [$6],
                 [$7])],
          [SCON_LOG_MSG([the failed program was:])
           SCON_LOG_FILE([conftest.$3])
           $7])

    unset scon_compiler scon_flags scon_output scon_status
    rm -rf conftest.* conftest${EXEEXT}
])dnl
