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
#include "src/mca/pt2pt/base/base.h"
#include "src/mca/pt2pt/base/static-components.h"
#include "src/mca/pt2pt/pt2pt.h"
/*
 * Globals
 */
scon_pt2pt_base_component_t *scon_pt2pt_base_selected_component = NULL;

scon_pt2pt_base_t scon_pt2pt_base;

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
static int scon_pt2pt_base_open(scon_mca_base_open_flag_t flags)
{

    /* setup globals */
    scon_pt2pt_base.max_uri_length = -1;
    SCON_CONSTRUCT(&scon_pt2pt_base.peers, scon_hash_table_t);
    scon_hash_table_init(&scon_pt2pt_base.peers, 128);
    SCON_CONSTRUCT(&scon_pt2pt_base.actives, scon_list_t);
    /* Open up all available components */
    return scon_mca_base_framework_components_open(&scon_pt2pt_base_framework, flags);
}


static int scon_pt2pt_base_register(scon_mca_base_register_flag_t flags)
{
    (void)scon_mca_base_var_register("scon", "pt2pt", "base", "enable_module_progress_threads",
                                "Whether to independently progress pt2pt messages for each interface",
                                SCON_MCA_BASE_VAR_TYPE_BOOL, NULL, 0, 0,
                                9,
                                SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                &scon_pt2pt_base.use_module_threads);
    return SCON_SUCCESS;
}

static int scon_pt2pt_base_close(void)
{
    scon_pt2pt_base_component_t *component;
    scon_mca_base_component_list_item_t *cli;
    scon_object_t *value;
    uint64_t key;

    /* shutdown all active transports */
    while (NULL != (cli = (scon_mca_base_component_list_item_t *) scon_list_remove_first (&scon_pt2pt_base.actives))) {
        component = (scon_pt2pt_base_component_t*)cli->cli_component;
        if (NULL != component->shutdown) {
            component->shutdown();
        }
        SCON_RELEASE(cli);
    }

    /* destruct our internal lists */
    SCON_DESTRUCT(&scon_pt2pt_base.actives);

    /* release all peers from the hash table */
    SCON_HASH_TABLE_FOREACH(key, uint64, value, &scon_pt2pt_base.peers) {
        if (NULL != value) {
            SCON_RELEASE(value);
        }
    }

    SCON_DESTRUCT(&scon_pt2pt_base.peers);
    return scon_mca_base_framework_components_close(&scon_pt2pt_base_framework, NULL);
}

/* Framework Declaration */
SCON_MCA_BASE_FRAMEWORK_DECLARE(scon, pt2pt, "pt2pt",
                                scon_pt2pt_base_register /* register */,
                                scon_pt2pt_base_open /* open */,
                                scon_pt2pt_base_close /* close */,
                                mca_pt2pt_base_static_components,
                                0);

static void pr_cons(scon_pt2pt_base_peer_t *ptr)
{
    ptr->component = NULL;
    SCON_CONSTRUCT(&ptr->addressable, scon_bitmap_t);
    scon_bitmap_init(&ptr->addressable, 8);
}
static void pr_des(scon_pt2pt_base_peer_t *ptr)
{
    SCON_DESTRUCT(&ptr->addressable);
}
SCON_CLASS_INSTANCE(scon_pt2pt_base_peer_t,
                   scon_object_t,
                   pr_cons, pr_des);
