/*
 * Copyright (c) 2016     Intel, Inc. All rights reserved.
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

#include "src/mca/topology/base/base.h"


/**
 * Function for selecting all runnable modules from those that are
 * available.
 *
 * Call the init function on all available modules.
 */
int scon_topology_base_select(void)
{
    int exit_status = SCON_SUCCESS;
    scon_topology_component_t *best_component = NULL;
    scon_topology_module_t *best_module = NULL;

    /*
     * Select the best component
     */
    if (SCON_SUCCESS != scon_mca_base_select("topology",
            scon_topology_base_framework.framework_output,
            &scon_topology_base_framework.framework_components,
            (scon_mca_base_module_t **) &best_module,
            (scon_mca_base_component_t **) &best_component, NULL) ) {
        /* This will only happen if no component was selected */
        exit_status = SCON_ERROR;
        goto cleanup;
    }
    /* Save the winner */
    scon_topology_base_selected_component = best_component;
cleanup:
    return exit_status;
}

/* return module of the requested component */
scon_topology_module_t * scon_topology_base_get_module(char *component_name)
{
    scon_topology_component_t *topo_comp;
    scon_mca_base_component_list_item_t *cli;
    SCON_LIST_FOREACH(cli, &scon_topology_base_framework.framework_components,
                       scon_mca_base_component_list_item_t) {
        topo_comp = (scon_topology_component_t *) cli->cli_component;
        scon_output_verbose(5, scon_topology_base_framework.framework_output,
                            "scon_topology_base_get_module returning module for %s",
                            component_name);
        if (0 == strncmp(component_name, topo_comp->base_version.scon_mca_component_name,
                     SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN)) {
            return topo_comp->get_module();
        }
    }
    return NULL;
}
