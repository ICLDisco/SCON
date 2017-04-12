/*
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2007-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2013      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2013-2017 Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "scon_config.h"
#include "scon_common.h"

#include <stddef.h>
#include <math.h>

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
#include "src/mca/collectives/base/base.h"
#include "src/mca/collectives/collectives.h"
#include "collectives_rcd.h"

/* Static API's */
static int init(scon_handle_t scon_handle);
static void finalize(scon_handle_t scon_handle);
static int allgather(scon_collectives_tracker_t *coll,
                     scon_buffer_t *buf);
static int barrier(scon_collectives_tracker_t *coll);

/* Module def */
scon_collectives_module_t scon_collectives_rcd_module = {
    init,
    NULL,
    barrier,
    allgather,
    finalize
};

/* internal functions */
static void rcd_allgather_process_data(scon_collectives_tracker_t *coll,
                                          uint32_t distance);
static int rcd_allgather_send_dist(scon_collectives_tracker_t *coll,
                                      scon_proc_t *peer,
                                      uint32_t distance);
static void rcd_allgather_recv_dist(int status,
                                       scon_handle_t handle,
                                       scon_proc_t* sender,
                                       scon_buffer_t* buffer,
                                       scon_msg_tag_t tag,
                                       void* cbdata);
static void rcd_barrier_recv_dist(int status,
                                       scon_handle_t handle,
                                       scon_proc_t* sender,
                                       scon_buffer_t* buffer,
                                       scon_msg_tag_t tag,
                                       void* cbdata);
static int rcd_finalize_coll(scon_collectives_tracker_t *coll,
                                int ret);

/**
 * Initialize the module
 */
static int init(scon_handle_t scon_handle)
{
    /* post the receives */
    pt2pt_base_api_recv_nb(scon_handle,
                           SCON_PROC_WILDCARD,
                           SCON_MSG_TAG_ALLGATHER_RCD,
                           SCON_MSG_PERSISTENT,
                           rcd_allgather_recv_dist, NULL,
                           NULL, 0);
    pt2pt_base_api_recv_nb(scon_handle,
                           SCON_PROC_WILDCARD,
                           SCON_MSG_TAG_BARRIER_RCD,
                           SCON_MSG_PERSISTENT,
                           rcd_barrier_recv_dist, NULL,
                           NULL, 0);
    return SCON_SUCCESS;
}

/**
 * Finalize the module
 */
static void finalize(scon_handle_t scon_handle)
{
    /* cancel the recv */
    pt2pt_base_api_recv_cancel(scon_handle, SCON_PROC_WILDCARD, SCON_MSG_TAG_BARRIER_RCD);
    pt2pt_base_api_recv_cancel(scon_handle, SCON_PROC_WILDCARD, SCON_MSG_TAG_ALLGATHER_RCD);
}

static int allgather(scon_collectives_tracker_t *coll,
                     scon_buffer_t *buf)
{

    uint32_t log2nprocs;

    /* check the number of involved daemons - if it is not a power of two,
     * then we cannot do it */
    if (0 == ((coll->sig->nprocs != 0) && !(coll->sig->nprocs & (coll->sig->nprocs - 1)))) {
        return SCON_ERR_TAKE_NEXT_OPTION;
    }

    log2nprocs = log2 (coll->sig->nprocs);

    scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                        "%s rcd: allgather nprocs =%d, on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)coll->sig->nprocs, coll->sig->scon_handle);

    /* mark local data received */
    if (log2nprocs) {
        scon_bitmap_init (&coll->distance_mask_recv, log2nprocs);
    }
    /* TO DO : set my own rank */
    coll->my_rank = SCON_PROC_MY_NAME->rank;

    /* record that we contributed */
    coll->nreported = 1;

    /* start by seeding the collection with our own data */
    scon_bfrop.copy_payload(&coll->bucket, buf);

    /* process data */
    rcd_allgather_process_data (coll, 0);

    return SCON_SUCCESS;
}

