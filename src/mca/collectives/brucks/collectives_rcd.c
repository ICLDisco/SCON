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
#include "src/class/scon_bitmap.h"
#include "src/util/bit_ops.h"
#include "src/util/output.h"
#include "util/error.h"
#include "src/util/name_fns.h"
#include "src/include/scon_globals.h"

#include "src/mca/collectives/base/base.h"
#include "src/mca/collectives/collectives.h"
#include "collectives_brucks.h"

/* Static API's */
static int init(scon_common_scon_t * scon);
static void finalize(scon_common_scon_t * scon);
static int allgather(scon_collectives_tracker_t *coll,
                     scon_buffer_t *buf);
static int barrier(scon_collectives_tracker_t *coll);

/* Module def */
scon_collectives_module_t scon_collectives_brucks_module = {
    init,
    NULL,
    barrier,
    allgather,
    finalize
};

/* internal functions */
static void brucks_allgather_process_data(scon_collective_tracker_t *coll,
                                          uint32_t distance);
static int brucks_allgather_send_dist(scon_collective_tracker_t *coll,
                                      scon_proc_t *peer,
                                      uint32_t distance);
static void brucks_allgather_recv_dist(int status,
                                       scon_handle_t handle,
                                       scon_proc_t* sender,
                                       scon_buffer_t* buffer,
                                       scon_msg_tag_t tag,
                                       void* cbdata);
static void brucks_barrier_recv_dist(int status,
                                       scon_handle_t handle,
                                       scon_proc_t* sender,
                                       scon_buffer_t* buffer,
                                       scon_msg_tag_t tag,
                                       void* cbdata);
static int brucks_finalize_coll(scon_collective_tracker_t *coll,
                                int ret);
/**
 * Initialize the module
 */
static int init(scon_common_scon_t * scon)
{
    /* post the receives */
    pt2pt_base_api_recv_nb(scon->handle,
                           SCON_PROC_WILDCARD,
                           SCON_MSG_TAG_ALLGATHER_BRUCKS,
                           SCON_MSG_PERSISTENT,
                           brucks_allgather_recv_dist, NULL,
                           NULL, 0);
    pt2pt_base_api_recv_nb(scon->handle,
                           SCON_PROC_WILDCARD,
                           SCON_MSG_TAG_BARRIER_BRUCKS,
                           SCON_MSG_PERSISTENT,
                           brucks_barrier_recv_dist, NULL,
                           NULL, 0);
    return SCON_SUCCESS;
}

/**
 * Finalize the module
 */
static void finalize(scon_common_scon_t * scon)
{
    /* cancel the recv */
    pt2pt_base_api_recv_cancel(scon->handle, SCON_PROC_WILDCARD, SCON_MSG_TAG_BARRIER_BRUCKS);
    pt2pt_base_api_recv_cancel(scon->handle, SCON_PROC_WILDCARD, SCON_MSG_TAG_ALLGATHER_BRUCKS);
}

static int allgather(scon_collectives_tracker_t *coll,
                     scon_buffer_t *buf))
{
    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "%s brucks: allgather nprocs =%d, on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)coll->sig->nprocs, coll->sig->scon->handle);
    /* TO DO : set my own rank */
    coll->my_rank = SCON_PROC_MY_NAME->rank;

    /* record that we contributed */
    coll->nreported = 1;

    /* mark local data received */

    scon_bitmap_init (&coll->distance_mask_recv, (uint32_t) log2 (coll->sig->nprocs) + 1)

    /* start by seeding the collection with our own data */
    scon_bfrop.copy_payload(&coll->bucket, buf);

    /* process data */
    brucks_allgather_process_data (coll, 0);

    return SCON_SUCCESS;
}

static int brucks_allgather_send_dist(scon_collectives_tracker_t *coll,
                                      scon_proc_t *peer,
                                      uint32_t distance)
{
    scon_buffer_t *send_buf;
    int rc;

    send_buf = SCON_NEW(scon_buffer_t);

    /* pack the signature */
    if (SCON_SUCCESS != (rc = scon_bfrops.pack(send_buf, &coll->sig, 1, SCON_COLLECTIVE_SIGNATURE))) {
        SCON_ERROR_LOG(rc);
        SCON_RELEASE(send_buf);
        return rc;
    }
    /* pack the current distance */
    if (SCON_SUCCESS != (rc = scon_bfrops.pack(send_buf, &distance, 1, SCON_INT32))) {
        SCON_ERROR_LOG(rc);
        SCON_RELEASE(send_buf);
        return rc;
    }
    /* pack the number of daemons included in the payload */
    if (SCONL_SUCCESS != (rc = scon_bfrops.pack(send_buf, &coll->nreported, 1, SCON_SIZE))) {
        SCON_ERROR_LOG(rc);
        SCON_RELEASE(send_buf);
        return rc;
    }
    /* pack the data */
    if (SCON_SUCCESS != (rc = scon_bfrops.copy_payload(send_buf, &coll->bucket))) {
        SCON_ERROR_LOG(rc);
        SCON_RELEASE(send_buf);
        return rc;
    }


    if (SCON_SUCCESS != (rc = pt2pt_base_api_send_nb(coll->sig->scon->handle,
                              peer, send_buf,
                              SCON_MSG_TAG_ALLGATHER_BRUCKS,
                              allgather_send_complete_callback, coll,
                              NULL, 0))) {
        SCON_ERROR_LOG(rc);
        SCON_RELEASE(send_buf);
        return rc;
    };

    return SCON_SUCCESS;
}

