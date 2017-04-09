/*
* Copyright (c) 2004-2011 The University of Tennessee and The University
*                         of Tennessee Research Foundation.  All rights
*                         reserved.
* Copyright (c) 2007-2012 Los Alamos National Security, LLC.  All rights
*                         reserved.
* Copyright (c) 2013      Cisco Systems, Inc.  All rights reserved.
* Copyright (c) 2013-2016 Intel, Inc.  All rights reserved.
* $COPYRIGHT$
*
* Additional copyrights may follow
*
* $HEADER$
*/

#include "scon_config.h"
#include "scon_common.h"

#include <stddef.h>

#include "src/buffer_ops/buffer_ops.h"
#include "src/buffer_ops/types.h"
#include "src/buffer_ops/internal.h"
#include "src/class/scon_bitmap.h"
#include "src/util/bit_ops.h"
#include "src/util/output.h"
#include "util/error.h"
#include "src/util/name_fns.h"
#include "src/include/scon_globals.h"

#include "src/mca/pt2pt/base/base.h"
#include "src/mca/comm/base/base.h"
#include "src/mca/collectives/base/base.h"
#include "src/mca/collectives/collectives.h"
#include "collectives_default.h"

/* Static API's */
static int init(scon_handle_t scon_handle);
static void finalize(scon_handle_t scon_handle);
static int xcast(scon_xcast_t *xcast);
static int allgather(scon_collectives_tracker_t *coll,
                     scon_buffer_t *buf);
static int barrier(scon_collectives_tracker_t *coll);

/* Module def */
scon_collectives_module_t scon_collectives_default_module = {
    init,
    xcast,
    barrier,
    allgather,
    finalize
};

/* internal functions */
static void xcast_recv(scon_status_t status,
                       scon_handle_t scon_handle,
                       scon_proc_t *peer,
                       scon_buffer_t *buf,
                       scon_msg_tag_t tag,
                       void *cbdata);
static void allgather_recv(scon_status_t status,
                           scon_handle_t scon_handle,
                           scon_proc_t *peer,
                           scon_buffer_t *buf,
                           scon_msg_tag_t tag,
                           void *cbdata);
static void barrier_release(scon_status_t status,
                            scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_buffer_t *buf,
                            scon_msg_tag_t tag,
                            void *cbdata);

/**
 * Initialize the module
 */
static int init(scon_handle_t scon_handle)
{
    /* post the receives */
    pt2pt_base_api_recv_nb(scon_handle,
                           SCON_PROC_WILDCARD,
                           SCON_MSG_TAG_XCAST,
                           SCON_MSG_PERSISTENT,
                           xcast_recv, NULL,
                           NULL, 0);
    pt2pt_base_api_recv_nb(scon_handle,
                           SCON_PROC_WILDCARD,
                           SCON_MSG_TAG_ALLGATHER_DIRECT,
                           SCON_MSG_PERSISTENT,
                           allgather_recv, NULL,
                           NULL, 0);
 /*   pt2pt_base_api_recv_nb(scon_handle,
                           SCON_PROC_WILDCARD,
                           SCON_MSG_TAG_BARRIER_DIRECT,
                           SCON_MSG_PERSISTENT,
                           xcast_recv, NULL,
                           NULL, 0);*/
    /* setup recv for  collective (allgather/barrier) release */
    pt2pt_base_api_recv_nb(scon_handle,
                           SCON_PROC_WILDCARD,
                           SCON_MSG_TAG_COLL_RELEASE,
                           SCON_MSG_PERSISTENT,
                           barrier_release, NULL,
                           NULL, 0);

    return SCON_SUCCESS;
}

/**
 * Finalize the module
 */
static void finalize(scon_handle_t scon_handle)
{
    /* cancel the recvs */
    pt2pt_base_api_recv_cancel(scon_handle, SCON_PROC_WILDCARD, SCON_MSG_TAG_XCAST);
    pt2pt_base_api_recv_cancel(scon_handle, SCON_PROC_WILDCARD, SCON_MSG_TAG_ALLGATHER_DIRECT);
    pt2pt_base_api_recv_cancel(scon_handle, SCON_PROC_WILDCARD, SCON_MSG_TAG_BARRIER_DIRECT);
    pt2pt_base_api_recv_cancel(scon_handle, SCON_PROC_WILDCARD, SCON_MSG_TAG_COLL_RELEASE);
    return;
}