static int rcd_allgather_send_dist(scon_collectives_tracker_t *coll,
                                      scon_proc_t *peer,
                                      uint32_t distance)
{
    scon_buffer_t *send_buf;
    int rc;

    //send_buf = SCON_NEW(scon_buffer_t);
    send_buf = (scon_buffer_t*) malloc (sizeof(scon_buffer_t));
    /* pack the signature */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(send_buf, &coll->sig, 1, SCON_COLLECTIVES_SIGNATURE))) {
        SCON_ERROR_LOG(rc);
        //SCON_RELEASE(send_buf);
        free(send_buf);
        return rc;
    }
    /* pack the current distance */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(send_buf, &distance, 1, SCON_INT32))) {
        SCON_ERROR_LOG(rc);
        //SCON_RELEASE(send_buf);
        free(send_buf);
        return rc;
    }
    /* pack the number of daemons included in the payload */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(send_buf, &coll->nreported, 1, SCON_SIZE))) {
        SCON_ERROR_LOG(rc);
        //SCON_RELEASE(send_buf);
        free(send_buf);
        return rc;
    }
    /* pack the data */
    if (SCON_SUCCESS != (rc = scon_bfrop.copy_payload(send_buf, &coll->bucket))) {
        SCON_ERROR_LOG(rc);
        //SCON_RELEASE(send_buf);
        free(send_buf);
        return rc;
    }


    if (SCON_SUCCESS != (rc = pt2pt_base_api_send_nb(coll->sig->scon_handle,
                              peer, send_buf,
                              SCON_MSG_TAG_ALLGATHER_RCD,
                              scon_collectives_base_allgather_send_complete_callback, coll,
                              NULL, 0))) {
        SCON_ERROR_LOG(rc);
        //SCON_RELEASE(send_buf);
        free(send_buf);
        free(send_buf);
        return rc;
    };

    return SCON_SUCCESS;
}

static void rcd_allgather_process_data(scon_collectives_tracker_t *coll,
                                       uint32_t distance)
{
    /* Communication step:
     At every step i, rank r:
     - exchanges message containing all data collected so far with rank peer = (r ^ 2^i).
     */
    uint32_t log2nprocs = (uint32_t) log2(coll->sig->nprocs);
    scon_proc_t peer;
    int rc, peer_index;
    while (distance < log2nprocs) {
        scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                            "%s rcd: allgather process distance =%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            distance);

        /* first send my current contents */
        peer_index = coll->my_rank ^ (1 << distance);
        assert (peer_index < (int)coll->sig->nprocs);
        peer = coll->sig->procs[peer_index];

        rcd_allgather_send_dist(coll, &peer, distance);

        /* check whether data for next distance is available */
        if (NULL == coll->buffers || NULL == coll->buffers[distance]) {
            break;
        }

        scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                            "%s rcd: allgather DATA found for distance =%d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            distance);
        if (SCON_SUCCESS != (rc = scon_bfrop.copy_payload(&coll->bucket, coll->buffers[distance]))) {
            SCON_ERROR_LOG(rc);
            rcd_finalize_coll(coll, rc);
            return;
        }
        coll->nreported += 1 << distance;
        scon_collectives_base_mark_distance_recv(coll, distance);
        SCON_RELEASE(coll->buffers[distance]);
        coll->buffers[distance] = NULL;
        ++distance;
    }

    scon_output_verbose(2, scon_collectives_base_framework.framework_output,
                        "%s collectives rcd allgather reported %lu process from %lu",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME), (unsigned long)coll->nreported,
                        (unsigned long)coll->sig->nprocs);

    /* if we are done, then complete things */
    if (coll->nreported == coll->sig->nprocs) {
        rcd_finalize_coll(coll, SCON_SUCCESS);
    }
}

