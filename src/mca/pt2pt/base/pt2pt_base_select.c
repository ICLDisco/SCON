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

#include "src/util/error.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/pt2pt/base/base.h"


/**
 * Function for selecting all runnable modules from those that are
 * available.
 *
 * Call the init function on all available modules.
 */
SCON_EXPORT int scon_pt2pt_base_select(void)
{
    scon_mca_base_component_list_item_t *cli, *cmp, *c2;
    scon_pt2pt_base_component_t *component, *c3;
    scon_mca_base_module_t *module = NULL;
    bool added;
    int i, rc, priority;

    /*
     * Traverse the list of available components.
     * For each call their 'query' functions to determine relative priority.
     */
    SCON_LIST_FOREACH(cli, &scon_pt2pt_base_framework.framework_components, scon_mca_base_component_list_item_t) {
        component = (scon_pt2pt_base_component_t *) cli->cli_component;
        scon_output (0, "mca:pt2pt:select: looking at component %s",
                     component->base_version.scon_mca_component_name);
        /*
         * If there is a query function then use it.
         */
        if (NULL == component->base_version.scon_mca_query_component) {
            scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                 "mca:pt2pt:select:pt2pt Skipping component [%s]. It does not implement a query function",
                                  component->base_version.scon_mca_component_name );
            continue;
        }

        /*
         * Query this component for the module and priority
         */
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                             "mca:pt2pt:select:pt2pt Querying component [%s]",
                              component->base_version.scon_mca_component_name);

        rc = component->base_version.scon_mca_query_component(&module, &priority);
        if (SCON_ERR_FATAL == rc) {
            /* a fatal error was detected by this component - e.g., the
             * user specified a required element and the component could
             * not find it. In this case, we must not continue as we might
             * find some other component that could run, causing us to do
             * something the user didn't want */
             return rc;
        } else if (SCON_SUCCESS != rc) {
            /* silently skip this component */
            continue;
        }

        /*
         * If no module was returned, then skip component
         */
        if (NULL == module) {
            scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                 "mca:pt2pt:select:pt2pt Skipping component [%s]. Query failed to return a module",
                                  component->base_version.scon_mca_component_name );
            continue;
        }
        /*
         * start the component
         */
        if (SCON_SUCCESS != component->start()) {
           scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                "mca:pt2pt:select: Skipping component [%s] - failed to startup",
                                component->base_version.scon_mca_component_name );
            continue;
        }

        /* record it, but maintain priority order */
        added = false;
        SCON_LIST_FOREACH(cmp, &scon_pt2pt_base.actives, scon_mca_base_component_list_item_t) {
            c3 = (scon_pt2pt_base_component_t *) cmp->cli_component;
            if (c3->priority > component->priority) {
                continue;
            }
            scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                "mca:pt2pt:select: Inserting component");
            c2 = SCON_NEW(scon_mca_base_component_list_item_t);
            c2->cli_component = (scon_mca_base_component_t*)component;
            scon_list_insert_pos(&scon_pt2pt_base.actives,
                                 &cmp->super, &c2->super);
            added = true;
            break;
        }
        if (!added) {
            /* add to end */
            scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                "mca:pt2pt:select: Adding component to end");
            c2 = SCON_NEW(scon_mca_base_component_list_item_t);
            c2->cli_component = (scon_mca_base_component_t*)component;
            scon_list_append(&scon_pt2pt_base.actives, &c2->super);
        }
    }
    scon_output(0, "mca:pt2pt:select:num available pt2pt components %d",
                scon_list_get_size(&scon_pt2pt_base.actives));
    if (0 == scon_list_get_size(&scon_pt2pt_base.actives)) {
        /* no support available means we really cannot run unless
         * we are a singleton */
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "mca:pt2pt:select: Init failed to return any available pt2pt components");
        return SCON_ERR_SILENT;
    }

    /* provide them an index so we can track their usability in a bitmap */
    i=0;
    SCON_LIST_FOREACH(cmp, &scon_pt2pt_base.actives, scon_mca_base_component_list_item_t) {
        c3 = (scon_pt2pt_base_component_t *) cmp->cli_component;
        c3->idx = i++;
    }

    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "mca:pt2pt:select: Found %d active transports",
                        (int)scon_list_get_size(&scon_pt2pt_base.actives));
    return SCON_SUCCESS;
}

scon_pt2pt_module_t * scon_pt2pt_base_get_module(char *comp_name) {
    scon_mca_base_component_list_item_t *cli;
    scon_pt2pt_base_component_t *pt2pt_comp;
    SCON_LIST_FOREACH(cli, &scon_pt2pt_base.actives, scon_mca_base_component_list_item_t) {
        pt2pt_comp = (scon_pt2pt_base_component_t *) cli->cli_component;
        if (0 == strncmp(comp_name, pt2pt_comp->base_version.scon_mca_component_name,
                         SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN)) {
           return pt2pt_comp->get_module();
        }
    }
    return NULL;
}
