/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2006 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2013-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include "src/class/scon_list.h"
#include "src/util/output.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/base/scon_mca_base_component_repository.h"
#include "scon_common.h"

void scon_mca_base_component_unload (const scon_mca_base_component_t *component, int output_id)
{
    int ret;

    /* Unload */
    scon_output_verbose (SCON_MCA_BASE_VERBOSE_COMPONENT, output_id,
                         "mca: base: close: unloading component %s",
                         component->scon_mca_component_name);

    ret = scon_mca_base_var_group_find (component->scon_mca_project_name, component->scon_mca_type_name,
                                   component->scon_mca_component_name);
    if (0 <= ret) {
        scon_mca_base_var_group_deregister (ret);
    }

    scon_mca_base_component_repository_release (component);
}

void scon_mca_base_component_close (const scon_mca_base_component_t *component, int output_id)
{
    /* Close */
    if (NULL != component->scon_mca_close_component) {
        component->scon_mca_close_component();
        scon_output_verbose (SCON_MCA_BASE_VERBOSE_COMPONENT, output_id,
                             "mca: base: close: component %s closed",
                             component->scon_mca_component_name);
    }

    scon_mca_base_component_unload (component, output_id);
}

int scon_mca_base_framework_components_close (scon_mca_base_framework_t *framework,
                                              const scon_mca_base_component_t *skip)
{
    return scon_mca_base_components_close (framework->framework_output,
                                           &framework->framework_components,
                                           skip);
}

int scon_mca_base_components_close(int output_id, scon_list_t *components,
                                   const scon_mca_base_component_t *skip)
{
    scon_mca_base_component_list_item_t *cli, *next;

    /* Close and unload all components in the available list, except the
       "skip" item.  This is handy to close out all non-selected
       components.  It's easier to simply remove the entire list and
       then simply re-add the skip entry when done. */

    SCON_LIST_FOREACH_SAFE(cli, next, components, scon_mca_base_component_list_item_t) {
        if (skip == cli->cli_component) {
            continue;
        }

        scon_mca_base_component_close (cli->cli_component, output_id);
        scon_list_remove_item (components, &cli->super);

        SCON_RELEASE(cli);
    }

    /* All done */
    return SCON_SUCCESS;
}