static void xcast_send_complete_callback (int status,
        scon_handle_t scon_handle,
        scon_proc_t* peer,
        scon_buffer_t* buffer,
        scon_msg_tag_t tag,
        void* cbdata)
{
    scon_xcast_t *xcast = (scon_xcast_t*) cbdata;
    /* record status */
    xcast->status = status;
    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "send status of xcast msg from %s"
                        "sent to proc %s for tag %d, nprocs =%d, on scon=%d is %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(peer),
                        xcast->tag, (int)xcast->nprocs,
                        xcast->scon_handle,
                        status);
    if (SCON_SUCCESS != status) {
        scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                            "xcast from %s forward to proc %s failed with error=%d"
                            "for tag %d, nprocs =%d, on scon=%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(peer),
                            status,
                            xcast->tag,
                            (int)xcast->nprocs, xcast->scon_handle);
        /* error occured processing xcast, release the buffer
           and call user's callback */
        //SCON_RELEASE(buffer);
        free(buffer);
        if (NULL != xcast->cbfunc) {
            xcast->cbfunc(xcast->status, xcast->scon_handle, xcast->procs, xcast->nprocs,
                          xcast->buf, xcast->tag, NULL, 0, xcast->cbdata);
        }
        SCON_RELEASE(xcast);
    }
}

static void xcast_relay_send_complete_callback (int status,
        scon_handle_t scon_handle,
        scon_proc_t* peer,
        scon_buffer_t* buffer,
        scon_msg_tag_t tag,
        void* cbdata)
{
}


static int xcast(scon_xcast_t *xcast)
{
    int rc;
    scon_buffer_t *xcast_buf;
    scon_collectives_signature_t *sig;
    scon_comm_scon_t *scon;
    sig = SCON_NEW(scon_collectives_signature_t);
    sig->scon_handle = xcast->scon_handle;
    sig->nprocs = xcast->nprocs;
    sig->procs = xcast->procs;
    sig->seq_num = 0;
    //xcast_buf = SCON_NEW(scon_buffer_t);
    xcast_buf = (scon_buffer_t*) malloc(sizeof(scon_buffer_t));
    scon_buffer_construct(xcast_buf);
    scon = scon_comm_base_get_scon(xcast->scon_handle);
    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "xcast from %s forwarding to master %s for tag %d, nprocs =%d, on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(SCON_GET_MASTER(scon)),
                        xcast->tag, (int)xcast->nprocs, xcast->scon_handle);
    /* pack the msg buffer with xcast header */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(xcast_buf, &sig, 1, SCON_COLLECTIVES_SIGNATURE))) {
        SCON_ERROR_LOG(rc);
        goto CLEANUP;
    }
    /* pass the actual message tag */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(xcast_buf, &xcast->tag, 1, SCON_UINT32))) {
        SCON_ERROR_LOG(rc);
        goto CLEANUP;
    }

    /* copy the payload into the new buffer - this is non-destructive, so our
     * caller is still responsible for releasing any memory in the buffer they
     * gave to us
     */
    if (SCON_SUCCESS != (rc = scon_bfrop.copy_payload(xcast_buf, xcast->buf))) {
        SCON_ERROR_LOG(rc);
        goto CLEANUP;
    }
    /* send it to the master process (could be myself) for relay */
    //SCON_RETAIN(xcast_buf);  // we'll let the pt2pt release it
    if ( SCON_SUCCESS != (rc = pt2pt_base_api_send_nb(xcast->scon_handle,
                               SCON_GET_MASTER(scon), xcast_buf,
                               SCON_MSG_TAG_XCAST,
                               xcast_send_complete_callback, xcast,
                               NULL, 0))) {
        SCON_ERROR_LOG(rc);
        goto CLEANUP;
    }
    return SCON_SUCCESS;
