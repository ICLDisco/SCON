/*
 * Copyright (c) 2015-2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**
 * @file
 *
 * SCON Native Component interface
 *
 *
 *
 */

#ifndef MCA_SCON_NATIVE_H
#define MCA_SCON_NATIVE_H

#include "orte_config.h"
#include "orte/mca/scon/scon.h"
#include "orte/mca/scon/base/base.h"

BEGIN_C_DECLS
/* scon native module declaration */
typedef struct {
    scon_module_t            super;
    opal_list_t              scons;
    /* more members to be added */
} scon_native_module_t;
extern scon_native_module_t scon_native_module;


/* Scon native component apis */

int scon_native_init   (scon_info_t info[],
                         size_t ninfo);
int scon_native_create (scon_proc_t procs[],
                        size_t nprocs,
                        scon_info_t info[],
                        size_t ninfo,
                        scon_create_cbfunc_t cbfunc,
                        void *cbdata);

int scon_native_getinfo ( scon_handle_t scon_handle,
                          scon_info_t info[],
                          size_t *ninfo);

int scon_native_send_nb ( scon_handle_t scon_handle,
                          scon_proc_t *peer,
                          scon_buffer_t *buf,
                          scon_msg_tag_t tag,
                          scon_send_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo);

int scon_native_recv_nb (scon_handle_t scon_handle,
                         scon_proc_t *peer,
                         scon_msg_tag_t tag,
                         bool persistent,
                         scon_recv_cbfunc_t cbfunc,
                         void *cbdata,
                         scon_info_t info[],
                         size_t ninfo);

int scon_native_recv_cancel (scon_handle_t scon_handle,
                             scon_proc_t *peer,
                             scon_msg_tag_t tag);

int scon_native_xcast (scon_handle_t scon_handle,
                       scon_proc_t procs[],
                       size_t nprocs,
                       scon_buffer_t *buf,
                       scon_msg_tag_t tag,
                       scon_xcast_cbfunc_t cbfunc,
                       void *cbdata,
                       scon_info_t info[],
                       size_t ninfo);

int scon_native_barrier (scon_handle_t scon_handle,
                         scon_proc_t procs[],
                         size_t nprocs,
                         scon_barrier_cbfunc_t cbfunc,
                         void *cbdata,
                         scon_info_t info[],
                         size_t ninfo);

int scon_native_delete (scon_handle_t scon_handle,
                        scon_op_cbfunc_t cbfunc,
                        void *cbdata,
                        scon_info_t info[],
                        size_t ninfo);

int scon_native_finalize (void);

/* function declaration */
void scon_native_convert_proc_name_orte_to_scon( orte_process_name_t *orte_proc,
                                                 scon_proc_t *scon_proc);
void orte_scon_native_process_create (int fd, short flags, void *cbdata);
void orte_scon_native_process_send (int fd, short flags, void *cbdata);
void orte_scon_native_process_delete (int fd, short flags, void *cbdata);
void orte_scon_native_process_xcast (int fd, short flags, void *cbdata);
void orte_scon_native_process_barrier (int fd, short flags, void *cbdata);
void scon_xcast_recv(scon_status_t status,
                     scon_handle_t scon_handle,
                     scon_proc_t *peer,
                     scon_buffer_t *buf,
                     scon_msg_tag_t tag,
                     void *cbdata);

END_C_DECLS

#endif /* MCA_SCON_NATIVE_H */
