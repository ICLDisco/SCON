/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"


#include "opal/mca/mca.h"
#include "opal/util/output.h"
#include "opal/mca/base/base.h"
#include "opal/mca/pmix/pmix.h"
#include "opal/mca/pmix/base/base.h"
#include "opal/util/arch.h"
#include "opal/runtime/opal_progress_threads.h"
#include "opal/class/opal_hash_table.h"

#include "orte/mca/rml/base/base.h"
#include "orte/mca/grpcomm/base/base.h"
#include "orte/mca/routed/base/base.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/dfs/base/base.h"
#include "orte/mca/errmgr/base/base.h"
#include "orte/mca/state/base/base.h"
#include "orte/mca/scon/base/base.h"
#include "orte/mca/scon/scon.h"
#include "orte/mca/scon/native/scon_native.h"

/** This is the SCON specific xcast receive request, it is similar to
 * the regular xcast recv but relays the msgs as SCON msgs instead of RML messages
 */
void scon_xcast_recv(scon_status_t status,
                            scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_buffer_t *buf,
                            scon_msg_tag_t tag,
                            void *cbdata)
{
    opal_list_item_t *item;
    orte_namelist_t *nm;
    int ret, cnt;
    opal_buffer_t *relay, *rly;
    opal_list_t coll;
    orte_grpcomm_signature_t *sig;
    scon_proc_t relay_peer;
    scon_msg_tag_t msg_tag;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "%s received xcast with %d bytes",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                        (int)buf->bytes_used);
    /* we need a passthru buffer to send to our children */
    rly = OBJ_NEW(opal_buffer_t);
    opal_dss.copy_payload(rly, buf);

    /* get the signature that we do not need */
    cnt=1;
    if (ORTE_SUCCESS != (ret = opal_dss.unpack(buf, &sig, &cnt, ORTE_SIGNATURE))) {
        ORTE_ERROR_LOG(ret);
        ORTE_FORCED_TERMINATE(ret);
        return;
    }
    /* get the target tag */
    cnt=1;
    if (ORTE_SUCCESS != (ret = opal_dss.unpack(buf, &msg_tag, &cnt, OPAL_UINT32))) {
        ORTE_ERROR_LOG(ret);
        ORTE_FORCED_TERMINATE(ret);
        return;
    }

    /* setup a buffer we can pass to ourselves - this just contains
     * the initial message, minus the headers inserted by xcast itself */
    relay = OBJ_NEW(opal_buffer_t);
    opal_dss.copy_payload(relay, buf);
    /* setup the relay list */
    OBJ_CONSTRUCT(&coll, opal_list_t);
    /* get the list of next recipients from the routed module */
    orte_routed.get_routing_list(&coll);

    /* if list is empty, no relay is required */
    if (opal_list_is_empty(&coll)) {
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                             "%s scon xcast:direct:send_relay - recipient list is empty!",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        OBJ_RELEASE(rly);
        goto CLEANUP;
    }

    /* send the message to each recipient on list, deconstructing it as we go */
    while (NULL != (item = opal_list_remove_first(&coll))) {
        nm = (orte_namelist_t*)item;
        OBJ_RETAIN(rly);
        OBJ_RETAIN(sig);

        scon_native_convert_proc_name_orte_to_scon(&nm->name, &relay_peer);
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                            "%s relaying xcast down the tree to proc %s",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             ORTE_NAME_PRINT(&nm->name));
        if (SCON_SUCCESS != (ret =  orte_scon.send(sig->scon_handle, &relay_peer, rly,
                                    ORTE_RML_TAG_SCON_XCAST, orte_scon_xcast_send_callback, sig,
                                    NULL, 0))) {
            ORTE_ERROR_LOG(ret);
            OBJ_RELEASE(rly);
            OBJ_RELEASE(item);
            OBJ_RELEASE(sig);
            continue;
        }
        OBJ_RELEASE(item);
    }
    OBJ_RELEASE(rly);  // retain accounting

CLEANUP:
    /* cleanup */
    OBJ_DESTRUCT(&coll);

    /* now send the relay buffer to myself for processing */
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        " %s scon_xcast_recv, sending msg to self on tag = %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), msg_tag);
    scon_native_convert_proc_name_orte_to_scon(ORTE_PROC_MY_NAME, &relay_peer);
    if (SCON_SUCCESS != (ret = orte_scon.send(sig->scon_handle, &relay_peer,
                                        relay, msg_tag,
                                        orte_scon_xcast_send_callback,
                                        NULL, NULL, 0))) {
            ORTE_ERROR_LOG(ret);
            OBJ_RELEASE(relay);
    }
    OBJ_RELEASE(sig);
}
/**
 * xcast processing - xcast on a scon is slightly different as we need to
 * scon sends instead of rml sends for relaying the message, so that the xcast
 * send buffer tag matches with a recv posted on the scon. In addition
 * the scon handle, cbfunc and cbdata are also added to the sig structure. */
