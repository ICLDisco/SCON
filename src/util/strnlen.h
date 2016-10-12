/*
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file
 *
 * Buffer strnlen function for portability to archaic platforms.
 */

#ifndef SCON_STRNLEN_H
#define SCON_STRNLEN_H

#include <src/include/scon_config.h>

#if defined(HAVE_STRNLEN)
#define SCON_STRNLEN(c, a, b)       \
    (c) = (int)strnlen(a, b)
#else
#define SCON_STRNLEN(c, a, b)           \
    do {                                \
        int _x;                         \
        (c) = 0;                        \
        for (_x=0; _x < (b); _x++) {    \
            if ('\0' == (a)[_x]) {      \
                break;                  \
            }                           \
            ++(c);                      \
        }                               \
    } while(0)
#endif

#endif /* SCON_STRNLEN_H */

