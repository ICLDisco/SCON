/**
 * copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 * This header defines  Scalable Overlay Network Interface for Topology
 */

/**
 * @file
 *
 * SCON topology  interface
 *
 * This layer provides the interface for SCON topology
 */
#ifndef SCON_MCA_TOPOLOGY_TOPOLOGY_H
#define SCON_MCA_TOPOLOGY_TOPOLOGY_H

#include "scon.h"
#include "scon_common.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/class/scon_bitmap.h"
#include "src/class/scon_list.h"
BEGIN_C_DECLS

/* struct for tracking routing trees */
typedef struct {
    scon_list_item_t super;
    unsigned int my_id;
    /* bitmap of members that are reachable from here */
    scon_bitmap_t relatives;
    unsigned int myparent_id;
    unsigned int mygrandparent_id;
    unsigned int mylifeline_id;
} scon_topo_t;
SCON_CLASS_DECLARATION(scon_topo_t);

typedef struct {
    scon_topo_t my_topo;
    scon_list_t my_peers;
} scon_topology_t;
SCON_CLASS_DECLARATION(scon_topology_t);

//* ******************************************************************** */
/**
 * Initialize the topology module
 *
 * Do whatever needs to be done to initialize the selected module
 *
 * @retval SCON_SUCCESS Success
 * @retval SCON_ERROR  Error code from whatever was encountered
 */
typedef int (*scon_topology_module_init_fn_t)(scon_topology_t *topo);

/**
 * Finalize the topology module
 *
 * Finalize the topology module, ending cleaning up all resources
 * associated with the module.  After the finalize function is called,
 * all interface functions (and the module structure itself) are not
 * available for use.
 *
 * @note Whether or not the finalize function returns successfully,
 * the module should not be used once this function is called.
 *
 * @retval SCON_SUCCESS Success
 * @retval SCON_ERROR   An unspecified error occurred
 */
typedef int (*scon_topology_module_finalize_fn_t)(scon_topology_t *topo);


/*
 * Delete route
 *
 * Delete the route to the specified proc from the topology. Note
 * that wildcards are supported to remove routes from, for example, all
 * procs in a given job
 */
typedef int (*scon_topology_module_delete_route_fn_t)(scon_topology_t *topo,
                                                      scon_proc_t *proc);

/**
 * Update route table with new information
 *
 * Update routing table with a new entry.  If an existing exact match
 * for the entry exists, it will be replaced with the current
 * information.  If the entry is new, it will be inserted behind all
 * entries of similar "mask".  So a wildcard cellid entry will be
 * inserted after any fully-specified entries and any other wildcard
 * cellid entries, but before any wildcard cellid and jobid entries.
 *
 * @retval SCON_SUCCESS Success
 * @retval SCON_ERR_NOT_SUPPORTED The updated is not supported.  This
 *                      is likely due to using partially-specified
 *                      names with a component that does not support
 *                      such functionality
 * @retval SCON_ERROR   An unspecified error occurred
 */
typedef int (*scon_topology_module_update_route_fn_t)(scon_topology_t *topo,
                                                     scon_proc_t *target,
                                                     scon_proc_t *route);

/**
 * Get the next hop towards the target
 *
 * Obtain the next process on the route to the target.  This function doesn't return the
 * entire path
 * to the target - it only returns the next hop. This could be the target itself,
 * or it could be an intermediate relay.
 */
typedef scon_proc_t  (*scon_topology_module_get_nexthop_fn_t)(scon_topology_t *topo,
                                                             scon_proc_t *target);

/**
 * Initialize the topology for the CON
 *
 * Initialize the routing table for the specified SCON,
 * at the end of the function, the routes to any other process in the
 * specified SCON-must- be defined (even if it is direct)
 */
typedef int (*scon_topology_module_init_routes_fn_t)(scon_topology_t *topo,
                                                     scon_handle_t scon_handle,
                                                     scon_buffer_t *topo_map);

/**
 * Report a route as "lost"
 *
 * Report that an existing connection has been lost, therefore potentially
 * "breaking" a route in the topology. It is critical that broken
 * connections be reported so that the selected topology module has the
 * option of dealing with it. This could consist of nothing more than
 * removing that route from the routing table, or could - in the case
 * of a "lifeline" connection - result in teardown of the SCON
 */
