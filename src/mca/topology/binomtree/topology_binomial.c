/*
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2007-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2013      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2013-2017 Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "scon_config.h"
#include "scon_common.h"

#include <stddef.h>

#include "src/buffer_ops/buffer_ops.h"
#include "src/class/scon_bitmap.h"
#include "src/util/bit_ops.h"
#include "src/util/output.h"
#include "util/error.h"
#include "src/util/name_fns.h"
#include "src/include/scon_globals.h"

#include "src/mca/topology/base/base.h"
#include "src/mca/topology/topology.h"

static int binom_init(scon_topology_t *topo);
static int binom_finalize(scon_topology_t *topo);
static int binom_delete_route(scon_topology_t *topo,
                        scon_proc_t *proc);
static int binom_update_route(scon_topology_t *topo,
                        scon_proc_t *target,
                        scon_proc_t *route);
static scon_proc_t binom_get_nexthop(scon_topology_t *topo,
                               scon_proc_t *target);
static int binom_init_routes (scon_topology_t *topo,
                        scon_handle_t scon_handle,
                        scon_buffer_t *topo_map);
static int binom_route_lost(scon_topology_t *topo,
                      const scon_proc_t *route);
static bool binom_route_is_defined(scon_topology_t *topo,
                             const scon_proc_t *target);
static void binom_update_topology(scon_topology_t *topo, int num_nodes);
static void binom_get_routing_list(scon_topology_t *topo,
                             scon_list_t *coll);
static int binom_set_lifeline(scon_topology_t *topo,
                        scon_proc_t *proc);
static size_t binom_num_routes(scon_topology_t *topo);

scon_topology_module_api_t binomtree_module_api = {
    .initialize = binom_init,
    .finalize = binom_finalize,
    .delete_route = binom_delete_route,
    .update_route = binom_update_route,
    .get_nexthop = binom_get_nexthop,
    .init_routes = binom_init_routes,
    .route_lost = binom_route_lost,
    .route_is_defined = binom_route_is_defined,
    .set_lifeline = binom_set_lifeline,
    .update_topology = binom_update_topology,
    .get_routing_list = binom_get_routing_list,
    .num_routes = binom_num_routes
};



static int binom_init(scon_topology_t *topo)
{
    topo->my_topo.my_id = SCON_PROC_MY_NAME->rank;
    /* TO  DO remove hardcoding and set lifeline to scon master process's topo id */
    topo->my_topo.mylifeline_id = 0;
    /* setup the list of peers */
    SCON_CONSTRUCT(&topo->my_peers, scon_list_t);
    return SCON_SUCCESS;
}

static int binom_finalize(scon_topology_t *topo)
{
    scon_topo_t *peer;
    scon_list_item_t *item;
    for (item = scon_list_get_first(&topo->my_peers);
            item != scon_list_get_end(&topo->my_peers);
            item = scon_list_get_next(item)) {
        peer = (scon_topo_t*)item;
        SCON_RELEASE(peer);
    }
    SCON_DESTRUCT(&topo->my_peers);
    return SCON_SUCCESS;
}

static int binom_delete_route(scon_topology_t *topo,
                        scon_proc_t *proc)
{
    /** TO DO : need to delete the route and rewire the tree **/

    return SCON_SUCCESS;
}

static int binom_update_route(scon_topology_t *topo,
                        scon_proc_t *target,
                        scon_proc_t *route)
{
   /* TO DO : need to update the logical mapping for of the node to
   * the new process */
    return SCON_SUCCESS;
}


