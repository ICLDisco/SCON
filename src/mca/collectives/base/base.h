/*
 * Copyright (c) 2016     Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**
 * @file
 *
 * Collectives Framework maintenence interface
 *
 *
 *
 */

#ifndef SCON_COLLECTIVES_BASE_H
#define SCON_COLLECTIVES_BASE_H

#include <scon_config.h>
#include <scon_types.h>
#include "src/mca/base/base.h"
#include "src/class/scon_hash_table.h"
#include "src/mca/collectives/collectives.h"
/* select a component */
int scon_collectives_base_select(void);

/*
 * globals that might be needed
 */
typedef struct {
    scon_list_t actives;
    scon_list_t ongoing;
    scon_hash_table_t coll_table;
} scon_collectives_base_t;

/** Collectives framework stub APIs **/
int collectives_base_api_xcast(scon_handle_t scon_handle,
                               scon_proc_t procs[],
                               size_t nprocs,
                               scon_buffer_t *buf,
                               scon_msg_tag_t tag,
                               scon_xcast_cbfunc_t cbfunc,
                               void *cbdata,
                               scon_info_t info[],
                               size_t ninfo);

int collectives_base_api_barrier(scon_handle_t scon_handle,
                                 scon_proc_t procs[],
                                 size_t nprocs,
                                 scon_barrier_cbfunc_t cbfunc,
                                 void *cbdata,
                                 scon_info_t info[],
                                 size_t ninfo);

int collectives_base_api_allgather(scon_handle_t scon_handle,
                                   scon_proc_t procs[],
                                   size_t nprocs,
                                   scon_buffer_t *buf,
                                   scon_allgather_cbfunc_t cbfunc,
                                   void *cbdata,
                                   scon_info_t info[],
                                   size_t ninfo);

/* helper functions */
scon_collectives_tracker_t* scon_collectives_base_get_tracker(scon_collectives_signature_t *sig, bool create);
void scon_collectives_base_mark_distance_recv(scon_collectives_tracker_t *coll, uint32_t distance);
unsigned int scon_collectives_base_check_distance_recv(scon_collectives_tracker_t *coll, uint32_t distance);

void scon_collectives_base_allgather_send_complete_callback (
                                  int status, scon_handle_t scon_handle,
                                  scon_proc_t* peer,
                                  scon_buffer_t* buffer,
                                  scon_msg_tag_t tag,
                                  void* cbdata);
/* extern declarations */
SCON_EXPORT extern scon_collectives_base_t scon_collectives_base;
SCON_EXPORT extern scon_mca_base_framework_t scon_collectives_base_framework;
scon_collectives_module_t * scon_collectives_base_get_module(char *component_name);
END_C_DECLS

#endif /* SCON_COLLECTIVES_BASE_H */
