/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <scon_config.h>
//#include "util/constants.h"
#include "src/mca/comm/base/base.h"
#include "src/mca/comm/comm.h"

#include "src/mca/mca.h"
#include "src/util/output.h"
#include "src/mca/base/base.h"

/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */
#include "src/mca/comm/base/static-components.h"

/*
 * Global variables
 */
comm_base_t comm_base = {{{0}}};
scon_comm_module_t scon_comm_module = {0};
scon_comm_base_component_t *scon_comm_base_selected_component = NULL;
static int scon_comm_base_register(scon_mca_base_register_flag_t flags)
{
    /* register scon framework level params */
    return SCON_SUCCESS;
}

static int scon_comm_base_close(void)
{
    /* destruct our internal lists */
    SCON_DESTRUCT(&comm_base.actives);
    SCON_DESTRUCT(&comm_base.scons);
    return scon_mca_base_framework_components_close(&scon_comm_base_framework, NULL);
}

/**
 * Function for finding and opening either all MCA components,
 * or the one that was specifically requested via a MCA parameter.
 */
static int scon_comm_base_open(scon_mca_base_open_flag_t flags)
{
    /* setup globals */
    SCON_CONSTRUCT(&comm_base.actives, scon_list_t);
    SCON_CONSTRUCT(&comm_base.scons, scon_pointer_array_t);
    if (SCON_SUCCESS != scon_pointer_array_init(&comm_base.scons, 1,
                                                 INT_MAX, 1)) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }
   /* Open up all available components */
    return scon_mca_base_framework_components_open(&scon_comm_base_framework, flags);
}

SCON_EXPORT scon_comm_scon_t * scon_comm_base_get_scon (scon_handle_t handle)
{
    scon_comm_scon_t *scon;
    int index = get_index(handle);
    if ((0 <= index) && (SCON_INDEX_UNDEFINED != index)) {
        scon = (scon_comm_scon_t*) scon_pointer_array_get_item (&comm_base.scons,
                                       index);
        return scon;
    }
    else
        return NULL;
}

SCON_EXPORT void scon_comm_base_add_scon(scon_comm_scon_t *scon)
{
    int add_index;
    add_index = scon_pointer_array_add (&comm_base.scons, scon);
    scon->handle = add_index + 1;
}

SCON_EXPORT void scon_comm_base_remove_scon(scon_comm_scon_t *scon)
{
    int remove_index = scon->handle - 1;
    scon_output(0, "removing scon %d", scon->handle);
    scon_pointer_array_set_item (&comm_base.scons, remove_index, NULL);

}


SCON_MCA_BASE_FRAMEWORK_DECLARE(scon, comm, "comm",
                           scon_comm_base_register, scon_comm_base_open, scon_comm_base_close,
                           mca_comm_base_static_components, 0);

/***   SCON CLASS INSTANCES   ***/

static void scon_cons (scon_comm_scon_t *ptr)
{
    ptr->nmembers = 0;
    ptr->handle = SCON_HANDLE_INVALID;
    SCON_CONSTRUCT(&ptr->members, scon_list_t);
    SCON_CONSTRUCT(&ptr->posted_recvs, scon_list_t);
    SCON_CONSTRUCT(&ptr->queued_msgs, scon_list_t);
    SCON_CONSTRUCT(&ptr->unmatched_msgs, scon_list_t);
}

static void scon_des (scon_comm_scon_t *ptr)
{
    SCON_LIST_DESTRUCT(&ptr->members);
    SCON_LIST_DESTRUCT(&ptr->posted_recvs);
    SCON_LIST_DESTRUCT(&ptr->queued_msgs);
    SCON_LIST_DESTRUCT(&ptr->unmatched_msgs);
}

SCON_CLASS_INSTANCE (scon_comm_scon_t,
                    scon_list_item_t,
                    scon_cons, scon_des);

SCON_CLASS_INSTANCE (scon_member_t,
                    scon_list_item_t,
                    NULL, NULL);

SCON_CLASS_INSTANCE (scon_create_t,
                    scon_list_item_t,
                    NULL, NULL);

SCON_CLASS_INSTANCE (scon_teardown_t,
                    scon_list_item_t,
                    NULL, NULL);

static void scon_req_cons(scon_req_t *ptr)
{
    SCON_CONSTRUCT(&ptr->post.create,  scon_create_t);
    SCON_CONSTRUCT(&ptr->post.teardown,  scon_teardown_t);
}

SCON_CLASS_INSTANCE(scon_req_t,
                    scon_object_t,
                    scon_req_cons, NULL);
