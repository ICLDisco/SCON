/*
 * Copyright (c) 2007      Los Alamos National Security, LLC.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_TOPOLOGY_RADIX_H
#define SCON_TOPOLOGY_RADIX_H

#include "scon_config.h"

#include "src/mca/topology/topology.h"

BEGIN_C_DECLS
typedef struct {
    scon_topology_component_t super;
    int radix;
} scon_topology_radixtree_component_t;
SCON_EXPORT extern scon_topology_radixtree_component_t mca_topology_radixtree_component;
SCON_EXPORT extern scon_topology_module_api_t radixtree_module_api;
END_C_DECLS

#endif
