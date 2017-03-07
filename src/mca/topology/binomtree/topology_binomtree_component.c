/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2007-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2004-2008 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2016      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>
#include <scon_common.h>

#include "src/mca/mca.h"
#include "src/mca/base/base.h"

#include "topology_binomial.h"

static scon_topology_module_t* binomtree_get_module(void);

/**
 * component definition
 */
SCON_EXPORT scon_topology_component_t scon_topology_binomtree_component = {
    /* First, the mca_base_component_t struct containing meta
       information about the component itself */
    .base_version =
        {
            SCON_TOPOLOGY_BASE_VERSION_1_0_0,
            .scon_mca_component_name = "binomtree",
            /* Component name and version */
            SCON_MCA_BASE_MAKE_VERSION(component, SCON_MAJOR_VERSION,
            SCON_MINOR_VERSION,
            SCON_RELEASE_VERSION),
        },
        .get_module = binomtree_get_module
};

static scon_topology_module_t* binomtree_get_module()
{
    /* make this selected ONLY if the user directs as this module scales
     * poorly compared to our other options
     */
    scon_topology_module_t *binomtree_module = malloc (sizeof(scon_topology_module_t)) ;
    binomtree_module->api = binomtree_module_api;
    SCON_CONSTRUCT(&binomtree_module->topology, scon_topology_t);
    return binomtree_module ;
}
