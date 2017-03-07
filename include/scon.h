/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015 Intel, Inc. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * scon.h - open hpc scalable overlay network  interface
 * $HEADER$
 */
#ifndef SCON_H
#define SCON_H

#include "scon_common.h"
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
typedef uint32_t scon_msg_tag_t;


/* callback fn for scon_create operation. The status and id of the newly
 * initialized scon is returned. The scon_handle should be used by the caller
 * when calling further operations (send, recv, finalize..) on the scon */
typedef void (*scon_create_cbfunc_t) (scon_status_t status,
                                      scon_handle_t scon_handle,
                                      void *cbdata);
/* scon send callback funtion. */
typedef void (*scon_send_cbfunc_t) (scon_status_t status,
                                    scon_handle_t scon_handle,
                                    scon_proc_t *peer,
                                    scon_buffer_t *buf,
                                    scon_msg_tag_t tag,
                                    void *cbdata);
/* scon recv callback function */
typedef void (*scon_recv_cbfunc_t) (scon_status_t status,
                                    scon_handle_t scon_handle,
                                    scon_proc_t *peer,
                                    scon_buffer_t *buf,
                                    scon_msg_tag_t tag,
                                    void *cbdata);
/* scon operation callback function */
typedef void (*scon_op_cbfunc_t) (scon_status_t status,
                                  void *cbdata);

/* scon xcast callback function */
typedef void (*scon_xcast_cbfunc_t) (scon_status_t status,
                                     scon_handle_t scon_handle,
                                     scon_proc_t procs[],
                                     size_t nprocs,
                                     scon_buffer_t *buf,
                                     scon_msg_tag_t tag,
                                     scon_info_t info[],
                                     size_t ninfo,
                                     void *cbdata);

/* scon barrier callback function */
typedef void (*scon_barrier_cbfunc_t) (scon_status_t status,
                                       scon_handle_t scon_handle,
                                       scon_proc_t procs[],
                                       size_t nprocs,
                                       scon_info_t info[],
                                       size_t ninfo,
                                       void *cbdata);
/* scon barrier callback function */
typedef void (*scon_allgather_cbfunc_t) (scon_status_t status,
                                       scon_handle_t scon_handle,
                                       scon_proc_t procs[],
                                       size_t nprocs,
                                       scon_buffer_t *buf,
                                       scon_info_t info[],
                                       size_t ninfo,
                                       void *cbdata);
/**
 * @func scon_init initializes the scon library.
 * @param info   - array of info keys specifying the requested SCON library capabilities.
 *                 The caller can specify capabilities of all its SCONs here. This is an
 *                 optimization param that lets the caller selectively initialize the
 *                 required capabilities.
 * @param ninfo  - size of info array.
 *
 * @return init status.
 */
scon_status_t scon_init( scon_info_t info[],
                         size_t ninfo);

/**
 * @func scon_create creates a scon as specified by input params
 * @param procs    -  list of participating procs, including the calling proc
 *                    this param can be set to null, if the participating procs are
 *                    described by the proc info struct, in which case scon processes
 *                    the participant list based on the keys.
 * @param nprocs   -  number of participating processes, size of procs[] array can
 *                    be set to 0 when procs = null.
 * @param proc_info - array of proc_info keys describing the participating scon procs.
 *                    refer to scon_proc_xxx keys for more information. This
 *                    param can be set to null if all participating processes are specified
 *                    explicitly in procs[]
 * @param nproc_info- size of proc_info array.
 * @param topo_info - array of topology info (key-value pairs) describing the scon topology. If null
 *                    scon will have the default auto topology configuration
 * @param ninfo     - number of topo keys.
 * @param attr_info - arrary of scon attributes info keys.Users can specify attribute of the scon
 * @param nattr_info- size of attr_info[]
 * @return          - returns handle to the newly created SCON. Note that the SCON creation operation
 *                    is not complete when this fn returns, however a handle is returned so the caller
 *                    can start queuing sends and recvs.
 */
scon_handle_t scon_create(scon_proc_t procs[],
                          size_t nprocs,
                          scon_info_t info[],
                          size_t ninfo,
                          scon_create_cbfunc_t cbfunc,
                          void *cbdata);

scon_status_t scon_get_info (scon_handle_t scon_handle,
                             scon_info_t **info,
                             size_t *ninfo);

scon_status_t scon_send_nb (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_buffer_t *buf,
                            scon_msg_tag_t tag,
                            scon_send_cbfunc_t cbfunc,
                            void *cbdata,
                            scon_info_t info[],
                            size_t ninfo);

scon_status_t scon_recv_nb (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_msg_tag_t tag,
                            bool persistent,
                            scon_recv_cbfunc_t cbfunc,
                            void *cbdata,
                            scon_info_t info[],
                            size_t ninfo);

scon_status_t scon_recv_cancel (scon_handle_t scon_handle,
                                scon_proc_t *peer,
                                scon_msg_tag_t tag);

scon_status_t scon_delete(scon_handle_t scon_handle,
                          scon_op_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo);

scon_status_t scon_xcast (scon_handle_t scon_handle,
                          scon_proc_t procs[],
                          size_t nprocs,
                          scon_buffer_t *buf,
                          scon_msg_tag_t tag,
                          scon_xcast_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo);

scon_status_t scon_barrier(scon_handle_t scon_handle,
                           scon_proc_t procs[],
                           size_t nprocs,
                           scon_barrier_cbfunc_t cbfunc,
                           void *cbdata,
                           scon_info_t info[],
                           size_t ninfo);


scon_status_t scon_allgather(scon_handle_t scon_handle,
                           scon_proc_t procs[],
                           size_t nprocs,
                           scon_buffer_t *buf,
                           scon_allgather_cbfunc_t cbfunc,
                           void *cbdata,
                           scon_info_t info[],
                           size_t ninfo);

scon_status_t scon_finalize(void);


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif

