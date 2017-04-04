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
 * Topology Framework maintenence interface
 *
 *
 *
 */

#ifndef SCON_TOPOLOGY_BASE_H
#define SCON_TOPOLOGY_BASE_H

#include <scon_config.h>
#include "src/mca/topology/topology.h"
#include "src/mca/base/base.h"

SCON_EXPORT extern scon_mca_base_framework_t scon_topology_base_framework;
SCON_EXPORT extern scon_topology_component_t* scon_topology_base_selected_component;

/* select a component */
int scon_topology_base_select(void);

/* return module of the requested component */
scon_topology_module_t * scon_topology_base_get_module(char *component_name);

void scon_topology_base_convert_topoid_to_procid( scon_proc_t *route,
        unsigned int route_rank,
        scon_proc_t *target);

void scon_topology_base_xcast_routing(scon_list_t *routes,
                                      scon_list_t *children);

#define SCON_TOPO_ID_INVALID INT_MAX;
END_C_DECLS

#endif /* SSCON_TOPOLOGY_BASE_H */
