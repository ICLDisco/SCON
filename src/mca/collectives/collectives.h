/**
 * copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 * This header defines  Scalable Overlay Network Interface for Runtime messaging
 */

/**
 * @file
 *
 * SCON collectives  interface
 *
 * This layer provides the interface for SCON collectives ie group
 * communications
 */
#ifndef SCON_MCA_COLLECTIVES_COLLECTIVES_H
#define SCON_MCA_COLLECTIVES_COLLECTIVES_H

#include "scon.h"
#include "scon_common.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/class/scon_list.h"
BEGIN_C_DECLS

/* Provide a generic callback function to handle book keeping for
 * non blocking collectives on scon
 */
void scon_collectives_xcast_send_callback (scon_status_t status,
                                           scon_handle_t scon_handle,
                                           scon_proc_t *peer,
                                           scon_buffer_t *buf,
                                           scon_msg_tag_t tag,
                                           void *cbdata);

typedef int (*scon_mca_collectives_base_component_start_fn_t)(void);
typedef void (*scon_mca_collectives_base_component_shutdown_fn_t)(void);

/**
 * scon xcast - xcast a msg to all or specified SCON peers
 */
typedef int (*scon_collectives_base_module_xcast_fn_t) (scon_handle_t scon_handle,
                                            scon_proc_t procs[],
                                            size_t nprocs,
                                            scon_buffer_t *buf,
                                            scon_msg_tag_t tag,
                                            scon_xcast_cbfunc_t cbfunc,
                                            void *cbdata,
                                            scon_info_t info[],
                                            size_t ninfo);

/**
 * scon barrier - performing a non blocking barrier operation on all or specific
 *                scon peers.
 */
typedef int (*scon_collectives_base_module_barrier_fn_t) (scon_handle_t scon_handle,
                                              scon_proc_t procs[],
                                              size_t nprocs,
                                              scon_barrier_cbfunc_t cbfunc,
                                              void *cbdata,
                                              scon_info_t info[],
                                              size_t ninfo);




/**
 * Collectives module definition
 */
struct scon_collectives_base_module_1_0_0_t  {
    scon_collectives_base_module_xcast_fn_t              xcast;
    scon_collectives_base_module_barrier_fn_t            barrier;
};
typedef struct scon_collectives_base_module_1_0_0_t scon_collectives_base_module_1_0_0_t;
typedef struct scon_collectives_base_module_1_0_0_t scon_collectives_base_module_t;

/**
 * collectives Component definition
 */
struct scon_collectives_base_component_1_0_0_t {
    /** MCA base component */
    scon_mca_base_component_t                                  base_version;
    /** MCA base data */
    scon_mca_base_component_data_t                             base_data;
    scon_mca_collectives_base_component_start_fn_t             start;
    scon_mca_collectives_base_component_shutdown_fn_t          shutdown;
    /** Default priority */
    int priority;
};
typedef struct scon_collectives_base_component_1_0_0_t  scon_collectives_base_component_1_0_0_t;
typedef struct scon_collectives_base_component_1_0_0_t  scon_collectives_base_component_t;

/**
 * Macro for use in components that are of type common
 */
#define SCON_COLLECTIVES_BASE_VERSION_1_0_0              \
    SCON_MCA_BASE_VERSION_1_0_0("collectives", 1, 0, 0)

END_C_DECLS

#endif /* SCON_MCA_COLLECTIVES_COLLECTIVES_H */
