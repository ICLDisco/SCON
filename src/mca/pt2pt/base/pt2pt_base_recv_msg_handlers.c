/* -*- C -*-
 *
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2013 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/** @file:
 *
 */

/*
 * includes
 */
#include "src/buffer_ops/buffer_ops.h"
#include "src/util/output.h"
#include "src/class/scon_list.h"
#include "src/util/name_fns.h"
#include "src/mca/comm/base/base.h"
#include "src/mca/pt2pt/base/base.h"
#include "src/buffer_ops/types.h"

static void match_posted_recv(scon_posted_recv_t *rcv,
                              bool get_all,
                              scon_comm_scon_t *scon);

static void pt2pt_base_complete_recv_msg (scon_recv_t **recv_msg);

void pt2pt_base_post_recv(int sd, short args, void *cbdata)
{
    scon_recv_req_t *req = (scon_recv_req_t*)cbdata;
    scon_posted_recv_t *post, *recv;
    scon_comm_scon_t *scon;
    scon_ns_cmp_bitmask_t mask = SCON_NS_CMP_ALL | SCON_NS_CMP_WILD;

    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s posting recv",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));

    if (NULL == req) {
        /* this can only happen if something is really wrong, but
         * someone managed to get here in a bizarre test */
        scon_output(0, "%s CANNOT POST NULL SCON RECV REQUEST",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        return;
    }
    post = req->post;
    scon = scon_comm_base_get_scon(post->scon_handle);
    if (NULL == scon) {
        /* something went wrong */
        scon_output(0, "%s dangling SCON RECV request, invalid scon handle %d",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), post->scon_handle );
    }
    /* if the request is to cancel a recv, then find the recv
     * and remove it from our list
     */
    if (req->cancel) {
        SCON_LIST_FOREACH(recv, &scon->posted_recvs, scon_posted_recv_t) {
            if (SCON_EQUAL == scon_util_compare_name_fields(mask, &post->peer, &recv->peer) &&
                post->tag == recv->tag) {
                scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                    "%s canceling recv %d for peer %s",
                                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                    post->tag, SCON_PRINT_PROC(&recv->peer));
                /* got a match - remove it */
                scon_list_remove_item(&scon->posted_recvs, &recv->super);
                SCON_RELEASE(recv);
                break;
            }
        }
        SCON_RELEASE(req);
        return;
    }

    /* bozo check - cannot have two receives for the same peer/tag combination */
    SCON_LIST_FOREACH(recv, &scon->posted_recvs, scon_posted_recv_t) {
        if (SCON_EQUAL == scon_util_compare_name_fields(mask, &post->peer, &recv->peer) &&
            post->tag == recv->tag) {
            scon_output(0, "%s TWO RECEIVES WITH SAME PEER %s AND TAG %d - ABORTING",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&post->peer), post->tag);
            //abort();
        }
    }
    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s posting %s recv on tag %d for peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (post->persistent) ? "persistent" : "non-persistent",
                        post->tag, SCON_PRINT_PROC(&post->peer));
    /* add it to the list of recvs */
    scon_list_append(&scon->posted_recvs, &post->super);
    req->post = NULL;
    /* handle any messages that may have already arrived for this recv */
    match_posted_recv(post, post->persistent, scon);

    /* cleanup */
    SCON_RELEASE(req);
}