CLEANUP:
    scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                        "xcast from %s forward to master %s failed with error=%d"
                        "for tag %d, nprocs =%d, on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(SCON_GET_MASTER(scon)),
                        rc,
                        xcast->tag,
                        (int)xcast->nprocs, xcast->scon_handle);
    /* error occured processing xcast, release the buffer
       and call user's callback */
   // SCON_RELEASE(xcast_buf);
    free(xcast_buf);
    xcast->status = rc;
    if (NULL != xcast->cbfunc) {
        xcast->cbfunc(xcast->status, xcast->scon_handle, xcast->procs, xcast->nprocs,
                      xcast->buf, xcast->tag, NULL, 0, xcast->cbdata);
    }
    SCON_RELEASE(xcast);
    return rc;
}

static int allgather(scon_collectives_tracker_t *coll,
                     scon_buffer_t *buf)
{
    int rc;
    scon_buffer_t *relay;
    scon_allgather_t *allgather;
    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "%s allgather  forwarding to ourserlves nprocs =%d, on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)coll->sig->nprocs, coll->sig->scon_handle);

    /* the base functions pushed us into the event library
     * before calling us, so we can safely access global data
     * at this point */

   // relay = SCON_NEW(scon_buffer_t);
    relay = (scon_buffer_t*) malloc(sizeof(scon_buffer_t));
    scon_buffer_construct(relay);
    /* pack the signature */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(relay, &coll->sig, 1,
                              SCON_COLLECTIVES_SIGNATURE))) {
        SCON_ERROR_LOG(rc);
        goto CLEANUP;
    }

    /* pass along the payload */
    if (SCON_SUCCESS != (rc = scon_bfrop.copy_payload(relay, buf))) {
        SCON_ERROR_LOG(rc);
        goto CLEANUP;
    }
    /* send the info to ourselves for tracking */
    if (SCON_SUCCESS != (rc = pt2pt_base_api_send_nb(coll->sig->scon_handle,
                              SCON_PROC_MY_NAME, relay,
                              SCON_MSG_TAG_ALLGATHER_DIRECT,
                              scon_collectives_base_allgather_send_complete_callback,
                              coll,
                              NULL, 0))) {
        SCON_ERROR_LOG(rc);
        goto CLEANUP;
    }
    return SCON_SUCCESS;
CLEANUP:
    scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                        "%s allgather forward to ourselves failed with error=%d"
                        "nprocs =%d, on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        rc,
                        (int)coll->sig->nprocs, coll->sig->scon_handle);
    /* error occured processing allgather, release the  relay buffer
       and call user's callback */
    //SCON_RELEASE(relay);
    free(relay);
   allgather = (scon_allgather_t*) coll->req;
    allgather->status = rc;
    if (NULL != allgather->cbfunc) {
        allgather->cbfunc(allgather->status,
                          allgather->scon_handle,
                          allgather->procs,
                          allgather->nprocs,
                          allgather->buf,
                          allgather->info,
                          allgather->ninfo,
                          allgather->cbdata);
    }
    SCON_RELEASE(allgather);
    SCON_RELEASE(coll);
    return rc;
}
static int barrier(scon_collectives_tracker_t *coll)
{
    return SCON_ERR_NOT_IMPLEMENTED;
}

