/*
 * Copyright (C) 2014      Artem Polyakov <artpol84@gmail.com>
 * Copyright (c) 2014-2017 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_UTIL_TIMING_H
#define SCON_UTIL_TIMING_H

#include <src/include/scon_config.h>


#include "src/class/scon_list.h"

#if SCON_ENABLE_TIMING

#define SCON_TIMING_DESCR_MAX 1024
#define SCON_TIMING_BUFSIZE 32
#define SCON_TIMING_OUTBUF_SIZE (10*1024)

typedef enum {
    SCON_TIMING_TRACE,
    SCON_TIMING_INTDESCR,
    SCON_TIMING_INTBEGIN,
    SCON_TIMING_INTEND
} scon_event_type_t;

typedef struct {
    scon_list_item_t super;
    int fib;
    scon_event_type_t type;
    const char *func;
    const char *file;
    int line;
    double ts, ts_ovh;
    char descr[SCON_TIMING_DESCR_MAX];
    int id;
} scon_timing_event_t;

typedef double (*get_ts_t)(void);

typedef struct scon_timing_t
{
    int next_id_cntr;
    // not thread safe!
    // The whole implementation is not thread safe now
    // since it is supposed to be used in service
    // thread only. Fix in the future or now?
    int current_id;
    scon_list_t *events;
    scon_timing_event_t *buffer;
    size_t buffer_offset, buffer_size;
    get_ts_t get_ts;
} scon_timing_t;

typedef struct {
    scon_timing_t *t;
    scon_timing_event_t *ev;
    int errcode;
} scon_timing_prep_t;

/* Pass down our namespace and rank for pretty-print purposes */
void scon_init_id(char* nspace, int rank);

/**
 * Initialize timing structure.
 *
 * @param t pointer to the timing handler structure
 */
void scon_timing_init(scon_timing_t *t);

/**
 * Prepare timing event, do all printf-like processing.
 * Should not be directly used - for service purposes only.
 *
 * @param t pointer to the timing handler structure
 * @param fmt printf-like format
 * @param ... other parameters that should be converted to string representation
 *
 * @retval partly filled scon_timing_prep_t structure
  */
scon_timing_prep_t scon_timing_prep_ev(scon_timing_t *t, const char *fmt, ...);

/**
 * Prepare timing event, ignore printf-like processing.
 * Should not be directly used - for service purposes only.
 *
 * @param t pointer to the timing handler structure
 * @param fmt printf-like format
 * @param ... other parameters that should be converted to string representation
 *
 * @retval partly filled scon_timing_prep_t structure
  */
scon_timing_prep_t scon_timing_prep_ev_end(scon_timing_t *t, const char *fmt, ...);

/**
 * Enqueue timing event into the list of events in handler 't'.
 *
 * @param p result of scon_timing_prep_ev
 * @param func function name where event occurs
 * @param file file name where event occurs
 * @param line line number in the file
 *
 * @retval
 */
void scon_timing_add_step(scon_timing_prep_t p, const char *func,
                          const char *file, int line);

/**
 * Enqueue the description of the interval into a list of events
 * in handler 't'.
 *
 * @param p result of scon_timing_prep_ev
 * @param func function name where event occurs
 * @param file file name where event occurs
 * @param line line number in the file
 *
 * @retval id of event interval
 */
int scon_timing_descr(scon_timing_prep_t p, const char *func,
                      const char *file, int line);

/**
 * Enqueue the beginning of timing interval that already has the
 * description and assigned id into the list of events
 * in handler 't'.
 *
 * @param p result of scon_timing_prep_ev
 * @param func function name where event occurs
 * @param file file name where event occurs
 * @param line line number in the file
 *
 * @retval
 */
void scon_timing_start_id(scon_timing_t *t, int id, const char *func,
                          const char *file, int line);

