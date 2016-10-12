/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include "scon_common.h"
#include "src/util/output.h"

#include "scon_mca_base_framework.h"
#include "scon_mca_base_var.h"
#include "src/mca/base/base.h"

bool scon_mca_base_framework_is_registered (struct scon_mca_base_framework_t *framework)
{
    return !!(framework->framework_flags & SCON_MCA_BASE_FRAMEWORK_FLAG_REGISTERED);
}

bool scon_mca_base_framework_is_open (struct scon_mca_base_framework_t *framework)
{
    return !!(framework->framework_flags & SCON_MCA_BASE_FRAMEWORK_FLAG_OPEN);
}

static void framework_open_output (struct scon_mca_base_framework_t *framework)
{
    if (0 < framework->framework_verbose) {
        if (-1 == framework->framework_output) {
            framework->framework_output = scon_output_open (NULL);
        }
        scon_output_set_verbosity(framework->framework_output,
                                  framework->framework_verbose);
    } else if (-1 != framework->framework_output) {
        scon_output_close (framework->framework_output);
        framework->framework_output = -1;
    }
}

static void framework_close_output (struct scon_mca_base_framework_t *framework)
{
    if (-1 != framework->framework_output) {
        scon_output_close (framework->framework_output);
        framework->framework_output = -1;
    }
}

int scon_mca_base_framework_register (struct scon_mca_base_framework_t *framework,
                                 scon_mca_base_register_flag_t flags)
{
    char *desc;
    int ret;

    assert (NULL != framework);

    framework->framework_refcnt++;

    if (scon_mca_base_framework_is_registered (framework)) {
        return SCON_SUCCESS;
    }

    SCON_CONSTRUCT(&framework->framework_components, scon_list_t);

    if (framework->framework_flags & SCON_MCA_BASE_FRAMEWORK_FLAG_NO_DSO) {
        flags |= SCON_MCA_BASE_REGISTER_STATIC_ONLY;
    }

    if (!(SCON_MCA_BASE_FRAMEWORK_FLAG_NOREGISTER & framework->framework_flags)) {
        /* register this framework with the MCA variable system */
        ret = scon_mca_base_var_group_register (framework->framework_project,
                                           framework->framework_name,
                                           NULL, framework->framework_description);
        if (0 > ret) {
            return ret;
        }

        asprintf (&desc, "Default selection set of components for the %s framework (<none>"
                  " means use all components that can be found)", framework->framework_name);
        ret = scon_mca_base_var_register (framework->framework_project, framework->framework_name,
                                     NULL, NULL, desc, SCON_MCA_BASE_VAR_TYPE_STRING, NULL, 0,
                                     SCON_MCA_BASE_VAR_FLAG_SETTABLE, SCON_INFO_LVL_2,
                                     SCON_MCA_BASE_VAR_SCOPE_ALL_EQ, &framework->framework_selection);
        free (desc);
        if (0 > ret) {
            return ret;
        }

        /* register a verbosity variable for this framework */
        ret = asprintf (&desc, "Verbosity level for the %s framework (default: 0)",
                        framework->framework_name);
        if (0 > ret) {
            return SCON_ERR_OUT_OF_RESOURCE;
        }

        framework->framework_verbose = SCON_MCA_BASE_VERBOSE_ERROR;
        ret = scon_mca_base_framework_var_register (framework, "verbose", desc,
                                               SCON_MCA_BASE_VAR_TYPE_INT,
                                               &scon_mca_base_var_enum_verbose, 0,
                                               SCON_MCA_BASE_VAR_FLAG_SETTABLE,
                                               SCON_INFO_LVL_8,
                                               SCON_MCA_BASE_VAR_SCOPE_LOCAL,
                                               &framework->framework_verbose);
        free(desc);
        if (0 > ret) {
            return ret;
        }

        /* check the initial verbosity and open the output if necessary. we
           will recheck this on open */
        framework_open_output (framework);

        /* register framework variables */
        if (NULL != framework->framework_register) {
            ret = framework->framework_register (flags);
            if (SCON_SUCCESS != ret) {
                return ret;
            }
        }

        /* register components variables */
        ret = scon_mca_base_framework_components_register (framework, flags);
        if (SCON_SUCCESS != ret) {
            return ret;
        }
    }

    framework->framework_flags |= SCON_MCA_BASE_FRAMEWORK_FLAG_REGISTERED;

    /* framework did not provide a register function */
    return SCON_SUCCESS;
}

