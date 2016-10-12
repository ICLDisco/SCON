/*
 *
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>
#include "src/mca/topology/base/base.h"
#include "src/mca/topology/base/static-components.h"

/*
 * Globals
 */
scon_topology_module_t *scon_topology;
scon_topology_component_t *scon_topology_base_selected_component = NULL;


/*
 * Function for finding and opening either all MCA components,
 * or the one that was specifically requested via a MCA parameter.
 *
 * Note that we really don't need this function -- we could specify a
 * NULL pointer in the framework declare and the base would do this
 * exact same thing.  However, we need to have at least some
 * executable code in this file, or some linkers (cough cough OS X
 * cough cough) may not actually link in this .o file.
 */
int scon_topology_base_open(scon_mca_base_open_flag_t flags)
{
    /* Open up all available components */
    return scon_mca_base_framework_components_open(&scon_topology_base_framework, flags);
}

/* Framework Declaration */
SCON_MCA_BASE_FRAMEWORK_DECLARE(scon, topology, "Topology framework",
                                NULL /* register */,
                                scon_topology_base_open /* open */,
                                NULL /* close */,
                                mca_topology_base_static_components,
                                0);