static scon_proc_t binom_get_nexthop(scon_topology_t *topo,
                             scon_proc_t *target)
{
    scon_proc_t *route;
    scon_list_item_t *item;
    scon_topo_t *child;
    unsigned int route_rank = SCON_RANK_UNDEF;
    /* sanity check */
    if ((SCON_RANK_WILDCARD == target->rank) ||
        (SCON_RANK_UNDEF == target->rank)) {
        route = NULL;
        goto found;
    }

    /* if it is me, then the route is just direct */
    if (SCON_EQUAL == scon_util_compare_name_fields(SCON_NS_CMP_ALL, target, SCON_PROC_MY_NAME)) {
        route_rank = target->rank;
        goto found;
    }
    /* if we are trying to reach the lifeline/master process, then route it thru
     the parent */
    if(target->rank == topo->my_topo.mylifeline_id) {
        route_rank = topo->my_topo.myparent_id;
        goto found;
    }
    /* search downward  links to determine the next hop */
    for (item = scon_list_get_first(&topo->my_peers);
            item != scon_list_get_end(&topo->my_peers);
            item = scon_list_get_next(item)) {
        child = (scon_topo_t*)item;
        if (child->my_id == target->rank) {
            /* this is my direct downward link  */
            route_rank = child->my_id;
            goto found;
        }
        /* otherwise, see if the target is rechable through this child */
        if (scon_bitmap_is_set_bit(&child->relatives, target->rank)) {
            /* yep - we need to route through this child */
            route_rank = child->my_id;
            goto found;
        }
    }

    /* if we get here, then the target is not beneath
     * any of our children, so we have to route upwards through our parent
     */
    route_rank = topo->my_topo.myparent_id;

 found:
    route = (scon_proc_t*) malloc(sizeof(scon_proc_t));
    scon_topology_base_convert_topoid_to_procid( route, route_rank, target);
    scon_output_verbose(2, scon_topology_base_framework.framework_output,
                        "binomial_topology:get route - route to %s from %s is %s",
                         SCON_PRINT_PROC(target),
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         SCON_PRINT_PROC(route));

    return *route;
}

static int binom_route_lost(scon_topology_t *topo,
                      const scon_proc_t *route)
{
    scon_list_item_t *item;
    scon_topo_t *child;
    scon_output_verbose(2, scon_topology_base_framework.framework_output,
                         "%s route to %s lost",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         SCON_PRINT_PROC(route));
    /* if this is my child node remove it from the child list */
    for (item = scon_list_get_first(&topo->my_peers);
         item != scon_list_get_end(&topo->my_peers);
         item = scon_list_get_next(item)) {
        child = (scon_topo_t*)item;
        if (child->my_id == route->rank) {
                scon_output_verbose(2, scon_topology_base_framework.framework_output,
                                     "%s topology_binomial: removing route to child  %s",
                                     SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                     SCON_PRINT_PROC(route));
                scon_list_remove_item(&topo->my_peers, item);
                SCON_RELEASE(item);
                return SCON_SUCCESS;
        }
    }
    /* we don't care about this one, so return success */
    return SCON_SUCCESS;
}


static bool binom_route_is_defined(scon_topology_t *topo,
                             const scon_proc_t *target)
{
    /* find out what daemon hosts this proc */
    if (SCON_RANK_UNDEF == target->rank) {
        return false;
    }
    return true;
}

static int binom_set_lifeline(scon_topology_t *topo,
                        scon_proc_t *proc)
{
    /* TBD */
    return SCON_SUCCESS;
}