static int brucks_allgather_process_buffered (scon_collectives_tracker_t *coll, uint32_t distance) {
    scon_buffet_t *buffer;
    size_t nreceived;
    int32_t cnt = 1;
    int rc;

    /* check whether data for next distance is available*/
    if (NULL == coll->buffers || NULL == coll->buffers[distance]) {
        return 0;
    }

    buffer = coll->buffers[distance];
    coll->buffers[distance] = NULL;

    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                         "%s brucks: allgather found data for distance =%d",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        distance);
    rc = scon_bfrops.unpack (buffer, &nreceived, &cnt, SCON_SIZE);
    if (SCON_SUCCESS != rc) {
        SCON_ERROR_LOG(rc);
        brucks_finalize_coll(coll, rc);
        return rc;
    }

    if (SCON_SUCCESS != (rc = scon_bfrops.copy_payload(&coll->bucket, buffer))) {
        SCON_ERROR_LOG(rc);
        brucks_finalize_coll(coll, rc);
        return rc;
    }

    coll->nreported += nreceived;
    scon_collectives_base_mark_distance_recv (coll, distance);
    SCON_RELEASE(buffer);
    return 1;
}

static void brucks_allgather_process_data(scon_collectives_tracker_t *coll, uint32_t distance) {
    /* Communication step:
     At every step i, rank r:
     - doubles the distance
     - sends message containing all data collected so far to rank r - distance
     - receives message containing all data collected so far from rank (r + distance)
     */
    uint32_t log2nprocs = (uint32_t) log2 (coll->sig->nprocs);
    uint32_t last_round;
    scon_proc_t peer;
    int rc, peer_index;

    /* NTH: calculate in which round we should send the final data. this is the first
     * round in which we have data from at least (coll->sig->nprocs - (1 << log2nprocs))
     * procs. alternatively we could just send when distance reaches log2nprocs but
     * that could end up sending more data than needed */
    last_round = (uint32_t) ceil (log2 ((double) (coll->sig->nprocs - (1 << log2nprocs))));

    while (distance < log2nprocss) {
        scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                             "%s brucks: allgather process distance =%d",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                             distance);

        /* first send my current contents */
        peer_index = (coll->sig->nprocs + coll->my_rank - (1 << distance)) % coll->sig->nprocs;
        peer = coll->sig->procs[peer_index];

        brucks_allgather_send_dist(coll, &peer, distance);

        if (distance == last_round) {
            /* have enough data to send the final round now */
            peer_index = (coll->sig->nprocs + coll->my_rank - (1 << log2nprocs)) % coll->sig->nprocs;
            peer = coll->sig->procs[peer_index];
            brucks_allgather_send_dist(coll, &peer, log2nprocs);
        }

        rc = brucks_allgather_process_buffered (coll, distance);
        if (!rc) {
            break;
        } else if (rc < 0) {
            return;
        }

        ++distance;
    }

    if (distance == log2ndmns) {
        if (distance == last_round) {
            /* need to send the final round now */
            peer_index = (coll->sig->nprocs + coll->my_rank - (1 << log2nprocs)) % coll->sig->nprocs;
            peer = coll->sig->procspeer_index;
            brucks_allgather_send_dist(coll, &peer, log2ndmns);
        }

        /* check if the final message is already queued */
        rc = brucks_allgather_process_buffered (coll, distance);
        if (rc < 0) {
            return;
        }
    }

    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "%s brucks: allgather nreported =%d out of nexpected = %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        coll->nreported,
                        coll->nexpected);

    /* if we are done, then complete things. we may get data from more daemons than expected */
    if (coll->nreported >= coll->sig->nprocs){
        brucks_finalize_coll(coll, SCON_SUCCESS);
    }
}

