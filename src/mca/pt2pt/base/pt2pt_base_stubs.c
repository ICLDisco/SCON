/*
 * Copyright (c) 2016     Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "scon_config.h"
#include <scon_common.h>
#include <scon.h>

#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/pt2pt/base/base.h"
#include "src/mca/pt2pt/pt2pt.h"
#include "src/mca/comm/base/base.h"
#include "src/util/name_fns.h"
#include "src/util/error.h"
#include "src/util/scon_pmix.h"

static void pt2pt_base_send_complete (int status,
                                      scon_handle_t scon_handle,
                                      scon_proc_t* peer,
                                      scon_buffer_t* buffer,
                                      scon_msg_tag_t tag,
                                      void* cbdata)
{
    scon_send_req_t *req = (scon_send_req_t*) cbdata;
    req->post.send.status = status;
    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        " %s scon_scon_native_send_complete on scon %d to %s status=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), req->post.send.scon_handle,
                        SCON_PRINT_PROC(peer), status);
    /* complete the req to the caller and release the req */
    if(NULL != req->post.send.cbfunc) {
        req->post.send.cbfunc(status,scon_handle,
                          &req->post.send.dst, buffer, tag,
                          req->post.send.cbdata);
    }
    SCON_RELEASE(req);
}

void pt2pt_base_process_send (int fd, short flags, void *cbdata)
{
    scon_send_req_t *req = (scon_send_req_t*) cbdata;
    scon_comm_scon_t *scon = scon_comm_base_get_scon(req->post.send.scon_handle);
    scon_proc_t peer = req->post.send.dst;
    scon_handle_t peer_handle;
    scon_proc_t hop;
    scon_recv_t *rcv;
    scon_pt2pt_base_peer_t *pr;
    uint64_t ui64;
    scon_value_t *val = NULL;
    char *uri = NULL;
    size_t uri_sz;
    /* if this is a send to ourselves lets handle it in base instead of sending it
    down */
    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s scon_send checking for self send dst = %s tag =%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer),
                        req->post.send.tag);
    if (SCON_EQUAL == scon_util_compare_name_fields(SCON_NS_CMP_ALL, &peer, SCON_PROC_MY_NAME)) {
        /* local delivery */
        scon_output_verbose(1, scon_pt2pt_base_framework.framework_output,
                            "%s scon_send_to_self at tag %d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME), req->post.send.tag);
        /* copy the message for the recv */
        rcv = SCON_NEW(scon_recv_t);
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
        scon_event_set(scon_pt2pt_base.pt2pt_evbase, &rcv->ev, -1,
                       SCON_EV_WRITE,
                       pt2pt_base_process_recv_msg, rcv);
        scon_event_set_priority(&rcv->ev, SCON_MSG_PRI);
        scon_event_active(&rcv->ev, SCON_EV_WRITE, 1);
        scon_output_verbose(1, scon_pt2pt_base_framework.framework_output,
                            "%s scon_send sending to self completing send dst = %s tag =%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&peer),
                            req->post.send.tag);
        pt2pt_base_send_complete(SCON_SUCCESS,
                                 scon->handle,
                                 SCON_PROC_MY_NAME,
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
    peer_handle = scon_base_get_handle(scon, &peer);
    /* before farwording the request to the module lets make sure that we have this
       peer's contact information */
    hop = scon->topology_module->api.get_nexthop(&scon->topology_module->topology,
            &peer);
    // memcpy(&ui64, (char*)&hop, sizeof(uint64_t));
    scon_util_convert_process_name_to_uint64(&ui64, &hop);
    if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_base.peers,
            ui64, (void**)&pr) || NULL == pr) {
        scon_output(0,  "%s pt2pt:base:send unknown peer %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&hop));
        scon_pmix_get(&hop, SCON_PMIX_PROC_URI, NULL, 0, &val);
        if (SCON_SUCCESS ==  scon_pmix_get(&hop, SCON_PMIX_PROC_URI, NULL, 0, &val)) {
            scon_value_unload(val, (void **)&uri,
                              &uri_sz, SCON_STRING);
            if (NULL != uri) {
                scon_pt2pt_base_set_contact_info(uri);
                if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_base.peers,
                        ui64, (void**)&pr) ||
                        NULL == pr) {
                    /* cannot proceed further, fail the req */
                    SCON_ERROR_LOG(SCON_ERR_ADDRESSEE_UNKNOWN);
                    req->post.send.status = SCON_ERR_ADDRESSEE_UNKNOWN;
                    PT2PT_SEND_COMPLETE(&req->post.send);
                    return;
                }
            } else {
                SCON_ERROR_LOG(SCON_ERR_ADDRESSEE_UNKNOWN);
                req->post.send.status = SCON_ERR_ADDRESSEE_UNKNOWN;
                PT2PT_SEND_COMPLETE(&req->post.send);
                return;
            }
        }
    }
    /* the last sanity check we need to do is ensure that this is reachable by the pt2pt component
       associated with this scon */
    if( pr->module != scon->pt2pt_module) {
        SCON_ERROR_LOG(SCON_ERR_ADDRESSEE_UNKNOWN);
        scon_output(0, "pt2pt_base_process_send: %s oops: proc %s is not reachable from the scon's pt2pt module",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), SCON_PRINT_PROC(&hop));
        req->post.send.status = SCON_ERR_ADDRESSEE_UNKNOWN;
        PT2PT_SEND_COMPLETE(&req->post.send);
        return;
    }
    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s scon_scon_native_process_send: "
                        "route to %s is %s on src scon %d dest scon%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer),
                        SCON_PRINT_PROC(&peer),
                        scon->handle,
                        peer_handle);
    scon->pt2pt_module->send(&req->post.send);
}


