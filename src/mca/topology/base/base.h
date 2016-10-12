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

extern scon_mca_base_framework_t scon_topology_base_framework;
extern scon_topology_component_t* scon_topology_base_selected_component;

/* select a component */
int scon_topology_base_select(void);

END_C_DECLS

#endif /* SSCON_TOPOLOGY_BASE_H */
