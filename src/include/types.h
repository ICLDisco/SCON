/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2014-2015 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_TYPES_H
#define SCON_TYPES_H

#include <src/include/scon_config.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include SCON_EVENT_HEADER
#include SCON_EVENT2_THREAD_HEADER

#if SCON_ENABLE_DEBUG
#include "src/util/output.h"
#endif


/*
 * portable assignment of pointer to int
 */

typedef union {
   uint64_t lval;
   uint32_t ival;
   void*    pval;
   struct {
       uint32_t uval;
       uint32_t lval;
   } sval;
} scon_ptr_t;

/*
 * handle differences in iovec
 */

#if defined(__APPLE__) || defined(__WINDOWS__)
typedef char* scon_iov_base_ptr_t;
#define SCON_IOVBASE char
#else
#define SCON_IOVBASE void
typedef void* scon_iov_base_ptr_t;
#endif

/*
 * handle differences in socklen_t
 */

#if defined(HAVE_SOCKLEN_T)
typedef socklen_t scon_socklen_t;
#else
typedef int scon_socklen_t;
#endif


#define scon_htons htons
#define scon_ntohs ntohs


/*
 * Convert a 64 bit value to network byte order.
 */
static inline uint64_t scon_hton64(uint64_t val) __scon_attribute_const__;
static inline uint64_t scon_hton64(uint64_t val)
{
#ifdef HAVE_UNIX_BYTESWAP
    union { uint64_t ll;
            uint32_t l[2];
    } w, r;

    /* platform already in network byte order? */
    if(htonl(1) == 1L)
        return val;
    w.ll = val;
    r.l[0] = htonl(w.l[1]);
    r.l[1] = htonl(w.l[0]);
    return r.ll;
#else
    return val;
#endif
}

/*
 * Convert a 64 bit value from network to host byte order.
 */

static inline uint64_t scon_ntoh64(uint64_t val) __scon_attribute_const__;
static inline uint64_t scon_ntoh64(uint64_t val)
{
#ifdef HAVE_UNIX_BYTESWAP
    union { uint64_t ll;
            uint32_t l[2];
    } w, r;

    /* platform already in network byte order? */
    if(htonl(1) == 1L)
        return val;
    w.ll = val;
    r.l[0] = ntohl(w.l[1]);
    r.l[1] = ntohl(w.l[0]);
    return r.ll;
#else
    return val;
#endif
}


/**
 * Convert between a local representation of pointer and a 64 bits value.
 */
static inline uint64_t scon_ptr_ptol( void* ptr ) __scon_attribute_const__;
static inline uint64_t scon_ptr_ptol( void* ptr )
{
    return (uint64_t)(uintptr_t) ptr;
}

static inline void* scon_ptr_ltop( uint64_t value ) __scon_attribute_const__;
static inline void* scon_ptr_ltop( uint64_t value )
{
#if SIZEOF_VOID_P == 4 && SCON_ENABLE_DEBUG
    if (value > ((1ULL << 32) - 1ULL)) {
        scon_output(0, "Warning: truncating value in scon_ptr_ltop");
    }
#endif
    return (void*)(uintptr_t) value;
}

#if defined(WORDS_BIGENDIAN) || !defined(HAVE_UNIX_BYTESWAP)
static inline uint16_t scon_swap_bytes2(uint16_t val) __scon_attribute_const__;
static inline uint16_t scon_swap_bytes2(uint16_t val)
{
    union { uint16_t bigval;
            uint8_t  arrayval[2];
    } w, r;

    w.bigval = val;
    r.arrayval[0] = w.arrayval[1];
    r.arrayval[1] = w.arrayval[0];

    return r.bigval;
}

static inline uint32_t scon_swap_bytes4(uint32_t val) __scon_attribute_const__;
static inline uint32_t scon_swap_bytes4(uint32_t val)
{
    union { uint32_t bigval;
            uint8_t  arrayval[4];
    } w, r;

    w.bigval = val;
    r.arrayval[0] = w.arrayval[3];
    r.arrayval[1] = w.arrayval[2];
    r.arrayval[2] = w.arrayval[1];
    r.arrayval[3] = w.arrayval[0];

    return r.bigval;
}

static inline uint64_t scon_swap_bytes8(uint64_t val) __scon_attribute_const__;
static inline uint64_t scon_swap_bytes8(uint64_t val)
{
    union { uint64_t bigval;
            uint8_t  arrayval[8];
    } w, r;

    w.bigval = val;
    r.arrayval[0] = w.arrayval[7];
    r.arrayval[1] = w.arrayval[6];
    r.arrayval[2] = w.arrayval[5];
    r.arrayval[3] = w.arrayval[4];
    r.arrayval[4] = w.arrayval[3];
    r.arrayval[5] = w.arrayval[2];
    r.arrayval[6] = w.arrayval[1];
    r.arrayval[7] = w.arrayval[0];

    return r.bigval;
}

#else
#define scon_swap_bytes2 htons
#define scon_swap_bytes4 htonl
#define scon_swap_bytes8 hton64
#endif /* WORDS_BIGENDIAN || !HAVE_UNIX_BYTESWAP */

#define SCON_EV_TIMEOUT EV_TIMEOUT
#define SCON_EV_READ    EV_READ
#define SCON_EV_WRITE   EV_WRITE
#define SCON_EV_SIGNAL  EV_SIGNAL
/* Persistent event: won't get removed automatically when activated. */
#define SCON_EV_PERSIST EV_PERSIST

#define SCON_EVLOOP_ONCE     EVLOOP_ONCE        /**< Block at most once. */
#define SCON_EVLOOP_NONBLOCK EVLOOP_NONBLOCK    /**< Do not block. */

typedef struct event_base scon_event_base_t;
typedef struct event scon_event_t;

#define scon_event_base_create() event_base_new()

#define scon_event_base_free(b) event_base_free(b)

#define scon_event_base_loopbreak(b) event_base_loopbreak(b)

#define scon_event_base_loopexit(b) event_base_loopexit(b, NULL)

/* thread support APIs */
#define scon_event_use_threads() evthread_use_pthreads()

/* Basic event APIs */
#define scon_event_enable_debug_mode() event_enable_debug_mode()

#define scon_event_set(b, x, fd, fg, cb, arg) event_assign((x), (b), (fd), (fg), (event_callback_fn) (cb), (arg))

#define scon_event_add(ev, tv) event_add((ev), (tv))

#define scon_event_del(ev) event_del((ev))

#define scon_event_active(x, y, z) event_active((x), (y), (z))

#define scon_event_new(b, fd, fg, cb, arg) event_new((b), (fd), (fg), (event_callback_fn) (cb), (arg))

#define scon_event_loop(b, fg) event_base_loop((b), (fg))

#ifdef HAVE_LIBEVENT_GLOBAL_SHUTDOWN
#define scon_libevent_global_shutdown() libevent_global_shutdown()
#endif

#endif /* SCON_TYPES_H */