SCON_EXPORT int pt2pt_base_api_send_nb (scon_handle_t scon_handle,
                         scon_proc_t *peer,
                         scon_buffer_t *buf,
                         scon_msg_tag_t tag,
                         scon_send_cbfunc_t cbfunc,
                         void *cbdata,
                         scon_info_t info[],
                         size_t ninfo)
{
    scon_comm_scon_t *scon;
    scon_send_req_t *req;
    /* get the scon object*/
    if( NULL == (scon = scon_comm_base_get_scon(scon_handle)))
    {
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* create a send req and do the rest of the processing in
          an event */
        req = SCON_NEW(scon_send_req_t);
        req->post.send.scon_handle = scon->handle;
        req->post.send.buf = buf;
        req->post.send.tag = tag;
        req->post.send.dst = *peer;
        req->post.send.cbfunc = cbfunc;
        req->post.send.cbdata = cbdata;
        /* setup the event for rest of the processing  */
        scon_event_set(scon_globals.evbase, &req->ev, -1, SCON_EV_WRITE, pt2pt_base_process_send, req);
        scon_event_set_priority(&req->ev, SCON_MSG_PRI);
        scon_event_active(&req->ev, SCON_EV_WRITE, 1);
        scon_output_verbose(1, scon_pt2pt_base_framework.framework_output,
                             "%s scon_native_send_nb - set event done",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        return SCON_SUCCESS;
    }
}

SCON_EXPORT int pt2pt_base_api_recv_nb (scon_handle_t scon_handle,
                         scon_proc_t *peer,
                         scon_msg_tag_t tag,
                         bool persistent,
                         scon_recv_cbfunc_t cbfunc,
                         void *cbdata,
                         scon_info_t info[],
                         size_t ninfo)
{
    scon_comm_scon_t *scon;
    scon_recv_req_t *req;
    scon_output(0, "scon_native_recv_nb recv posted message on scon %d tag %d to rank %d",
                scon_handle, tag, peer->rank);
    /* get the scon object*/
    if( NULL == (scon = scon_comm_base_get_scon(scon_handle)))
    {
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* now push the request into the event base so we can add
         * the receive to our list of posted recvs */
        req = SCON_NEW(scon_recv_req_t);
        req->post = SCON_NEW(scon_posted_recv_t);
        req->post->scon_handle = scon_handle;
        req->post->peer = *peer;
        req->post->tag = tag;
        req->post->persistent = persistent;
        req->post->cbfunc = cbfunc;
        req->post->cbdata = cbdata;
        scon_event_set(scon_globals.evbase, &req->ev, -1,
                       SCON_EV_WRITE,
                       pt2pt_base_post_recv, req);
        scon_event_set_priority(&req->ev, SCON_MSG_PRI);
        scon_event_active(&req->ev, SCON_EV_WRITE, 1);

    }
    return SCON_SUCCESS;
}

SCON_EXPORT int pt2pt_base_api_recv_cancel (scon_handle_t scon_handle,
                             scon_proc_t *peer,
                             scon_msg_tag_t tag)
{
    return SCON_ERROR;
}

static void send_cons(scon_send_t  *ptr)
{
    ptr->buf = NULL;
    ptr->cbfunc = NULL;
    ptr->cbdata = NULL;
    ptr->info = NULL;
}
SCON_CLASS_INSTANCE (scon_send_t,
                     scon_list_item_t,
                     send_cons, NULL);

static void send_req_cons(scon_send_req_t *ptr)
{
    SCON_CONSTRUCT(&ptr->post.send,  scon_send_t);
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