typedef int (*scon_topology_module_route_lost_fn_t)(scon_topology_t *topo,
                                                    const scon_proc_t *route);

/*
 * Is this route defined?
 *
 * Check to see if a route to the specified target has been defined. The
 * function returns "true" if it has, and "false" if no route to the
 * target was previously defined.
 *
 * This is needed because routed modules will return their "wildcard"
 * route if we request a route to a target that they don't know about.
 * In some cases, though, we truly -do- need to know if a route was
 * specifically defined.
 */
typedef bool (*scon_topology_module_route_is_defined_fn_t)(scon_topology_t *topo,
                                                           const scon_proc_t *target);

/*
 * Update the module's routing plan
 *
 *
 */
typedef void (*scon_topology_module_update_topology_fn_t)(scon_topology_t *topo,
                                                          int num_nodes);
/*
 * Get the routing list for an xcast collective
 *
 * Fills the target list with scon_namelist_t so that
 * the collectives framework will know who to send xcast to
 * next
 */
typedef void (*scon_topology_module_get_routing_list_fn_t)(scon_topology_t *topo,
                                                           scon_list_t *coll);

/*
 * Set lifeline process
 *
 * Defines the lifeline to be the specified process. Should contact to
 * that process be lost, the errmgr will be called, possibly resulting
 * in termination of the process and job.
 */
typedef int (*scon_topology_module_set_lifeline_fn_t)(scon_topology_t *topo,
                                                      scon_proc_t *proc);

/*
 * Get the number of routes supported by this process
 *
 * Returns the size of the routing tree using an O(1) function
 */
typedef size_t (*scon_topology_module_num_routes_fn_t)(scon_topology_t *topo);


/* ******************************************************************** */


/**
 * topology module interface
 *
 * Module interface to the topology system.  A global
 * instance of this module, scon_topology, provices an interface into the
 * active topology interface.
 */
struct scon_topology_module_api_1_0_0_t {
    /** Startup/shutdown the  topology system and clean up resources */
    scon_topology_module_init_fn_t                    initialize;
    scon_topology_module_finalize_fn_t                finalize;
    /* API functions */
    scon_topology_module_delete_route_fn_t            delete_route;
    scon_topology_module_update_route_fn_t            update_route;
    scon_topology_module_get_nexthop_fn_t             get_nexthop;
    scon_topology_module_init_routes_fn_t             init_routes;
    scon_topology_module_route_lost_fn_t              route_lost;
    scon_topology_module_route_is_defined_fn_t        route_is_defined;
    scon_topology_module_set_lifeline_fn_t            set_lifeline;
    scon_topology_module_update_topology_fn_t         update_topology;
    scon_topology_module_get_routing_list_fn_t        get_routing_list;
    scon_topology_module_num_routes_fn_t              num_routes;

};
/** Convenience typedef */
typedef struct scon_topology_module_api_1_0_0_t  scon_topology_module_api_1_0_0_t;
typedef struct scon_topology_module_api_1_0_0_t  scon_topology_module_api_t;

struct scon_topology_module_1_0_0_t {
    scon_topology_module_api_t                        api;
    scon_topology_t                                   topology;
};
/** Convenience typedef */
typedef struct scon_topology_module_1_0_0_t  scon_topology_module_1_0_0_t;
typedef struct scon_topology_module_1_0_0_t  scon_topology_module_t;

/**
 * topology component specific functions
 */
typedef scon_topology_module_t* (*scon_mca_topology_get_module_fn_t) (void);

/**
 * topology component definition
 */
struct scon_topology_base_component_1_0_0_t {
    /** MCA base component */
    scon_mca_base_component_t                                  base_version;
    /** MCA base data */
    scon_mca_base_component_data_t                             base_data;
    /** Default priority */
    int priority;
    /** get module */
    scon_mca_topology_get_module_fn_t           get_module;
};
typedef struct scon_topology_base_component_1_0_0_t  scon_topology_component_t;

/**
 * Macro for use in components that are of type common
 */
#define SCON_TOPOLOGY_BASE_VERSION_1_0_0              \
    SCON_MCA_BASE_VERSION_1_0_0("topology", 1, 0, 0)

END_C_DECLS

#endif /* SCON_MCA_TOPOLOGY_TOPOLOGY_H */