/**
 * Enqueue the end of timing interval that already has
 * description and assigned id into the list of events
 * in handler 't'.
 *
 * @param p result of scon_timing_prep_ev
 * @param func function name where event occurs
 * @param file file name where event occurs
 * @param line line number in the file
 *
 * @retval
 */
void scon_timing_end(scon_timing_t *t, int id, const char *func,
                     const char *file, int line );

/**
 * Enqueue both description and start of timing interval
 * into the list of events and assign its id.
 *
 * @param p result of scon_timing_prep_ev
 * @param func function name where event occurs
 * @param file file name where event occurs
 * @param line line number in the file
 *
 * @retval interval id
 */
static inline int scon_timing_start_init(scon_timing_prep_t p,
                                         const char *func,
                                         const char *file, int line)
{
    int id = scon_timing_descr(p, func, file, line);
    if( id < 0 )
        return id;
    scon_timing_start_id(p.t, id, func, file, line);
    return id;
}

/**
 * The wrapper that is used to stop last measurement in SCON_TIMING_MNEXT.
 *
 * @param p result of scon_timing_prep_ev
 * @param func function name where event occurs
 * @param file file name where event occurs
 * @param line line number in the file
 *
 * @retval interval id
 */
void scon_timing_end_prep(scon_timing_prep_t p,
                          const char *func, const char *file, int line);

/**
 * Report all events that were enqueued in the timing handler 't'.
 * - if fname == NULL the output will be done using scon_output and
 * each line will be prefixed with "prefix" to ease grep'ing.
 * - otherwise the corresponding file will be used for output in "append" mode
 * WARRNING: not all filesystems provide enough support for that feature, some records may
 * disappear.
 *
 * @param t timing handler
 * @param account_overhead consider malloc overhead introduced by timing code
 * @param prefix prefix to use when no fname was specifyed to ease grep'ing
 * @param fname name of the output file (may be NULL)
 *
 * @retval SCON_SUCCESS On success
 * @retval SCON_ERROR or SCON_ERR_OUT_OF_RESOURCE On failure
 */
scon_status_t scon_timing_report(scon_timing_t *t, char *fname);

/**
 * Report all intervals that were enqueued in the timing handler 't'.
 * - if fname == NULL the output will be done using scon_output and
 * each line will be prefixed with "prefix" to ease grep'ing.
 * - otherwise the corresponding file will be used for output in "append" mode
 * WARRNING: not all filesystems provide enough support for that feature, some records may
 * disappear.
 *
 * @param t timing handler
 * @param account_overhead consider malloc overhead introduced by timing code
  * @param fname name of the output file (may be NULL)
 *
 * @retval SCON_SUCCESS On success
 * @retval SCON_ERROR or SCON_ERR_OUT_OF_RESOURCE On failure
 */
scon_status_t scon_timing_deltas(scon_timing_t *t, char *fname);

/**
 * Release all memory allocated for the timing handler 't'.
 *
 * @param t timing handler
 *
 * @retval
 */
void scon_timing_release(scon_timing_t *t);

/**
 * Macro for passing down process id - compiled out
 * when configured without --enable-timing
 */
#define SCON_TIMING_ID(n, r) scon_timing_id((n), (r));

/**
 * Main macro for use in declaring scon timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 */
#define SCON_TIMING_DECLARE(t) scon_timing_t t;   /* need semicolon here to avoid warnings when not enabled */

/**
 * Main macro for use in declaring external scon timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 */
#define SCON_TIMING_DECLARE_EXT(x, t) x extern scon_timing_t t;  /* need semicolon here to avoid warnings when not enabled */

/**
 * Main macro for use in initializing scon timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_init()
 */
#define SCON_TIMING_INIT(t) scon_timing_init(t)

/**
 * Macro that enqueues event with its description to the specified
 * timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_add_step()
 */
#define SCON_TIMING_EVENT(x) scon_timing_add_step( scon_timing_prep_ev x, __FUNCTION__, __FILE__, __LINE__)