void orte_scon_native_process_xcast(int fd, short flags, void *cbdata)
{
    orte_scon_send_req_t *req = (orte_scon_send_req_t *) cbdata;
    orte_scon_scon_t *scon = req->post.xcast.scon;
    orte_grpcomm_signature_t *sig;
    orte_scon_member_t * sm;
    int i = 0;
    sig = OBJ_NEW(orte_grpcomm_signature_t);
    if(req->post.xcast.nprocs == 0) {
        sig->sz = opal_list_get_size(&scon->members);
        sig->signature = (orte_process_name_t*)malloc(sig->sz *
                         sizeof(orte_process_name_t));
        memset(sig->signature, 0, sig->sz * sizeof(orte_process_name_t));
        OPAL_LIST_FOREACH(sm, &scon->members, orte_scon_member_t) {
            sig->signature[i].jobid = sm->name.jobid;
            sig->signature[i].vpid = sm->name.vpid;
            ++i;
        }
        sig->scon_handle = scon->handle;
    }
    else {
        sig->sz = req->post.xcast.nprocs;
        sig->signature = (orte_process_name_t*)malloc(sig->sz *
                         sizeof(orte_process_name_t));
        memset(sig->signature, 0, sig->sz * sizeof(orte_process_name_t));
        for (i = 0; i < (int)sig->sz; i++) {
            opal_convert_string_to_jobid(&sig->signature[i].jobid,
                                         req->post.xcast.procs[i].job_name);
            sig->signature[i].vpid = req->post.xcast.procs[i].rank;
            ++i;
        }
    }
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        " %s calling grp xcast for tag %d, sig->sz =%d, sig->scon=%d",
                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), req->post.xcast.tag, (int)sig->sz, sig->scon_handle);
    orte_grpcomm.xcast(sig, req->post.xcast.tag, req->post.xcast.buf);
    OBJ_RELEASE(req);
}

/**
 * scon barrier completion function
 */
static void scon_barrier_release (int status,
                                  opal_buffer_t *buf,
                                  void *cbdata)
{
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "%s got scon_barrier_release",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    orte_scon_send_req_t *req = (orte_scon_send_req_t *) cbdata;
    orte_scon_barrier_t barrier_req = req->post.barrier;
    /* call the callback and release the req */
    if(NULL != barrier_req.cbfunc) {
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                            "%s calling barrier release callback",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
                            barrier_req.cbfunc (status, barrier_req.scon->handle, NULL,0,
                            barrier_req.cbdata);
    }
    else
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                           "%s barrier release callback is null",
                       ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    OBJ_RELEASE(req);
}

/**
 * Process scon_barrier request, the existing grpcomm barrier function works,
 * we just need to relay the completion as the signature are different
 */
void orte_scon_native_process_barrier(int fd, short flags, void *cbdata)
{
    orte_scon_send_req_t *req = (orte_scon_send_req_t *) cbdata;
    orte_scon_scon_t *scon = req->post.barrier.scon;
    orte_grpcomm_signature_t *sig;
    orte_scon_member_t * sm;
    scon_status_t rc;
    int i = 0;
    scon_buffer_t *buf;
    sig = OBJ_NEW(orte_grpcomm_signature_t);
    if(req->post.barrier.nprocs == 0) {
        sig->sz = opal_list_get_size(&scon->members);
        sig->signature = (orte_process_name_t*)malloc(sig->sz *
                         sizeof(orte_process_name_t));
        memset(sig->signature, 0, sig->sz * sizeof(orte_process_name_t));
        OPAL_LIST_FOREACH(sm, &scon->members, orte_scon_member_t) {
            sig->signature[i].jobid = sm->name.jobid;
            sig->signature[i].vpid = sm->name.vpid;
            ++i;
        }
        sig->scon_handle = scon->handle;
       /* sig->cbfunc = req->post.barrier.cbfunc;
        sig->cbdata = cbdata;*/
    }
    else {
        sig->sz = req->post.barrier.nprocs;
        sig->signature = (orte_process_name_t*)malloc(sig->sz *
                         sizeof(orte_process_name_t));
        memset(sig->signature, 0, sig->sz * sizeof(orte_process_name_t));
        for (i = 0; i < (int) sig->sz; i++) {
            opal_convert_string_to_jobid(&sig->signature[i].jobid,
                                         req->post.barrier.procs[i].job_name);
            sig->signature[i].vpid = req->post.barrier.procs[i].rank;
            ++i;
        }
    }
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        " %s calling grp barrier for sig->sz =%d, sig->scon=%d",
                       ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), (int) sig->sz, sig->scon_handle);
    buf = OBJ_NEW(opal_buffer_t);
    if (ORTE_SUCCESS != (rc = orte_grpcomm.allgather(sig, buf, scon_barrier_release, req))) {
        ORTE_ERROR_LOG(rc);
    }

}

