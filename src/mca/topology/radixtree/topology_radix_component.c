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

#include "topology_radix.h"

static scon_topology_module_t* radix_get_module(void);
static int topology_radix_component_register(void);
/**
 * component definition
 */
scon_topology_radixtree_component_t mca_topology_radixtree_component = {

    .super =
    {
    /* First, the mca_base_component_t struct containing meta
       information about the component itself */
    .base_version =
        {
            SCON_TOPOLOGY_BASE_VERSION_1_0_0,
            .scon_mca_component_name = "radixtree",
            /* Component name and version */
            SCON_MCA_BASE_MAKE_VERSION(component, SCON_MAJOR_VERSION,
            SCON_MINOR_VERSION,
            SCON_RELEASE_VERSION),
            .scon_mca_register_component_params = topology_radix_component_register,
        },
        .get_module = radix_get_module
    },
    .radix = 64,
};


static int topology_radix_component_register(void)
{
    scon_mca_base_component_t *c = &mca_topology_radixtree_component.super.base_version;

    mca_topology_radixtree_component.radix = 64;
    (void) scon_mca_base_component_var_register(c, "tree radix" ,
                                           "Radix to be used for topology radix tree",
                                           SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                           SCON_INFO_LVL_9,
                                           SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                           &mca_topology_radixtree_component.radix);

    return SCON_SUCCESS;
}

static scon_topology_module_t* radix_get_module()
{
    /* make this selected ONLY if the user directs as this module scales
     * poorly compared to our other options
     */
    scon_topology_module_t *radixtree_module = malloc (sizeof(scon_topology_module_t)) ;
    radixtree_module->api = radixtree_module_api;
    SCON_CONSTRUCT(&radixtree_module->topology, scon_topology_t);
    return radixtree_module ;
}