static void rcd_allgather_recv_dist(int status,
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
                        "%s rcd: allgather receiving from %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(sender));

    /* unpack the signature */
    cnt = 1;
    if (SCON_SUCCESS != (rc = scon_bfrop.unpack(buffer, &sig, &cnt, SCON_COLLECTIVES_SIGNATURE))) {
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
    distance = -1;
    cnt = 1;
    if (SCON_SUCCESS != (rc = scon_bfrop.unpack(buffer, &distance, &cnt, SCON_INT32))) {
        SCON_RELEASE(sig);
        SCON_ERROR_LOG(rc);
        rcd_finalize_coll(coll, rc);
        return;
    }
    assert(0 == scon_collectives_base_check_distance_recv(coll, distance));
    /* Check whether we can process next distance */
    if (coll->nreported && (!distance || scon_collectives_base_check_distance_recv(coll, distance - 1))) {
        scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                             "%s grpcomm:coll:rcd data from %d distance received, "
                             "Process the next distance.",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME), distance);
        /* capture any provided content */
        if (SCON_SUCCESS != (rc = scon_bfrop.copy_payload(&coll->bucket, buffer))) {
            SCON_RELEASE(sig);
            SCON_ERROR_LOG(rc);
            rcd_finalize_coll(coll, rc);
            return;
        }
        coll->nreported += (1 << distance);
        scon_collectives_base_mark_distance_recv(coll, distance);
        rcd_allgather_process_data(coll, distance + 1);
    } else {
        scon_output_verbose(2,  scon_collectives_base_framework.framework_output,
                             "%s grpcomm:coll:rcd data from %d distance received, "
                             "still waiting for data.",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME), distance);
        if (NULL == coll->buffers) {
            if (NULL == (coll->buffers = (scon_buffer_t **) calloc ((uint32_t) log2 (coll->sig->nprocs) , sizeof(scon_buffer_t *)))) {
                rc = SCON_ERR_OUT_OF_RESOURCE;
                SCON_RELEASE(sig);
                SCON_ERROR_LOG(rc);
                rcd_finalize_coll(coll, rc);
                return;
            }
        }
        //if (NULL == (coll->buffers[distance] = SCON_NEW(scon_buffer_t))) {
        if (NULL == (coll->buffers[distance] = (scon_buffer_t*) malloc (sizeof(scon_buffer_t)))) {
            rc = SCON_ERR_OUT_OF_RESOURCE;
            SCON_RELEASE(sig);
            SCON_ERROR_LOG(rc);
            rcd_finalize_coll(coll, rc);
            return;
        }
        if (SCON_SUCCESS != (rc = scon_bfrop.copy_payload(coll->buffers[distance], buffer))) {
            SCON_RELEASE(sig);
            SCON_ERROR_LOG(rc);
            rcd_finalize_coll(coll, rc);
            return;
        }
    }
    SCON_RELEASE(sig);
}

static int rcd_finalize_coll(scon_collectives_tracker_t *coll, int ret)
{
    scon_allgather_t *allgather = (scon_allgather_t *) coll->req;
    scon_output_verbose(5,  scon_collectives_base_framework.framework_output,
                        "%s rcd allgather/barrier collective complete on scon %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        coll->sig->scon_handle);
    /** TO DO : handle barrier also here **/
    /* execute the callback */
    if ((NULL != allgather) && (NULL != allgather->cbfunc)) {
        allgather->cbfunc(ret, coll->sig->scon_handle, allgather->procs,
                                allgather->nprocs,  &coll->bucket,
                                allgather->info,
                                allgather->ninfo,
                                allgather->cbdata);
    }
    scon_list_remove_item(&scon_collectives_base.ongoing, &coll->super);
    SCON_RELEASE(allgather);
    SCON_RELEASE(coll);
    return SCON_SUCCESS;
}

static void rcd_barrier_recv_dist(int status,
                                  scon_handle_t handle,
                                  scon_proc_t* sender,
                                  scon_buffer_t* buffer,
                                  scon_msg_tag_t tag,
                                  void* cbdata)
{

}

static int barrier(scon_collectives_tracker_t *coll)
{
    return SCON_ERR_NOT_IMPLEMENTED;
}
