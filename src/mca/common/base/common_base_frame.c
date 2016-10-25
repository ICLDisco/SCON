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
#include "src/mca/common/base/base.h"
#include "src/mca/common/common.h"

#include "src/mca/mca.h"
#include "src/util/output.h"
#include "src/mca/base/base.h"

/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */
#include "src/mca/common/base/static-components.h"

/*
 * Global variables
 */
scon_common_base_t scon_common_base = {{{0}}};
scon_common_module_t scon_common = {0};
scon_common_base_component_t *scon_common_base_selected_component = NULL;
static int scon_common_base_register(scon_mca_base_register_flag_t flags)
{
    /* register scon framework level params */
    return SCON_SUCCESS;
}

static int scon_common_base_close(void)
{
    scon_common_base_component_t *component;
    scon_mca_base_component_list_item_t *cli;
    /* shutdown all active transports */
 /*   while (NULL != (cli = (scon_mca_base_component_list_item_t *) scon_list_remove_first
                          (&scon_common_base.actives))) {
        component = (scon_common_base_component_t*)cli->cli_component;
        if (NULL != component->scon_mca_close_component) {
            component->scon_mca_close_component();
        }
        OBJ_RELEASE(cli);
    }*/
    /* destruct our internal lists */
    SCON_DESTRUCT(&scon_common_base.actives);
    SCON_DESTRUCT(&scon_common_base.scons);
    return scon_mca_base_framework_components_close(&scon_common_base_framework, NULL);
}

/**
 * Function for finding and opening either all MCA components,
 * or the one that was specifically requested via a MCA parameter.
 */
static int scon_common_base_open(scon_mca_base_open_flag_t flags)
{
    /* setup globals */
    SCON_CONSTRUCT(&scon_common_base.actives, scon_list_t);
    SCON_CONSTRUCT(&scon_common_base.scons, scon_pointer_array_t);
    if (SCON_SUCCESS != scon_pointer_array_init(&scon_common_base.scons, 0,
                                                 INT_MAX, 1)) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }
   /* Open up all available components */
    return scon_mca_base_framework_components_open(&scon_common_base_framework, flags);
}


SCON_MCA_BASE_FRAMEWORK_DECLARE(scon, common, "common",
                           scon_common_base_register, scon_common_base_open, scon_common_base_close,
                           mca_common_base_static_components, 0);

#if 0
/* xcast send callback function */
void orte_scon_xcast_send_callback(scon_status_t status,
                                   scon_handle_t scon_handle,
                                   scon_proc_t *peer,
                                   scon_buffer_t *buf,
                                   scon_msg_tag_t tag,
                                   void *cbdata)
{
    //orte_grpcomm_signature_t *sig = (orte_grpcomm_signature_t *) cbdata;
   // scon_xcast_cbfunc_t cbfunc;
    if(SCON_SUCCESS == status) {
        opal_output_verbose(1, orte_scon_base_framework.framework_output,
                            "%s scon xcast, send relay to scon root successfull",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    } else {
        /* we need to complete the xcast request now with the incomplete status*/
     /*   if(NULL != sig->cbfunc) {
            cbfunc = (scon_xcast_cbfunc_t) cbfunc;*/
            /* this code needs to address process translation for now we will just
               pass null for procs array */
          /*  cbfunc(status, sig->scon_handle, NULL, 1,
                        buf, tag , sig->cbdata);
        }*/
    }
    OBJ_RELEASE(buf);
    //OBJ_RELEASE(sig);
}
#endif

/***   SCON CLASS INSTANCES   ***/

static void scon_cons (scon_common_scon_t *ptr)
{
    ptr->num_procs = 0;
    ptr->handle = SCON_HANDLE_INVALID;
    ptr->topo = SCON_TOPO_UNDEFINED;
    SCON_CONSTRUCT(&ptr->members, scon_list_t);
    SCON_CONSTRUCT(&ptr->attributes, scon_list_t);
    SCON_CONSTRUCT(&ptr->posted_recvs, scon_list_t);
    SCON_CONSTRUCT(&ptr->queued_msgs, scon_list_t);
    SCON_CONSTRUCT(&ptr->unmatched_msgs, scon_list_t);
}

static void scon_des (scon_common_scon_t *ptr)
{
    SCON_LIST_DESTRUCT(&ptr->members);
    SCON_LIST_DESTRUCT(&ptr->attributes);
    SCON_LIST_DESTRUCT(&ptr->posted_recvs);
    SCON_LIST_DESTRUCT(&ptr->queued_msgs);
    SCON_LIST_DESTRUCT(&ptr->unmatched_msgs);
}

SCON_CLASS_INSTANCE (scon_common_scon_t,
                    scon_list_item_t,
                    scon_cons, scon_des);

SCON_CLASS_INSTANCE (scon_member_t,
                    scon_list_item_t,
                    NULL, NULL);

SCON_CLASS_INSTANCE (scon_create_t,
                    scon_list_item_t,
                    NULL, NULL);

SCON_CLASS_INSTANCE (scon_send_t,
                    scon_list_item_t,
                    NULL, NULL);

SCON_CLASS_INSTANCE (scon_xcast_t,
                    scon_list_item_t,
                    NULL, NULL);

SCON_CLASS_INSTANCE (scon_barrier_t,
                    scon_list_item_t,
                    NULL, NULL);

SCON_CLASS_INSTANCE (scon_teardown_t,
                    scon_list_item_t,
                    NULL, NULL);

static void send_req_cons(scon_send_req_t *ptr)
{
    SCON_CONSTRUCT(&ptr->post.create, scon_create_t);
    SCON_CONSTRUCT(&ptr->post.send,  scon_send_t);
    SCON_CONSTRUCT(&ptr->post.teardown, scon_teardown_t);
    SCON_CONSTRUCT(&ptr->post.xcast, scon_xcast_t);
    SCON_CONSTRUCT(&ptr->post.barrier, scon_barrier_t);
}

SCON_CLASS_INSTANCE(scon_send_req_t,
                   scon_object_t,
                   send_req_cons, NULL);

static void recv_cons(scon_recv_t *ptr)
{
    ptr->iov.iov_base = NULL;
    ptr->iov.iov_len = 0;
}
static void recv_des(scon_recv_t *ptr)
{
    if (NULL != ptr->iov.iov_base) {
        free(ptr->iov.iov_base);
    }
}
SCON_CLASS_INSTANCE(scon_recv_t,
                   scon_list_item_t,
                   recv_cons, recv_des);

SCON_CLASS_INSTANCE(scon_posted_recv_t,
                   scon_list_item_t,
                   NULL, NULL);

static void prq_cons(scon_recv_req_t *ptr)
{
    ptr->cancel = false;
}
static void prq_des(scon_recv_req_t *ptr)
{
    if (NULL != ptr->post) {
        SCON_RELEASE(ptr->post);
    }
}
SCON_CLASS_INSTANCE(scon_recv_req_t,
                   scon_object_t,
                   prq_cons, prq_des);
