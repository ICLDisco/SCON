/*
 *
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <scon_config.h>
#include <scon_types.h>
#include <scon_globals.h>
#include "scon_common.h"
#include "src/mca/collectives/base/base.h"
#include "src/mca/collectives/base/static-components.h"

/*
 * Globals
 */
scon_collectives_base_t scon_collectives_base;
int scon_collectives_base_open(scon_mca_base_open_flag_t flags);
int scon_collectives_base_close(void);
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
int scon_collectives_base_open(scon_mca_base_open_flag_t flags)
{
    SCON_CONSTRUCT(&scon_collectives_base.actives, scon_list_t);
    SCON_CONSTRUCT(&scon_collectives_base.ongoing, scon_list_t);
    SCON_CONSTRUCT(&scon_collectives_base.coll_table, scon_hash_table_t);
    scon_hash_table_init(&scon_collectives_base.coll_table, 128);
    /* Open up all available components */
    return scon_mca_base_framework_components_open(&scon_collectives_base_framework, flags);
}

int scon_collectives_base_close(void)
{
    SCON_DESTRUCT(&scon_collectives_base.actives);
    SCON_DESTRUCT(&scon_collectives_base.ongoing);
    SCON_DESTRUCT(&scon_collectives_base.coll_table);
    return scon_mca_base_framework_components_close(&scon_collectives_base_framework, NULL);
}


/* Framework Declaration */
SCON_MCA_BASE_FRAMEWORK_DECLARE(scon, collectives, "Collectives framework",
                                NULL /* register */,
                                scon_collectives_base_open /* open */,
                                scon_collectives_base_close /* close */,
                                mca_collectives_base_static_components,
                                0);

/* object instances */
SCON_CLASS_INSTANCE (scon_xcast_t,
                     scon_list_item_t,
                     NULL, NULL);

SCON_CLASS_INSTANCE (scon_barrier_t,
                     scon_list_item_t,
                     NULL, NULL);

SCON_CLASS_INSTANCE (scon_allgather_t,
                     scon_list_item_t,
                     NULL, NULL);

SCON_CLASS_INSTANCE (scon_coll_req_t,
                     scon_object_t,
                     NULL, NULL);

static void sigcon(scon_collectives_signature_t *s)
{
    s->procs = NULL;
    s->scon_handle = SCON_HANDLE_INVALID;
    s->nprocs = 0;
    s->seq_num = 0;
}
static void sigdes(scon_collectives_signature_t *s)
{
    if (NULL != s->procs) {
        free(s->procs);
    }
}
SCON_CLASS_INSTANCE(scon_collectives_signature_t,
                   scon_object_t,
                   sigcon, sigdes);

static void tcon(scon_collectives_tracker_t *p)
{
    p->sig = NULL;
    SCON_CONSTRUCT(&p->distance_mask_recv, scon_bitmap_t);
    p->nexpected = 0;
    p->nreported = 0;
    p->req = NULL;
    p->buffers = NULL;
}
static void tdes(scon_collectives_tracker_t *p)
{
    if (NULL != p->sig) {
        SCON_RELEASE(p->sig);
    }
    SCON_DESTRUCT(&p->distance_mask_recv);
    free(p->buffers);
}
SCON_CLASS_INSTANCE(scon_collectives_tracker_t,
                   scon_list_item_t,
                   tcon, tdes);
