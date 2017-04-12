/**
 * copyright (c) 2015-2017 Intel, Inc. All rights reserved.
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
#include "scon_types.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "src/class/scon_bitmap.h"
#include "src/class/scon_list.h"
BEGIN_C_DECLS
/* Define a collective signature so we don't need to
 * track global collective id's */
typedef struct {
    scon_object_t super;
    scon_handle_t scon_handle;
    scon_proc_t *procs;
    size_t nprocs;
    uint32_t seq_num;
} scon_collectives_signature_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_collectives_signature_t);

/* Internal component object for tracking ongoing
 * allgather  operations */
typedef struct {
    scon_list_item_t super;
    /* collective's signature */
    scon_collectives_signature_t *sig;
    /* collection bucket */
    scon_buffer_t bucket;
    /** my index in the participant array */
    unsigned long my_rank;
    /* number of buckets expected */
    size_t nexpected;
    /* number reported in */
    size_t nreported;
    /* distance masks for receive */
    scon_bitmap_t distance_mask_recv;
    /* received buckets */
    scon_buffer_t ** buffers;
    /* all gather or barrier req */
    void *req;
} scon_collectives_tracker_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_collectives_tracker_t);

/* scon xcast req */
typedef struct {
    scon_list_item_t super;
    /*status */
    int status;
    /*  scon handle */
    scon_handle_t scon_handle;
    /* recipients */
    scon_proc_t *procs;
    /* num recipients */
    size_t nprocs;
    /*  msg buffer */
    scon_buffer_t *buf;
    /* msg tag */
    scon_msg_tag_t tag;
    /* user's callback function */
    scon_xcast_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
    /* info struct */
    scon_info_t *info;
    /* number of info */
    size_t ninfo;
} scon_xcast_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_xcast_t);

/* scon barrier req */
typedef struct {
    scon_list_item_t super;
    /*status */
    int status;
    /*  scon object */
    scon_handle_t  scon_handle;
    /* partcipants */
    scon_proc_t *procs;
    /* num participants */
    size_t nprocs;
    /* user's callback function */
    scon_barrier_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
    /* info struct */
    scon_info_t *info;
    /* number of info */
    size_t ninfo;
} scon_barrier_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_barrier_t);

/* scon allgather req */
typedef struct {
    scon_list_item_t super;
    /*status */
    int status;
    /*  scon object */
    scon_handle_t scon_handle;
    /* partcipants */
    scon_proc_t *procs;
    /* num participants */
    size_t nprocs;
    /* my buffer */
    scon_buffer_t *buf;
    /* user's callback function */
    scon_allgather_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
    /* info struct */
    scon_info_t *info;
    /* number of info */
    size_t ninfo;
} scon_allgather_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_allgather_t);

/* collectives request obj */
typedef struct {
    scon_object_t super;
    scon_event_t ev;
    union {
        scon_xcast_t xcast;
        scon_barrier_t barrier;
        scon_allgather_t allgather;
    }post;
} scon_coll_req_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_coll_req_t);

/**
* module init
*/
typedef int (*scon_collectives_base_module_init_fn_t) (scon_handle_t scon_handle);
/**
* smodule finalize
*/
typedef void (*scon_collectives_base_module_finalize_fn_t) (scon_handle_t scon_handle);
/**
 * scon xcast - xcast a msg to all or specified SCON members
 */
typedef int (*scon_collectives_base_module_xcast_fn_t) (scon_xcast_t *xcast);

/**
 * scon barrier - performing a non blocking barrier operation on all or specific
 *                set of scon mebers.
 */
typedef int (*scon_collectives_base_module_barrier_fn_t) (scon_collectives_tracker_t * coll);
/**
 * scon allgather - performing a non blocking allgather operation on all or specific
 *                set of scon mebers.
 */
typedef int (*scon_collectives_base_module_allgather_fn_t) (scon_collectives_tracker_t * coll,
                                                           scon_buffer_t *buf);

/**
 * Collectives module definition
 */
struct scon_collectives_module_1_0_0_t  {
    scon_collectives_base_module_init_fn_t               init;
    scon_collectives_base_module_xcast_fn_t              xcast;
    scon_collectives_base_module_barrier_fn_t            barrier;
    scon_collectives_base_module_allgather_fn_t          allgather;
    scon_collectives_base_module_finalize_fn_t           finalize;
};

typedef struct scon_collectives_module_1_0_0_t scon_collectives_module_1_0_0_t;
typedef struct scon_collectives_module_1_0_0_t scon_collectives_module_t;

/**
 * collectives component specific functions
 */
typedef int (*scon_mca_collectives_base_component_start_fn_t)(void);
typedef void (*scon_mca_collectives_base_component_shutdown_fn_t)(void);

typedef scon_collectives_module_t* (*scon_mca_collectives_get_module_fn_t) (void);
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
    int                                                        priority;
    /** get module */
    scon_mca_collectives_get_module_fn_t                       get_module;
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
