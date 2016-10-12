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
#include "orte/mca/oob/base/base.h"
#include "orte/mca/scon/base/base.h"
#include "orte/mca/scon/scon.h"
#include "orte/mca/scon/native/scon_native.h"

void orte_scon_native_send_complete (int status,
                                     orte_process_name_t* peer,
                                     struct opal_buffer_t* buffer,
                                     orte_rml_tag_t tag,
                                     void* cbdata);

void orte_scon_native_process_send (int fd, short flags, void *cbdata)
{
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    orte_scon_scon_t *scon = req->post.send.scon;
    orte_process_name_t peer;
    scon_handle_t peer_handle;
    orte_scon_recv_t *rcv;
    /* convert the destination process to orte */
    opal_convert_string_to_jobid(&peer.jobid, req->post.send.dst.job_name);
    peer.vpid = req->post.send.dst.rank;
    /* if this is a send to ourselves lets handle it in SCON instead of sending it
    down */
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "%s scon_send checking for self send dst = %s tag =%d",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                        ORTE_NAME_PRINT(&peer),
                        req->post.send.tag);
    if (OPAL_EQUAL == orte_util_compare_name_fields(ORTE_NS_CMP_ALL, &peer, ORTE_PROC_MY_NAME)) {
        /* local delivery */
        opal_output_verbose(1, orte_scon_base_framework.framework_output,
                            "%s scon_send_to_self at tag %d",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), req->post.send.tag);
        /* copy the message for the recv */
        rcv = OBJ_NEW(orte_scon_recv_t);
        rcv->scon_handle = scon->handle;
        rcv->sender = peer;
        rcv->tag = req->post.send.tag;
        if (0 < req->post.send.buf->bytes_used) {
            rcv->iov.iov_base = (IOVBASE_TYPE*)malloc(req->post.send.buf->bytes_used);
            memcpy(rcv->iov.iov_base, req->post.send.buf->base_ptr, req->post.send.buf->bytes_used);
            rcv->iov.iov_len = req->post.send.buf->bytes_used;
        }
        /* post the message for receipt - then execute the send complete callback
         */
        opal_event_set(orte_event_base, &rcv->ev, -1,
                       OPAL_EV_WRITE,
                       orte_scon_base_process_recv_msg, rcv);
        opal_event_set_priority(&rcv->ev, ORTE_MSG_PRI);
        opal_event_active(&rcv->ev, OPAL_EV_WRITE, 1);
        opal_output_verbose(1, orte_scon_base_framework.framework_output,
                            "%s scon_send sending to self completing send dst = %s tag =%d",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                            ORTE_NAME_PRINT(&peer),
                            req->post.send.tag);
        orte_scon_native_send_complete(SCON_SUCCESS,
                                       ORTE_PROC_MY_NAME,
                                       req->post.send.buf,
                                       req->post.send.tag,
                                       req);
        return;
    }

    /*** TO DO: The scon has to stick in routing here, ie find the route,
     * post and do the routing here, and also stick in the scon ids correctly.
     * This is currently handled in oob, we to do that up here, so OFI also
     * routes the message END TO DO ****/
    /* For now we will just get the scon id of the destination for sending this message */
    peer_handle = orte_scon_base_get_handle(scon, &peer);
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "%s orte_scon_native_process_send: "
                        "route to %s is %s on src scon %d dest scon%d",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                        ORTE_NAME_PRINT(&peer),
                        ORTE_NAME_PRINT(&peer),
                        scon->handle,
                        peer_handle);
    ORTE_RML_SEND_MESSAGE_SCON(peer, req->post.send.tag, req->post.send.buf,
                               orte_scon_native_send_complete, req, peer_handle);
}

/* rml posts the send completion to SCON. The SCON  relays it to the original sender
 * after sticking in the scon info handle for now, may be more in future..
 * We have to do this relay because the signature of scon send completion callback
 * RML send completion callback are different.
 */
void orte_scon_native_send_complete (int status,
                                           orte_process_name_t* peer,
                                           struct opal_buffer_t* buffer,
                                           orte_rml_tag_t tag,
                                           void* cbdata)
{
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    req->post.send.status = status;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        " %s orte_scon_native_send_complete on scon %d to %s status=%d",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), req->post.send.scon->handle,
                        ORTE_NAME_PRINT(peer), status);
    /* complete the req to the caller and release the req */

    req->post.send.cbfunc(status, req->post.send.scon->handle,
                           &req->post.send.dst, buffer, tag,
                           req->post.send.cbdata);
    OBJ_RELEASE(req);
}




