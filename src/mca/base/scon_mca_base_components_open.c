/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2013 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011-2015 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2014      Hochschule Esslingen.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "src/class/scon_list.h"
#include "src/util/argv.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "scon_common.h"

/*
 * Local functions
 */
static int open_components(scon_mca_base_framework_t *framework);

struct scon_mca_base_dummy_framework_list_item_t {
    scon_list_item_t super;
    scon_mca_base_framework_t framework;
};

/**
 * Function for finding and opening either all MCA components, or the
 * one that was specifically requested via a MCA parameter.
 */
int scon_mca_base_framework_components_open (scon_mca_base_framework_t *framework,
                                        scon_mca_base_open_flag_t flags)
{
    /* Open flags are not used at this time. Suppress compiler warning. */
    if (flags & SCON_MCA_BASE_OPEN_FIND_COMPONENTS) {
        bool open_dso_components = !(flags & SCON_MCA_BASE_OPEN_STATIC_ONLY);
        /* Find and load requested components */
        int ret = scon_mca_base_component_find(NULL, framework, false, open_dso_components);
        if (SCON_SUCCESS != ret) {
            return ret;
        }
    }

    /* Open all registered components */
    return open_components (framework);
}

/*
 * Traverse the entire list of found components (a list of
 * scon_mca_base_component_t instances).  If the requested_component_names
 * array is empty, or the name of each component in the list of found
 * components is in the requested_components_array, try to open it.
 * If it opens, add it to the components_available list.
 */
static int open_components(scon_mca_base_framework_t *framework)
{
    scon_list_t *components = &framework->framework_components;
    uint32_t open_only_flags = SCON_MCA_BASE_METADATA_PARAM_NONE;
    int output_id = framework->framework_output;
    scon_mca_base_component_list_item_t *cli, *next;
    int ret;

    /*
     * Pre-process the list with parameter constraints
     * e.g., If requested to select only CR enabled components
     *       then only make available those components.
     *
     * JJH Note: Currently checkpoint/restart is the only user of this
     *           functionality. If other component constraint options are
     *           added, then this logic can be used for all contraint
     *           options.
     *
     * NTH: Logic moved to scon_mca_base_components_filter.
     */

    /* If scon_mca_base_framework_register_components was called with the MCA_BASE_COMPONENTS_ALL flag
       we need to trim down and close any extra components we do not want open */
    ret = scon_mca_base_components_filter (framework, open_only_flags);
    if (SCON_SUCCESS != ret) {
        return ret;
    }
    scon_output ( 0,
                         "mca: base: components_open: opening %s components",
                         framework->framework_name);

    /* Announce */
    scon_output_verbose (SCON_MCA_BASE_VERBOSE_COMPONENT, output_id,
                         "mca: base: components_open: opening %s components",
                         framework->framework_name);

    /* Traverse the list of components */
    SCON_LIST_FOREACH_SAFE(cli, next, components, scon_mca_base_component_list_item_t) {
        const scon_mca_base_component_t *component = cli->cli_component;
        scon_output (0,
                             "mca: base: components_open: found loaded component %s",
                             component->scon_mca_component_name);
        scon_output_verbose (SCON_MCA_BASE_VERBOSE_COMPONENT, output_id,
                             "mca: base: components_open: found loaded component %s",
                             component->scon_mca_component_name);

        if (NULL != component->scon_mca_open_component) {
            /* Call open if register didn't call it already */
            ret = component->scon_mca_open_component();
            scon_output (0,
                                 "mca: base: components_open: "
                                 "component %s open function returned %d",
                                 component->scon_mca_component_name, ret);
            if (SCON_SUCCESS == ret) {
                scon_output_verbose (SCON_MCA_BASE_VERBOSE_COMPONENT, output_id,
                                     "mca: base: components_open: "
                                     "component %s open function successful",
                                     component->scon_mca_component_name);
            } else {
                if (SCON_ERR_NOT_AVAILABLE != ret) {
                    /* If the component returns SCON_ERR_NOT_AVAILABLE,
                       it's a cue to "silently ignore me" -- it's not a
                       failure, it's just a way for the component to say
                       "nope!".

                       Otherwise, however, display an error.  We may end
                       up displaying this twice, but it may go to separate
                       streams.  So better to be redundant than to not
                       display the error in the stream where it was
                       expected. */

                    if (scon_mca_base_component_show_load_errors) {
                        scon_output_verbose (SCON_MCA_BASE_VERBOSE_ERROR, output_id,
                                             "mca: base: components_open: component %s "
                                             "/ %s open function failed",
                                             component->scon_mca_type_name,
                                             component->scon_mca_component_name);
                    }
                    scon_output_verbose (SCON_MCA_BASE_VERBOSE_COMPONENT, output_id,
                                         "mca: base: components_open: "
                                         "component %s open function failed",
                                         component->scon_mca_component_name);
                }

                scon_mca_base_component_close (component, output_id);

                scon_list_remove_item (components, &cli->super);
                SCON_RELEASE(cli);
            }
        }
    }

    /* All done */

    return SCON_SUCCESS;
}