static int binomial_tree(int rank, int parent, int me, int num_procs,
                         int *nchildren, scon_list_t *childrn,
                         scon_bitmap_t *relatives, bool mine)
{
    int i, bitmap, peer, hibit, mask, found;
    scon_topo_t *child;
    scon_bitmap_t *relations;
    scon_output_verbose(2, scon_topology_base_framework.framework_output,
                         "%s topology:binomial rank %d parent %d me %d num_procs %d",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         rank, parent, me, num_procs);
    /* is this me? */
    if (me == rank) {
        bitmap = scon_cube_dim(num_procs);

        hibit = scon_hibit(rank, bitmap);
        --bitmap;

        for (i = hibit + 1, mask = 1 << i; i <= bitmap; ++i, mask <<= 1) {
            peer = rank | mask;
            if (peer < num_procs) {
                child = SCON_NEW(scon_topo_t);
                child->my_id = peer;
                scon_output_verbose(2, scon_topology_base_framework.framework_output,
                                     "%s topology:binomial %d found child %d",
                                     SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                     rank,
                                     child->my_id);

                if (mine) {
                    /* this is a direct child - add it to my list */
                    scon_list_append(childrn, &child->super);
                    (*nchildren)++;
                    /* setup the relatives bitmap */
                    scon_bitmap_init(&child->relatives, num_procs);

                    /* point to the relatives */
                    relations = &child->relatives;
                } else {
                    /* we are recording someone's relatives - set the bit */
                    scon_bitmap_set_bit(relatives, peer);
                    /* point to this relations */
                    relations = relatives;
                }
                /* search for this child's relatives */
                binomial_tree(0, 0, peer, num_procs, nchildren, childrn, relations, false);
            }
        }
        return parent;
    }

    /* find the children of this rank */
    scon_output_verbose(2, scon_topology_base_framework.framework_output,
                         "%s topology:binomial find children of rank %d",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME), rank);
    bitmap = scon_cube_dim(num_procs);

    hibit = scon_hibit(rank, bitmap);
    --bitmap;

    for (i = hibit + 1, mask = 1 << i; i <= bitmap; ++i, mask <<= 1) {
        peer = rank | mask;
        scon_output_verbose(2, scon_topology_base_framework.framework_output,
                            "%s topology:binomial find children checking peer %d",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME), peer);
        if (peer < num_procs) {
            scon_output_verbose(2, scon_topology_base_framework.framework_output,
                                "%s topology::binomial find children computing tree",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME));
            /* execute compute on this child */
            if (0 <= (found = binomial_tree(peer, rank, me, num_procs, nchildren, childrn, relatives, mine))) {
                scon_output_verbose(2, scon_topology_base_framework.framework_output,
                                    "%s topology::binomial find children returning found value %d",
                                     SCON_PRINT_PROC(SCON_PROC_MY_NAME), found);
                return found;
            }
        }
    }
    return -1;
}

static void binom_update_topology(scon_topology_t *topo, int num_nodes)
{
    scon_topo_t *child;
    int j;
    scon_list_item_t *item;
    int num_children = 0;
    /* clear the list of children if any are already present */
    while (NULL != (item = scon_list_remove_first(&topo->my_peers))) {
        SCON_RELEASE(item);
    }

    /* compute my direct children and the bitmap that shows which ranks
     * lie underneath their branch
     */
    topo->my_topo.myparent_id = binomial_tree(0, 0, SCON_PROC_MY_NAME->rank,
                                   num_nodes,
                                   &num_children, &topo->my_peers, NULL, true);

    if (0 < scon_output_get_verbosity(scon_topology_base_framework.framework_output)) {
        scon_output(0, "%s: parent %d num_children %d", SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PROC_MY_NAME->rank, num_children);
        for (item = scon_list_get_first(&topo->my_peers);
             item != scon_list_get_end(&topo->my_peers);
             item = scon_list_get_next(item)) {
            child = (scon_topo_t*)item;
            scon_output(0, "%s: \tchild %d", SCON_PRINT_PROC(SCON_PROC_MY_NAME), child->my_id);
            for (j=0; j < num_nodes; j++) {
                if (scon_bitmap_is_set_bit(&child->relatives, j)) {
                    scon_output(0, "%s: \t\trelation %d", SCON_PRINT_PROC(SCON_PROC_MY_NAME), j);
                }
            }
        }
    }
}

static void binom_get_routing_list(scon_topology_t *topo,
                             scon_list_t *coll)
{
    scon_topology_base_xcast_routing(&topo->my_peers, coll);
}

static size_t binom_num_routes(scon_topology_t *topo)
{
    scon_output_verbose(2, scon_topology_base_framework.framework_output,
                         "%s num routes %d",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         (int)scon_list_get_size(&topo->my_peers));
    return scon_list_get_size(&topo->my_peers);
}

static int binom_init_routes (scon_topology_t *topo,
                              scon_handle_t scon_handle,
                              scon_buffer_t *topo_map)
{
    return SCON_ERR_NOT_IMPLEMENTED;
}
