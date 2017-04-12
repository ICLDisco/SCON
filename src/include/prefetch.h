/*
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2014-2017  Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file
 *
 * Compiler-specific prefetch functions
 *
 * A small set of prefetch / prediction interfaces for using compiler
 * directives to improve memory prefetching and branch prediction
 */

#ifndef SCON_PREFETCH_H
#define SCON_PREFETCH_H

#if SCON_C_HAVE_BUILTIN_EXPECT
#define SCON_LIKELY(expression) __builtin_expect(!!(expression), 1)
#define SCON_UNLIKELY(expression) __builtin_expect(!!(expression), 0)
#else
#define SCON_LIKELY(expression) (expression)
#define SCON_UNLIKELY(expression) (expression)
#endif

#if SCON_C_HAVE_BUILTIN_PREFETCH
#define SCON_PREFETCH(address,rw,locality) __builtin_prefetch(address,rw,locality)
#else
#define SCON_PREFETCH(address,rw,locality)
#endif

#endif