int scon_mca_base_framework_open (struct scon_mca_base_framework_t *framework,
                             scon_mca_base_open_flag_t flags) {
    int ret;

    assert (NULL != framework);

    /* register this framework before opening it */
    ret = scon_mca_base_framework_register (framework, SCON_MCA_BASE_REGISTER_DEFAULT);
    if (SCON_SUCCESS != ret) {
        return ret;
    }

    /* check if this framework is already open */
    if (scon_mca_base_framework_is_open (framework)) {
        return SCON_SUCCESS;
    }

    if (SCON_MCA_BASE_FRAMEWORK_FLAG_NOREGISTER & framework->framework_flags) {
        flags |= SCON_MCA_BASE_OPEN_FIND_COMPONENTS;

        if (SCON_MCA_BASE_FRAMEWORK_FLAG_NO_DSO & framework->framework_flags) {
            flags |= SCON_MCA_BASE_OPEN_STATIC_ONLY;
        }
    }

    /* lock all of this frameworks's variables */
    ret = scon_mca_base_var_group_find (framework->framework_project,
                                   framework->framework_name,
                                   NULL);
    scon_mca_base_var_group_set_var_flag (ret, SCON_MCA_BASE_VAR_FLAG_SETTABLE, false);

    /* check the verbosity level and open (or close) the output */
    framework_open_output (framework);

    if (NULL != framework->framework_open) {
        ret = framework->framework_open (flags);
    } else {
        ret = scon_mca_base_framework_components_open (framework, flags);
    }

    if (SCON_SUCCESS != ret) {
        framework->framework_refcnt--;
    } else {
        framework->framework_flags |= SCON_MCA_BASE_FRAMEWORK_FLAG_OPEN;
    }

    return ret;
}

int scon_mca_base_framework_close (struct scon_mca_base_framework_t *framework) {
    bool is_open = scon_mca_base_framework_is_open (framework);
    bool is_registered = scon_mca_base_framework_is_registered (framework);
    int ret, group_id;

    assert (NULL != framework);

    if (!(is_open || is_registered)) {
        return SCON_SUCCESS;
    }

    assert (framework->framework_refcnt);
    if (--framework->framework_refcnt) {
        return SCON_SUCCESS;
    }

    /* find and deregister all component groups and variables */
    group_id = scon_mca_base_var_group_find (framework->framework_project,
                                        framework->framework_name, NULL);
    if (0 <= group_id) {
        (void) scon_mca_base_var_group_deregister (group_id);
    }

    /* close the framework and all of its components */
    if (is_open) {
        if (NULL != framework->framework_close) {
            ret = framework->framework_close ();
        } else {
            ret = scon_mca_base_framework_components_close (framework, NULL);
        }

        if (SCON_SUCCESS != ret) {
            return ret;
        }
    } else {
        scon_list_item_t *item;
        while (NULL != (item = scon_list_remove_first (&framework->framework_components))) {
            scon_mca_base_component_list_item_t *cli;
            cli = (scon_mca_base_component_list_item_t*) item;
            scon_mca_base_component_unload(cli->cli_component,
                                           framework->framework_output);
            SCON_RELEASE(item);
        }
        ret = SCON_SUCCESS;
    }

    framework->framework_flags &= ~(SCON_MCA_BASE_FRAMEWORK_FLAG_REGISTERED | SCON_MCA_BASE_FRAMEWORK_FLAG_OPEN);

    SCON_DESTRUCT(&framework->framework_components);

    framework_close_output (framework);

    return ret;
}
