/*
 * Copyright (c) 2016-2017    Intel, Inc. All rights reserved.
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
#include "src/mca/collectives/collectives.h"
#include "src/mca/collectives/base/base.h"


/**
 * Function for selecting all runnable modules from those that are
 * available.
 *
 * Call the init function on all available modules.
 */
int scon_collectives_base_select(void)
{
    int exit_status = SCON_SUCCESS;
    scon_collectives_base_component_t *best_component = NULL;
    scon_collectives_module_t *best_module = NULL;

    /*
     * Select the best component
     */
    if (SCON_SUCCESS != scon_mca_base_select("collectives",
            scon_collectives_base_framework.framework_output,
            &scon_collectives_base_framework.framework_components,
            (scon_mca_base_module_t **) &best_module,
            (scon_mca_base_component_t **) &best_component, NULL) ) {
        /* This will only happen if no component was selected */
        exit_status = SCON_ERROR;
        goto cleanup;
    }
    /* Save the winner */
    /*scon_collectives_base_selected_component = best_component;
    scon_collectives = *best_module;*/

cleanup:
    return exit_status;
}

/* return module of the requested component */
scon_collectives_module_t * scon_collectives_base_get_module(char *component_name)
{
    scon_collectives_base_component_t *coll_comp;
    scon_mca_base_component_list_item_t *cli;
    SCON_LIST_FOREACH(cli, &scon_collectives_base_framework.framework_components,
                       scon_mca_base_component_list_item_t) {
        coll_comp = (scon_collectives_base_component_t *) cli->cli_component;
        if (0 == strncmp(component_name, coll_comp->base_version.scon_mca_component_name,
                     SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN)) {
            return coll_comp->get_module();
        }
    }
    return NULL;
}