/**
 * MDESCR: Measurement DESCRiption
 * Introduce new timing measurement with string description for the specified
 * timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_descr()
 */
#define SCON_TIMING_MDESCR(x) scon_timing_descr( scon_timing_prep_ev x, __FUNCTION__, __FILE__, __LINE__)

/**
 * MSTART_ID: Measurement START by ID.
 * Marks the beginning of the measurement with ID=id on the
 * specified timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_start_id()
 */
#define SCON_TIMING_MSTART_ID(t, id) scon_timing_start_id(t, id, __FUNCTION__, __FILE__, __LINE__)

/**
 * MSTART: Measurement START
 * Introduce new timing measurement conjuncted with its start
 * on the specifyed timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_start_init()
 */
#define SCON_TIMING_MSTART(x) scon_timing_start_init( scon_timing_prep_ev x, __FUNCTION__, __FILE__, __LINE__)

/**
 * MSTOP: STOP Measurement
 * Finishes the most recent measurement on the specifyed timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_end()
 */
#define SCON_TIMING_MSTOP(t) scon_timing_end(t, -1, __FUNCTION__, __FILE__, __LINE__)

/**
 * MSTOP_ID: STOP Measurement with ID=id.
 * Finishes the measurement with give ID on the specifyed timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_end()
 */
#define SCON_TIMING_MSTOP_ID(t, id) scon_timing_end(t, id, __FUNCTION__, __FILE__, __LINE__)

/**
 * MNEXT: start NEXT Measurement
 * Convinient macro, may be implemented with the sequence of three previously
 * defined macroses:
 * - finish current measurement (SCON_TIMING_MSTOP);
 * - introduce new timing measurement (SCON_TIMING_MDESCR);
 * - starts next measurement (SCON_TIMING_MSTART_ID)
 * on the specifyed timing handler;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_start_init()
 */
#define SCON_TIMING_MNEXT(x) ( \
    scon_timing_end_prep(scon_timing_prep_ev_end x,             \
                            __FUNCTION__, __FILE__, __LINE__ ), \
    scon_timing_start_init( scon_timing_prep_ev x,              \
                            __FUNCTION__, __FILE__, __LINE__)   \
)

/**
 * The macro for use in reporting collected events with absolute values;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @param enable flag that enables/disables reporting. Used for fine-grained timing.
 * @see scon_timing_report()
 */
#define SCON_TIMING_REPORT(enable, t) { \
    if( enable ) { \
        scon_timing_report(t, scon_timing_output); \
    } \
}

/**
 * The macro for use in reporting collected events with relative times;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @param enable flag that enables/disables reporting. Used for fine-grained timing.
 * @see scon_timing_deltas()
 */
#define SCON_TIMING_DELTAS(enable, t) { \
    if( enable ) { \
        scon_timing_deltas(t, scon_timing_output); \
    } \
}

/**
 * Main macro for use in releasing allocated resources;
 * will be "compiled out" when SCON is configured without
 * --enable-timing.
 *
 * @see scon_timing_release()
 */
#define SCON_TIMING_RELEASE(t) scon_timing_release(t)

#else

#define SCON_TIMING_ID(n, r)

#define SCON_TIMING_DECLARE(t)

#define SCON_TIMING_DECLARE_EXT(x, t)

#define SCON_TIMING_INIT(t)

#define SCON_TIMING_EVENT(x)

#define SCON_TIMING_MDESCR(x)

#define SCON_TIMING_MSTART_ID(t, id)

#define SCON_TIMING_MSTART(x)

#define SCON_TIMING_MSTOP(t)

#define SCON_TIMING_MSTOP_ID(t, id)

#define SCON_TIMING_MNEXT(x)

#define SCON_TIMING_REPORT(enable, t)

#define SCON_TIMING_DELTAS(enable, t)

#define SCON_TIMING_RELEASE(t)

#endif

#endif
