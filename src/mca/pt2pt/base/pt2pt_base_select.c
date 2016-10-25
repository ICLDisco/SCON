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

#include "src/mca/pt2pt/base/base.h"


/**
 * Function for selecting all runnable modules from those that are
 * available.
 *
 * Call the init function on all available modules.
 */
int scon_pt2pt_base_select(void)
{
    int exit_status = SCON_SUCCESS;
    scon_pt2pt_base_component_t *best_component = NULL;
    scon_pt2pt_base_module_t *best_module = NULL;

    /*
     * Select the best component
     */
    if (SCON_SUCCESS != scon_mca_base_select("pt2pt",
            scon_pt2pt_base_framework.framework_output,
            &scon_pt2pt_base_framework.framework_components,
            (scon_mca_base_module_t **) &best_module,
            (scon_mca_base_component_t **) &best_component, NULL) ) {
        /* This will only happen if no component was selected */
        exit_status = SCON_ERROR;
        goto cleanup;
    }
    /* Save the winner */
    scon_pt2pt_base_selected_component = best_component;
    scon_pt2pt = *best_module;

cleanup:
    return exit_status;
}
