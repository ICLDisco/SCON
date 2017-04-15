/**
 * Copyright (c) 2015-2017 Intel, Inc. All rights reserved.
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
 * SCON comm  interface
 *
 * This layer provides the interface for the managing a SCON. It defines interfaces
 * for init, create, get info and delete operations on a SCON.
 */
#ifndef SCON_MCA_COMM_H
#define SCON_MCA_COMM_H

#include "scon.h"
#include "scon_common.h"
#include "src/include/scon_config.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/class/scon_list.h"
#include "src/mca/comm/base/base.h"

BEGIN_C_DECLS

#define SCON_INVALID_SCON_ID  0xFFFF



/**
 * scon module initialize function - this function takes the overall
 * overlay network configuration info and initializes the SCON accordingly
 */
typedef int (*scon_comm_base_module_init_fn_t) (scon_info_t info[],
                                           size_t ninfo);

/**
 * scon create function - creates a scon consisting of the specified
 * process with the specified configuration.
 */
typedef int (*scon_comm_base_module_create_fn_t) (scon_proc_t procs[],
                                             size_t nprocs,
                                             scon_info_t info[],
                                             size_t ninfo,
                                             scon_create_cbfunc_t cbfunc,
                                             void *cbdata);
/**
 * scon get info - gets (attributes) information of a specific scon or
 *                all available scons.
 */
typedef int (*scon_comm_base_module_get_info_fn_t) ( scon_handle_t scon_handle,
                                                scon_info_t **info,
                                                size_t *ninfo);

/**
 * scon delete - delete a scon -this is a non blocking operation as the delete
 *              should be a coordinated delete.
 */
typedef int (*scon_comm_base_module_delete_fn_t) (scon_handle_t scon_handle,
                                             scon_op_cbfunc_t cbfunc,
                                             void *cbdata,
                                             scon_info_t info[],
                                             size_t ninfo);
/**
 * scon finalize - finalize scon framework operations
 */
typedef int (*scon_comm_base_module_finalize_fn_t) (void);


/**
 * Common module definition
 */
struct scon_comm_base_module_1_0_0_t  {
    scon_comm_base_module_init_fn_t              init;
    scon_comm_base_module_create_fn_t            create;
    scon_comm_base_module_get_info_fn_t          getinfo;
    scon_comm_base_module_delete_fn_t            del;
    scon_comm_base_module_finalize_fn_t          finalize;
};
typedef struct scon_comm_base_module_1_0_0_t scon_comm_base_module_1_0_0_t;
typedef struct scon_comm_base_module_1_0_0_t scon_comm_module_t;

typedef scon_comm_module_t* (*scon_comm_base_component_get_module_fn_t)(void);

/**
 * Common Component definition
 */
struct scon_comm_base_component_1_0_0_t {
    /** MCA base component */
    scon_mca_base_component_t                             base;
    /** Default priority */
    int                                                   priority;
    /** get module **/
    scon_comm_base_component_get_module_fn_t            get_module;
};
typedef struct scon_comm_base_component_1_0_0_t scon_comm_base_component_1_0_0_t;
typedef struct scon_comm_base_component_1_0_0_t scon_comm_base_component_t;

/*
 * MCA Framework
 */
SCON_EXPORT extern scon_mca_base_framework_t scon_comm_base_framework;
SCON_EXPORT extern scon_comm_base_component_t* scon_comm_base_selected_component;
SCON_EXPORT extern scon_comm_module_t scon_comm_module;

/**
 * Macro for use in components that are of type comm
 */
#define SCON_COMM_BASE_VERSION_1_0_0              \
    SCON_MCA_BASE_VERSION_1_0_0("comm", 1, 0, 0)

END_C_DECLS

#endif /* SCON_MCA_COMM_H */