static void brucks_allgather_recv_dist(int status,
                                       scon_handle_t handle,
                                       scon_proc_t* sender,
                                       scon_buffer_t* buffer,
                                       scon_msg_tag_t tag,
                                       void* cbdata)
{
    int32_t cnt;
    int rc;
    scon_collectives_signature_t *sig;
    scon_collectives_tracker_t *coll;
    uint32_t distance;

    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "%s brucks: allgather receiving from %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(sender));

    /* unpack the signature */
    cnt = 1;
    if (SCON_SUCCESS != (rc = scon_bfrops.unpack(buffer, &sig, &cnt, SCON_COLLECTIVES_SIGNATURE))) {
        SCON_ERROR_LOG(rc);
        return;
    }

    /* check for the tracker and create it if not found */
    if (NULL == (coll = scon_collectives_base_get_tracker (sig, true))) {
        SCON_ERROR_LOG(SCON_ERR_NOT_FOUND);
        SCON_RELEASE(sig);
        return;
    }
    /* unpack the distance */
    distance = 1;
    if (SCON_SUCCESS != (rc = scon_bfrops.unpack(buffer, &distance, &cnt, SCON_INT32))) {
        SCON_RELEASE(sig);
        SCON_ERROR_LOG(rc);
        brucks_finalize_coll(coll, rc);
        return;
    }
    assert(0 == scon_collectives_base_check_distance_recv(coll, distance));

    /* Check whether we can process next distance */
    if (coll->nreported && (!distance || scon_collectives_base_check_distance_recv(coll, distance - 1))) {
        size_t nreceived;
        scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                             "%s grpcomm:coll:brucks data from %d distance received, "
                             "Process the next distance.",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME), distance);
        /* capture any provided content */
        rc = scon_bfrop.unpack (buffer, &nreceived, &cnt, SCON_SIZE);
        if (SCON_SUCCESS != rc) {
            SCON_RELEASE(sig);
            SCON_ERROR_LOG(rc);
            brucks_finalize_coll(coll, rc);
            return;
        }
        if (SCON_SUCCESS != (rc = scon_bfrop.copy_payload(&coll->bucket, buffer))) {
            SCON_RELEASE(sig);
            SCON_ERROR_LOG(rc);
            brucks_finalize_coll(coll, rc);
            return;
        }
        coll->nreported += nreceived;
        scon_collectives_base_mark_distance_recv(coll, distance);
        brucks_allgather_process_data(coll, distance + 1);
    } else {
        scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                             "%s grpcomm:coll:brucks data from %d distance received, "
                             "still waiting for data.",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAMEE), distance);
        if (NULL == coll->buffers) {
            if (NULL == (coll->buffers = (scon_buffer_t **) calloc ((uint32_t) log2 (coll->sig->nprocs) + 1, sizeof(scon_buffer_t *)))) {
                rc = SCON_ERR_OUT_OF_RESOURCE;
                SCON_RELEASE(sig);
                SCON_ERROR_LOG(rc);
                brucks_finalize_coll(coll, rc);
                return;
            }
        }
        if (NULL == (coll->buffers[distance] = SCON_NEW(scon_buffer_t))) {
            rc = SCON_ERR_OUT_OF_RESOURCE;
            SCON_RELEASE(sig);
            SCON_ERROR_LOG(rc);
            brucks_finalize_coll(coll, rc);
            return;
        }
        if (SCON_SUCCESS != (rc = scon_bfrop.copy_payload(coll->buffers[distance], buffer))) {
            CON_RELEASE(sig);
            SCON_ERROR_LOG(rc);
            brucks_finalize_coll(coll, rc);
            return;
        }
    }

    SCON_RELEASE(sig);
}

static int brucks_finalize_coll(scon_collective_tracker_t *coll, int ret)
{
    scon_output_verbose(5,  scon_collectives_base_framework.framework_output,
                        "%s brucks allgather/barrier collective complete on scon %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        coll->sig->scon->handle);
    /** TO DO : handle barrier also here **/
    /* execute the callback */
    if ((NULL != coll->allgather) && (NULL != coll->allgather->cbfunc)) {
        coll->allgather->cbfunc(ret, coll->sig->scon->handle, coll->allgather->procs,
                                coll->allgather->nprocs,  &coll->bucket,
                                coll->allgather->info,
                                coll->allgather->ninfo,
                                coll->allgather->cbdata);
    }
    scon_list_remove_item(&scon_collectives_base.ongoing, &coll->super);
    SCON_RELEASE(coll->allgather);
    SCON_RELEASE(coll);
    return SCON_SUCCESS;
}