static void allgather_recv(scon_status_t status,
                           scon_handle_t scon_handle,
                           scon_proc_t *peer,
                           scon_buffer_t *buf,
                           scon_msg_tag_t tag,
                           void *cbdata)
{
    int32_t cnt;
    int rc, ret;
    scon_collectives_signature_t *sig;
    scon_buffer_t *reply;
    scon_collectives_tracker_t *coll;
    scon_comm_scon_t *scon;
    scon_xcast_t *xcast;
    scon_proc_t *parent;
    /* retrieve the scon on which the msg was received */
    if( NULL == (scon = (scon_comm_base_get_scon(scon_handle))))
    {
        scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                            "%s allgather direct:received allgather from %s on invalid scon=%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(peer),
                            scon_handle);
        SCON_ERROR_LOG(SCON_ERR_NOT_FOUND);
    }
    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "%s allgather direct:received allgather from %s on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(peer),
                        scon_handle);
    /* unpack the signature */
    cnt = 1;
    if (SCON_SUCCESS != (rc = scon_bfrop.unpack(buf, &sig, &cnt, SCON_COLLECTIVES_SIGNATURE))) {
        SCON_ERROR_LOG(rc);
        return;
    }
    sig->scon_handle = scon->handle;
    /* check for the tracker and create it if not found */
    if (NULL == (coll = scon_collectives_base_get_tracker(sig, true))) {
        SCON_ERROR_LOG(SCON_ERR_NOT_FOUND);
        SCON_RELEASE(sig);
        return;
    }

    /* increment nprocs reported for collective */
    coll->nreported++;
    /* capture any provided content */
    scon_bfrop.copy_payload(&coll->bucket, buf);
    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "%s collectives:direct allgather recv nexpected %d nrep %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)coll->nexpected, (int)coll->nreported);
    scon_output(0,  "%s collectives:direct allgather recv nexpected %d nrep %d",
                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                (int)coll->nexpected, (int)coll->nreported);
    /* see if everyone has reported */
    if (coll->nreported == coll->nexpected) {
        if (is_master(scon)) {
            scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                                "%s collectives:default allgather master reports complete"
                                "releasing all gather",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME));
            /* the allgather is complete - send the xcast */
            //reply = SCON_NEW(scon_buffer_t);
            reply = (scon_buffer_t*) malloc(sizeof(scon_buffer_t));
            scon_buffer_construct(reply);
            /* pack the signature */
            if (SCON_SUCCESS != (rc = scon_bfrop.pack(reply, &sig, 1, SCON_COLLECTIVES_SIGNATURE))) {
                SCON_ERROR_LOG(rc);
               // SCON_RELEASE(reply);
                free(reply);
                SCON_RELEASE(sig);
                return;
            }
            /* pack the status - success since the allgather completed. This
             * would be an error if we timeout instead */
            ret = SCON_SUCCESS;
            if (SCON_SUCCESS != (rc = scon_bfrop.pack(reply, &ret, 1, SCON_INT))) {
                SCON_ERROR_LOG(rc);
                //SCON_RELEASE(reply);
                free(reply);
                SCON_RELEASE(sig);
                return;
            }
            /* transfer the collected bucket */
            scon_bfrop.copy_payload(reply, &coll->bucket);
            /* send the release via xcast */
            xcast = SCON_NEW(scon_xcast_t);
            xcast->scon_handle = scon->handle;
            xcast->procs = sig->procs;
            xcast->nprocs = sig->nprocs;
            xcast->buf = reply;
            xcast->tag = SCON_MSG_TAG_COLL_RELEASE;
            xcast->cbfunc = NULL;
            xcast->cbdata = NULL;
            xcast->info = NULL;
            xcast->ninfo = 0;
            scon_output(0, "%s allgather complete, sending xcast to release",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));
            scon->collective_module->xcast(xcast);
            //SCON_RELEASE(reply);
            //free(reply);
        } else {
            SCON_PROC_CREATE(parent,1);
            *parent = scon->topology_module->api.get_nexthop(&scon->topology_module->topology,
                      SCON_GET_MASTER(scon));
            scon_output(0, "%s received all inputs, sending my collection to master %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(SCON_GET_MASTER(scon)));
            scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                                "%s collectives:default allgather rollup  complete"
                                "sending bucket to parent %s",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                SCON_PRINT_PROC(parent));
            /* relay the bucket upward */
            // reply = SCON_NEW(scon_buffer_t);
            reply = (scon_buffer_t*) malloc(sizeof(scon_buffer_t));
            scon_buffer_construct(reply);
            /* pack the signature */
            if (SCON_SUCCESS != (rc = scon_bfrop.pack(reply, &sig, 1, SCON_COLLECTIVES_SIGNATURE))) {
                SCON_ERROR_LOG(rc);
                //SCON_RELEASE(reply);
                free(reply);
                SCON_RELEASE(sig);
                SCON_PROC_FREE(parent, 1);
                return;
            }
            /* transfer the collected bucket */
            scon_bfrop.copy_payload(reply, &coll->bucket);

            /* send the info to our parent */
            if(SCON_SUCCESS != (rc = pt2pt_base_api_send_nb(scon_handle,
                                     parent, reply,
                                     SCON_MSG_TAG_ALLGATHER_DIRECT,
                                     scon_collectives_base_allgather_send_complete_callback,
                                     coll,
                                     NULL, 0))) {
                SCON_ERROR_LOG(rc);
                //SCON_RELEASE(reply);
                free(reply);
                SCON_RELEASE(sig);
                SCON_PROC_FREE(parent, 1);
                return;
            }
            scon_output(0, "%s sent my allgather bucket up to %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(parent));
            SCON_PROC_FREE(parent, 1);
        }
    }
   // SCON_RELEASE(sig);
}

