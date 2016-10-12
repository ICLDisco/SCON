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
 * Point to Point messaging pt2pt Interface
 *
 * This layer provides interface for send/recv messages between two members of
 * the scalable overlay network
 */
#ifndef SCON_MCA_PT2PT_PT2PT_H
#define SCON_MCA_PT2PT_PT2PT_H

#include "scon.h"
#include "scon_common.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/class/scon_list.h"

BEGIN_C_DECLS

typedef int (*scon_mca_pt2pt_base_component_start_fn_t)(void);
typedef void (*scon_mca_pt2pt_base_component_shutdown_fn_t)(void);


/**
 * scon send  - non blocking send a msg to a peer scon process
 */
typedef int (*scon_pt2pt_base_module_send_nb_fn_t) ( scon_handle_t scon_handle,
                                               scon_proc_t *peer,
                                               scon_buffer_t *buf,
                                               scon_msg_tag_t tag,
                                               scon_send_cbfunc_t cbfunc,
                                               void *cbdata,
                                               scon_info_t info[],
                                               size_t ninfo);
/**
 * scon recv - non blocking recv of a msg from a peer scon process
 */
typedef int (*scon_pt2pt_base_module_recv_nb_fn_t) (scon_handle_t scon_handle,
                                              scon_proc_t *peer,
                                              scon_msg_tag_t tag,
                                              bool persistent,
                                              scon_recv_cbfunc_t cbfunc,
                                              void *cbdata,
                                              scon_info_t info[],
                                              size_t ninfo);
/**
 * scon recv cancel - cancel a previously posted  non blocking recv
 */
typedef int (*scon_pt2pt_base_module_recv_cancel_fn_t) (scon_handle_t scon_handle,
                                              scon_proc_t *peer,
                                              scon_msg_tag_t tag);

/**
 * pt2pt module definition
 */
struct scon_pt2pt_base_module_1_0_0_t {
    scon_pt2pt_base_module_send_nb_fn_t           send;
    scon_pt2pt_base_module_recv_nb_fn_t           recv;
    scon_pt2pt_base_module_recv_cancel_fn_t       recv_cancel;
} ;

typedef struct scon_pt2pt_base_module_1_0_0_t scon_pt2pt_base_module_1_0_0_t;
typedef struct scon_pt2pt_base_module_1_0_0_t scon_pt2pt_base_module_t;

/**
 * pt2pt Component definition
 */
struct scon_pt2pt_base_component_1_0_0_t {
    /** MCA base component */
    scon_mca_base_component_t                             base_version;
    /** MCA base data */
    scon_mca_base_component_data_t                        base_data;
    scon_mca_pt2pt_base_component_start_fn_t              start;
    scon_mca_pt2pt_base_component_shutdown_fn_t           shutdown;
    /** Default priority */
    int priority;
};
typedef struct scon_pt2pt_base_component_1_0_0_t scon_pt2pt_base_component_1_0_0_t;
typedef struct scon_pt2pt_base_component_1_0_0_t scon_pt2pt_base_component_t;

/**
 * Macro for use in components that are of type Pt2pt
 */
#define SCON_PT2PT_BASE_VERSION_1_0_0              \
    SCON_MCA_BASE_VERSION_1_0_0("pt2pt", 1, 0, 0)

END_C_DECLS

#endif /* SCON_MCA_PT2PT_PT2PT_H */
