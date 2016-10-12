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
 * Common orte Component interface
 *
 *
 *
 */

#ifndef SCON_COMMON_ORTE_COMMON_ORTE_H
#define SCON_COMMON_ORTE_COMMON_ORTE_H

#include "scon_config.h"
#include "src/mca/common/common.h"
#include "src/mca/common/base/base.h"

BEGIN_C_DECLS
/* common orte module declaration */
typedef struct {
    scon_common_module_t            super;
    scon_list_t              scons;
    /* more members to be added */
} scon_common_orte_module_t;
extern scon_common_orte_module_t scon_common_orte_module;


/* Scon native component apis */

int common_orte_init   (scon_info_t info[],
                         size_t ninfo);
int common_orte_create (scon_proc_t procs[],
                        size_t nprocs,
                        scon_info_t info[],
                        size_t ninfo,
                        scon_create_cbfunc_t cbfunc,
                        void *cbdata);

int common_orte_getinfo ( scon_handle_t scon_handle,
                          scon_info_t info[],
                          size_t *ninfo);

int common_orte_delete (scon_handle_t scon_handle,
                        scon_op_cbfunc_t cbfunc,
                        void *cbdata,
                        scon_info_t info[],
                        size_t ninfo);

int common_orte_finalize (void);

/* function declaration */
/*void common_orte_convert_proc_name_orte_to_scon( orte_process_name_t *orte_proc,
                                                 scon_proc_t *scon_proc);
void orte_common_orte_process_create (int fd, short flags, void *cbdata);
void orte_common_orte_process_send (int fd, short flags, void *cbdata);
void orte_common_orte_process_delete (int fd, short flags, void *cbdata);
void orte_common_orte_process_xcast (int fd, short flags, void *cbdata);
void orte_common_orte_process_barrier (int fd, short flags, void *cbdata);
void scon_xcast_recv(scon_status_t status,
                     scon_handle_t scon_handle,
                     scon_proc_t *peer,
                     scon_buffer_t *buf,
                     scon_msg_tag_t tag,
                     void *cbdata);*/

END_C_DECLS

#endif /* SCON_COMMON_ORTE_COMMON_ORTE_H */