static void xcast_recv(scon_status_t status,
                       scon_handle_t scon_handle,
                       scon_proc_t *peer,
                       scon_buffer_t *buf,
                       scon_msg_tag_t tag,
                       void *cbdata)
{
    int32_t cnt;
    int ret;
    scon_collectives_signature_t *sig;
    scon_buffer_t *relay, *msg;
    scon_comm_scon_t *scon;
    scon_list_t relay_list;
    scon_list_item_t *item;
    scon_proc_t *relay_proc;
    /* retrieve the scon on which the msg was received */
    if( NULL == (scon = scon_comm_base_get_scon(scon_handle)))
    {
        scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                            "%s xcast recv:received xcast from %s on invalid scon=%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(peer),
                            scon_handle);
        return;
    }
    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "%s xcast recv:received xcast relay from %s on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(peer),
                        scon_handle);
    /* we need a passthru buffer to send to our children */
    //relay = SCON_NEW(scon_buffer_t);
    relay = (scon_buffer_t*) malloc(sizeof(scon_buffer_t));
    scon_buffer_construct(relay);
    scon_bfrop.copy_payload(relay, buf);

    /* extract the signature that we do not need */
    cnt=1;
    if (SCON_SUCCESS != (ret = scon_bfrop.unpack(buf, &sig, &cnt, SCON_COLLECTIVES_SIGNATURE))) {
        SCON_ERROR_LOG(ret);
        scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                            "%s xcast recv:received xcast from %s unpack error scon=%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(peer),
                            scon_handle);
        return;
    }
    /* extract the target tag */
    cnt=1;
    if (SCON_SUCCESS != (ret = scon_bfrop.unpack(buf, &tag, &cnt, SCON_UINT32))) {
        SCON_ERROR_LOG(ret);
        scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                            "%s xcast recv:received xcast from %s unpack error scon=%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(peer),
                            scon_handle);
        return;
    }

    //SCON_RELEASE(sig);
    /* setup a buffer we can pass to ourselves - this just contains
     * the actual message, minus the headers inserted by xcast itself */
    //msg = SCON_NEW(scon_buffer_t);
    msg = (scon_buffer_t*) malloc (sizeof(scon_buffer_t));
    scon_buffer_construct(msg);
    scon_bfrop.copy_payload(msg, buf);


    /* setup the relay list */
    SCON_CONSTRUCT(&relay_list, scon_list_t);

    /* get the list of next recipients from the routed module */
    scon->topology_module->api.get_routing_list(&scon->topology_module->topology, &relay_list);

    /* if list is empty, no relay is required */
    /* send the message to each recipient on list, deconstructing it as we go */
    while (NULL != (item = scon_list_remove_first(&relay_list))) {
        relay_proc = (scon_proc_t*)((scon_proc_list_t*)item)->name;
        scon_output_verbose(5,  scon_collectives_base_framework.framework_output,
                            "%s collectives_default xcast recv: relaying xcast  msg of %d bytes to %s on scon=%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            (int)relay->bytes_used,
                            SCON_PRINT_PROC(relay_proc),
                            scon_handle);
        /* retain the buffer as we used the same one to send multiple procs */
        //SCON_RETAIN(relay);
        if (SCON_SUCCESS != (ret = pt2pt_base_api_send_nb(scon_handle,
                                   relay_proc,
                                   relay,
                                   SCON_MSG_TAG_XCAST,
                                   xcast_relay_send_complete_callback, NULL,
                                   NULL, 0)))  {
            scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                                "%s xcast recv: unable to relay xcast to %s  error %d scon=%d",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                SCON_PRINT_PROC(relay_proc),
                                ret,
                                scon_handle);
            SCON_ERROR_LOG(ret);
            SCON_RELEASE(relay);
            SCON_RELEASE(item);
            continue;
        }
        SCON_RELEASE(item);
    }
    //free(relay);
    //SCON_RELEASE(relay);  // retain accounting
    /* cleanup */
    SCON_DESTRUCT(&relay_list);

    /* now send the relay buffer to myself for processing  if I am included in the siglist*/
    if (SCON_SUCCESS != (ret = pt2pt_base_api_send_nb(scon_handle,
                               SCON_PROC_MY_NAME,
                               msg,
                               tag,
                               xcast_relay_send_complete_callback, NULL,
                               NULL, 0))) {
        scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                            "%s xcast recv: unable to relay xcast to self  error %d scon=%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            ret,
                            scon_handle);
        SCON_ERROR_LOG(ret);
        //SCON_RELEASE(msg);
        //free(msg);
    }
}