static void pt2pt_base_complete_recv_msg (scon_recv_t **recv_msg)
{
    scon_posted_recv_t *post;
    scon_ns_cmp_bitmask_t mask = SCON_NS_CMP_ALL | SCON_NS_CMP_WILD;
    scon_buffer_t buf;
    scon_recv_t *msg = *recv_msg;
    scon_comm_scon_t *scon;
    scon = scon_comm_base_get_scon(msg->scon_handle);

    if (NULL == scon) {
        SCON_ERROR_LOG(SCON_ERR_NOT_FOUND);
        scon_output(0, "OOPS received a message on a non existent SCON handle %d, tag %d",
                   msg->scon_handle, msg->tag);
        return;
    }

    /* see if we have a waiting recv for this message */
    SCON_LIST_FOREACH(post, &scon->posted_recvs, scon_posted_recv_t) {
        /* since names could include wildcards, must use
         * the more generalized comparison function
         */
        if (SCON_EQUAL == scon_util_compare_name_fields(mask, &msg->sender, &post->peer) &&
            msg->tag == post->tag) {
            /* deliver the data in the buffer */
            //SCON_CONSTRUCT(&buf, scon_buffer_t);
            scon_buffer_construct(&buf);
            if(SCON_SUCCESS != scon_buffer_load(&buf, msg->iov.iov_base, msg->iov.iov_len)) {
                scon_output(0, "%s error loading received buffer on scon %d tag =%d from peer %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon->handle, msg->tag,
                            SCON_PRINT_PROC(&msg->sender));
                SCON_ERROR_LOG(SCON_ERR_SILENT);
            }
            /* xfer ownership of the malloc'd data to the buffer */
            msg->iov.iov_base = NULL;
            post->cbfunc(SCON_SUCCESS, msg->scon_handle, &msg->sender, &buf, msg->tag, post->cbdata);
            /* the user must have unloaded the buffer if they wanted
             * to retain ownership of it, so release whatever remains
             */
            scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                     "%s message received  bytes from %s for tag %d called callback",
                                     SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                     SCON_PRINT_PROC(&msg->sender),
                                     msg->tag);
            /* release the message */
            SCON_RELEASE(msg);
            scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                 "%s message tag %d on released",
                                 SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                 post->tag);

            /* if the recv is non-persistent, remove it */
            if (!post->persistent) {
                scon_list_remove_item(&scon->posted_recvs, &post->super);
                scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                     "%s non persistent recv %p remove success releasing now",
                                     SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                     (void*)post);
                SCON_RELEASE(post);

            }
            return;
        }
    }
    /* we get here if no matching recv was found - we then hold
     * the message until such a recv is issued
     */
     scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "%s message received bytes from %s for tag %d on scon %d Not Matched adding to unmatched msgs",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&msg->sender),
                            msg->tag,
                            msg->scon_handle);
     scon_output(0, "message not matched on scon %d tag %d adding to queue", msg->scon_handle, msg->tag);
     scon_list_append(&scon->unmatched_msgs, &msg->super);
}

static void match_posted_recv(scon_posted_recv_t *rcv,
                              bool get_all,
                              scon_comm_scon_t *scon)
{
    scon_list_item_t *item, *next;
    scon_recv_t *msg;
    scon_ns_cmp_bitmask_t mask = SCON_NS_CMP_ALL | SCON_NS_CMP_WILD;
    /* scan thru the list of unmatched recvd messages and
     * see if any matches this spec - if so, push the first
     * into the recvd msg queue and look no further
     */
    item = scon_list_get_first(&scon->unmatched_msgs);
    while (item != scon_list_get_end(&scon->unmatched_msgs)) {
        next = scon_list_get_next(item);
        msg = (scon_recv_t*)item;
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "%s checking recv for %s against unmatched msg from %s on scon %d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&rcv->peer),
                            SCON_PRINT_PROC(&msg->sender),
                            scon->handle);

        /* since names could include wildcards, must use
         * the more generalized comparison function
         */
        if (SCON_EQUAL == scon_util_compare_name_fields(mask, &msg->sender, &rcv->peer) &&
            msg->tag == rcv->tag) {
            /* setup the event */
            scon_output(0, "matched recv message with unmatched msg on scon %d tag %d",
                           rcv->tag, rcv->scon_handle);
            scon_event_set(scon_globals.evbase, &msg->ev, -1,
                               SCON_EV_WRITE,
                               pt2pt_base_process_recv_msg, msg);
            scon_event_set_priority(&msg->ev, SCON_MSG_PRI);
            scon_event_active(&msg->ev, SCON_EV_WRITE, 1);
            scon_list_remove_item(&scon->unmatched_msgs, item);

            if (!get_all) {
                break;
            }
        }
        item = next;
    }
}

void pt2pt_base_process_recv_msg(int fd, short flags, void *cbdata)
{
    scon_recv_t *msg = (scon_recv_t*)cbdata;

    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                         "%s message received from %s for tag %d",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         SCON_PRINT_PROC(&msg->sender),
                         msg->tag);
    pt2pt_base_complete_recv_msg(&msg);
}