static void barrier_release(scon_status_t status,
                            scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_buffer_t *buf,
                            scon_msg_tag_t tag,
                            void *cbdata)
{
    int32_t cnt;
    int rc, ret;
    scon_collectives_signature_t *sig;
    scon_collectives_tracker_t *coll;
    scon_comm_scon_t *scon;
    scon_allgather_t *allgather = NULL;
    scon_output(0, "%s barrier_release: called with %d bytes on scon %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)buf->bytes_used,
                        scon_handle);
    scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                        "%s barrier_release: called with %d bytes on scon %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)buf->bytes_used,
                        scon_handle);
    /* retrieve the scon */
    /* retrieve the scon on which the msg was received */
    if( NULL == (scon = scon_comm_base_get_scon(scon_handle)))
    {
        scon_output_verbose(0,  scon_collectives_base_framework.framework_output,
                            "%s barrier_release: called with %d bytes on invalid scon=%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            (int)buf->bytes_used,
                            scon_handle);
        return;
    }
    /* unpack the signature */
    cnt = 1;
    if (SCON_SUCCESS != (rc = scon_bfrop.unpack(buf, &sig, &cnt, SCON_COLLECTIVES_SIGNATURE))) {
        SCON_ERROR_LOG(rc);
        return;
    }

    /* unpack the return status */
    cnt = 1;
    if (SCON_SUCCESS != (rc = scon_bfrop.unpack(buf, &ret, &cnt, SCON_INT))) {
        SCON_ERROR_LOG(rc);
        return;
    }

    /* check for the tracker - it is not an error if not
     * found as that just means we wre not involved
     * in the collective */
    if (NULL == (coll = scon_collectives_base_get_tracker(sig, false))) {
        SCON_RELEASE(sig);
        return;
    }

    /* execute the callback : TO DO we need to differentiate between a barrier and
    * allgather here */
    allgather = (scon_allgather_t*)coll->req;
    scon_output(0, "%s barrier release: calling allgather coll->req= %p coll->nreported = %lu ",
                SCON_PRINT_PROC(SCON_PROC_MY_NAME), (void*)allgather, coll->nreported);
    if (NULL != allgather && NULL != allgather->cbfunc) {
        scon_output(0, "%s barrier release: calling allgather cbfunc with status = %d",
                       SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                       status);
        allgather->cbfunc(status, scon_handle, allgather->procs,
                          allgather->nprocs,  buf, allgather->info,
                          allgather->ninfo, allgather->cbdata);
    }
    scon_list_remove_item(&scon_collectives_base.ongoing, &coll->super);
   // SCON_RELEASE(allgather);
    SCON_RELEASE(coll);
}
