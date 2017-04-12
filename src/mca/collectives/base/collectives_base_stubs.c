/*
 * Copyright (c) 2016-2017     Intel, Inc. All rights reserved.
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
#include "src/mca/collectives/base/base.h"
#include "src/mca/collectives/collectives.h"
#include "src/mca/comm/base/base.h"
#include "src/mca/pt2pt/pt2pt.h"
#include "src/mca/topology/topology.h"
#include "src/util/name_fns.h"


static void collectives_base_process_xcast (int fd, short flags, void *cbdata)
{
    scon_coll_req_t *req = (scon_coll_req_t*) cbdata;
    scon_comm_scon_t *scon;
    scon_xcast_t *xcast;
    scon_member_t * sm;
    size_t i = 0;
    scon = scon_comm_base_get_scon(req->post.xcast.scon_handle);
    xcast = SCON_NEW(scon_xcast_t);
    scon_collectives_module_t *xcast_module;
    if(req->post.xcast.nprocs == 0) {
        xcast->nprocs = scon_list_get_size(&scon->members);
        xcast->procs = (scon_proc_t*)malloc(xcast->nprocs *
                         sizeof(scon_proc_t));
        SCON_LIST_FOREACH(sm, &scon->members, scon_member_t) {
            strncpy(sm->name.job_name, xcast->procs[i].job_name, SCON_MAX_JOBLEN);
            xcast->procs[i].rank = sm->name.rank;
            ++i;
        }

    }
    else {
        xcast->nprocs = req->post.xcast.nprocs;
        xcast->procs = (scon_proc_t*)malloc(xcast->nprocs *
                                            sizeof(scon_proc_t));
        for(i = 0; i < xcast->nprocs; i++) {
            strncpy(req->post.xcast.procs[i].job_name, xcast->procs[i].job_name, SCON_MAX_JOBLEN);
            xcast->procs[i].rank = req->post.xcast.procs[i].rank;
        }
    }
    xcast->scon_handle= scon->handle;
    xcast->tag = req->post.xcast.tag;
    xcast->buf = req->post.xcast.buf ;
    xcast->cbfunc = req->post.xcast.cbfunc;
    xcast->cbdata = req->post.xcast.cbdata;
    scon_output_verbose(5, scon_collectives_base_framework.framework_output,
                        " %s calling collectives xcast for tag %d, nprocs =%d, on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        xcast->tag, (int)xcast->nprocs, xcast->scon_handle);
    /* not all collective modules support xcast so check it */
    if (NULL != scon->collective_module->xcast)
    {
        scon->collective_module->xcast(xcast);
    }
    else
    {
        xcast_module=scon_collectives_base_get_module("default");
        xcast_module->init(scon->handle);
        xcast_module->xcast(xcast);
    }
    SCON_RELEASE(req);
}

static void collectives_base_process_allgather (int fd, short flags, void *cbdata)
{
    scon_coll_req_t *req = (scon_coll_req_t*) cbdata;
    scon_collectives_tracker_t * coll;
    scon_collectives_signature_t *sig;
    scon_comm_scon_t *scon;
    scon_member_t * sm;
    void *seq_number;
    int ret;
    size_t i = 0;
    scon_allgather_t *allgather = SCON_NEW(scon_allgather_t);
    scon = scon_comm_base_get_scon(req->post.allgather.scon_handle);
    sig = SCON_NEW(scon_collectives_signature_t);
    sig->scon_handle = scon->handle;
    if(req->post.allgather.nprocs == 0) {
        sig->nprocs = scon_list_get_size(&scon->members);
        sig->procs = (scon_proc_t*)malloc(sig->nprocs *
                                          sizeof(scon_proc_t));
        SCON_LIST_FOREACH(sm, &scon->members, scon_member_t) {
            strncpy(sig->procs[i].job_name, sm->name.job_name,  SCON_MAX_JOBLEN);
            sig->procs[i].rank = sm->name.rank;
            ++i;
        }

    }
    else {
        sig->nprocs = req->post.allgather.nprocs;
        sig->procs = (scon_proc_t*)malloc(sig->nprocs *
                                          sizeof(scon_proc_t));
        for(i = 0; i < sig->nprocs; i++) {
            strncpy(sig->procs[i].job_name, req->post.allgather.procs[i].job_name, SCON_MAX_JOBLEN);
            sig->procs[i].rank = req->post.allgather.procs[i].rank;
        }
    }

    ret = scon_hash_table_get_value_ptr(&scon_collectives_base.coll_table,
                                        (void *)sig->procs,
                                        sig->nprocs * sizeof(scon_proc_t),
                                        &seq_number);
    if (SCON_ERR_NOT_FOUND == ret) {
        sig->seq_num = 0;
    } else if (SCON_SUCCESS == ret) {
        sig->seq_num = *((uint32_t *)(seq_number)) + 1;
    } else {
        scon_output(0, " %s SCON collectives base: fatal error sig hash table not initialized",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        scon_output_verbose(0, scon_collectives_base_framework.framework_output,
                            " %s SCON collectives base: fatal error sig hash table not initialized",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        SCON_ERROR_LOG(ret);
        SCON_RELEASE(req);
        return;
    }
    ret = scon_hash_table_set_value_ptr(&scon_collectives_base.coll_table,
                                        (void *)sig->procs, sig->nprocs * sizeof(scon_proc_t), (void *)&sig->seq_num);
    if (SCON_SUCCESS != ret) {
        scon_output_verbose(0, scon_collectives_base_framework.framework_output,
                            " %s SCON collectives base: fatal error sig hash table set failed",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        SCON_ERROR_LOG(ret);
        SCON_RELEASE(req);
        return;
    }
    /* retrieve an existing tracker, create it if not
    * already found. The allgather module is responsible
    * for releasing it upon completion of the collective */
    coll = scon_collectives_base_get_tracker(sig, true);
    scon_output_verbose(5, scon_collectives_base_framework.framework_output,
                        " %s calling allgather with  nprocs =%lu, on scon=%d buf =%p",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        req->post.allgather.nprocs, req->post.allgather.scon_handle,
                        (void*)req->post.allgather.buf);
    scon->collective_module->allgather(coll, req->post.allgather.buf);
    memcpy(allgather, &req->post.allgather, sizeof(scon_allgather_t));
    coll->req = (void*)allgather;
    SCON_RELEASE(req);
}

static void collectives_base_process_barrier (int fd, short flags, void *cbdata)
{
    scon_coll_req_t *req = (scon_coll_req_t*) cbdata;
    scon_comm_scon_t *scon;
    scon_collectives_tracker_t * coll;
    scon_collectives_signature_t *sig;
    scon_member_t * sm;
    void *seq_number;
    int ret;
    size_t i = 0;
    scon = scon_comm_base_get_scon(req->post.barrier.scon_handle);
    sig = SCON_NEW(scon_collectives_signature_t);
    sig->scon_handle = scon->handle;
    if(req->post.barrier.nprocs == 0) {
        sig->nprocs = scon_list_get_size(&scon->members);
        sig->procs = (scon_proc_t*)malloc(sig->nprocs *
                                                sizeof(scon_proc_t));
        SCON_LIST_FOREACH(sm, &scon->members, scon_member_t) {
            strncpy(sm->name.job_name, sig->procs[i].job_name, SCON_MAX_JOBLEN);
            sig->procs[i].rank = sm->name.rank;
            ++i;
        }

    }
    else {
        sig->nprocs = req->post.barrier.nprocs;
        sig->procs = (scon_proc_t*)malloc(sig->nprocs *
                                                sizeof(scon_proc_t));
        for(i = 0; i < sig->nprocs; i++) {
            strncpy(req->post.barrier.procs[i].job_name, sig->procs[i].job_name, SCON_MAX_JOBLEN);
            sig->procs[i].rank = req->post.barrier.procs[i].rank;
        }
    }
    ret = scon_hash_table_get_value_ptr(&scon_collectives_base.coll_table,
                                        (void *)sig->procs,
                                        sig->nprocs * sizeof(scon_proc_t),
                                        &seq_number);
    if (SCON_ERR_NOT_FOUND == ret) {
       sig->seq_num = 0;
    } else if (SCON_SUCCESS == ret) {
        sig->seq_num = *((uint32_t *)(seq_number)) + 1;
    } else {
        scon_output_verbose(0, scon_collectives_base_framework.framework_output,
                            " %s SCON collectives base: fatal error sig hash table not initialized",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        SCON_ERROR_LOG(ret);
        SCON_RELEASE(req);
        return;
    }
    ret = scon_hash_table_set_value_ptr(&scon_collectives_base.coll_table,
                                        (void *)sig->procs, sig->nprocs * sizeof(scon_proc_t), (void *)&sig->seq_num);
    if (SCON_SUCCESS != ret) {
        scon_output_verbose(0, scon_collectives_base_framework.framework_output,
                            " %s SCON collectives base: fatal error sig hash table set failed",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        SCON_ERROR_LOG(ret);
        SCON_RELEASE(req);
        return;
    }
    /* retrieve an existing tracker, create it if not
    * already found. The allgather module is responsible
    * for releasing it upon completion of the collective */
    coll = scon_collectives_base_get_tracker(sig, true);
    coll->req = &req->post.barrier;
    scon_output_verbose(5, scon_collectives_base_framework.framework_output,
                        " %s calling allgather with  nprocs =%lu, on scon=%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         coll->sig->nprocs, req->post.barrier.scon_handle);
    scon->collective_module->barrier(coll);
    SCON_RELEASE(req);
}
/* helper functions */
SCON_EXPORT scon_collectives_tracker_t* scon_collectives_base_get_tracker(
                                               scon_collectives_signature_t *sig,
                                               bool create)
{
    scon_collectives_tracker_t *coll;
    size_t n;
    bool match = true;
    scon_comm_scon_t *scon = scon_comm_base_get_scon(sig->scon_handle);
    scon_output(0, "searching for tracker for scon %d, seq num =%d num trackers=%lu",
                  sig->scon_handle, sig->seq_num,
                  scon_list_get_size(&scon_collectives_base.ongoing));
    /* search the existing tracker list to see if this already exists */
    SCON_LIST_FOREACH(coll, &scon_collectives_base.ongoing, scon_collectives_tracker_t) {

        /* check if there is an ongoing collective on the same set of procs */
        if((coll->sig->nprocs == sig->nprocs) && (coll->sig->seq_num == sig->seq_num) &&
            (coll->sig->scon_handle == sig->scon_handle)) {
            /* check if the participant list is the same */
            for(n = 0; n < sig->nprocs; n++) {
                if (SCON_EQUAL != scon_util_compare_name_fields(SCON_NS_CMP_ALL,
                        &sig->procs[n], &coll->sig->procs[n])) {
                    scon_output(0, "%s collectives_base_get_tracker failed to match %lu proc %s with %s",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME), n,
                                SCON_PRINT_PROC(&sig->procs[n]),
                                SCON_PRINT_PROC(&coll->sig->procs[n]));
                    match = false;
                    break;
                }
            }
            scon_output_verbose(1, scon_collectives_base_framework.framework_output,
                                "%s collectives_base_get_tracker match found = %d on scon %d ",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                match,
                                sig->scon_handle);
            if(match)
                return coll;
        }
    }
    /* if we get here, then this is a new collective - so create
     * the tracker for it */
    if (!create) {
        scon_output_verbose(1, scon_collectives_base_framework.framework_output,
                             "%s collectives:base: not creating new coll tracker",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME));

        return NULL;
    }
    coll = SCON_NEW(scon_collectives_tracker_t);
    coll->sig = sig;
  /*  coll->sig->scon_handle = sig->scon_handle;
    coll->sig->nprocs = sig->nprocs;
    coll->sig->seq_num = sig->seq_num;
    SCON_PROC_CREATE(coll->sig->procs, coll->sig->nprocs);
   // coll->sig->procs = (scon_proc_t*)malloc(coll->sig->nprocs *
    //                                        sizeof(scon_proc_t));
    for(i = 0; i < coll->sig->nprocs; i++) {
        strncpy(coll->sig->procs[i].job_name, sig->procs[i].job_name, SCON_MAX_JOBLEN);
        coll->sig->procs[i].rank = sig->procs[i].rank;
    }*/
    /*add this tracker to the list */
    scon_list_append(&scon_collectives_base.ongoing, &coll->super);
    scon_output(0, "%s scon_collectives_base_get_tracker, completed tracker setup calling num_routes",
                 SCON_PRINT_PROC(SCON_PROC_MY_NAME));
    /* To do  nexpected may not always be equal to the number of participants */
    if(NULL != scon->topology_module) {
        /* include ourselves too + number of procs under me*/
        coll->nexpected = 1+ scon->topology_module->api.num_routes(&scon->topology_module->topology);
    }
    else
        coll->nexpected = coll->sig->nprocs;
    return coll;
}

SCON_EXPORT void scon_collectives_base_mark_distance_recv(scon_collectives_tracker_t *coll,
                                               uint32_t distance)
{
    scon_bitmap_set_bit (&coll->distance_mask_recv, distance);
}
SCON_EXPORT unsigned int scon_collectives_base_check_distance_recv(scon_collectives_tracker_t *coll,
                                                        uint32_t distance)
{
    return scon_bitmap_is_set_bit (&coll->distance_mask_recv, distance);
}

/* stub functions */
SCON_EXPORT int collectives_base_api_xcast(scon_handle_t scon_handle,
                               scon_proc_t procs[],
                               size_t nprocs,
                               scon_buffer_t *buf,
                               scon_msg_tag_t tag,
                               scon_xcast_cbfunc_t cbfunc,
                               void *cbdata,
                               scon_info_t info[],
                               size_t ninfo)
{
    scon_comm_scon_t *scon;
    scon_coll_req_t *req;
    /* get the scon object*/
    if( NULL == (scon =scon_comm_base_get_scon(scon_handle)))
    {
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* create a collectives req and do the rest of the processing in
          an event */
        req = SCON_NEW(scon_coll_req_t);
        req->post.xcast.scon_handle = scon_handle ;
        req->post.xcast.buf = buf;
        req->post.xcast.tag = tag;
        req->post.xcast.procs = procs;
        req->post.xcast.nprocs = nprocs;
        req->post.xcast.cbfunc = cbfunc;
        req->post.xcast.cbdata = cbdata;
        req->post.xcast.info = info;
        req->post.xcast.ninfo = ninfo;
        scon_output_verbose(1, scon_collectives_base_framework.framework_output,
                            "%s collectives_base_api_xcast scon %d ",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            scon->handle);
        /* setup the event for rest of the processing  */
        scon_event_set(scon_globals.evbase, &req->ev, -1, SCON_EV_WRITE, collectives_base_process_xcast, req);
        scon_event_set_priority(&req->ev, SCON_MSG_PRI);
        scon_event_active(&req->ev, SCON_EV_WRITE, 1);
        scon_output_verbose(5, scon_collectives_base_framework.framework_output,
                             "%s collectives_base_api_xcast - set event done",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        return SCON_SUCCESS;
    }
}

SCON_EXPORT int collectives_base_api_barrier(scon_handle_t scon_handle,
                                 scon_proc_t procs[],
                                 size_t nprocs,
                                 scon_barrier_cbfunc_t cbfunc,
                                 void *cbdata,
                                 scon_info_t info[],
                                 size_t ninfo)
{
    scon_comm_scon_t *scon;
    scon_coll_req_t *req;
    /* get the scon object*/
    if( NULL == (scon = scon_comm_base_get_scon(scon_handle)))
    {
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* create a collectives req and do the rest of the processing in
          an event */
        req = SCON_NEW(scon_coll_req_t);
        req->post.barrier.scon_handle = scon_handle ;
        req->post.barrier.procs = procs;
        req->post.barrier.nprocs = nprocs;
        req->post.barrier.cbfunc = cbfunc;
        req->post.barrier.cbdata = cbdata;
        req->post.barrier.info = info;
        req->post.barrier.ninfo = ninfo;
        scon_output_verbose(1, scon_collectives_base_framework.framework_output,
                            "%s collectives_base_api_barrier scon %d ",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            scon->handle);
        /* setup the event for rest of the processing  */
        scon_event_set(scon_globals.evbase, &req->ev, -1, SCON_EV_WRITE, collectives_base_process_barrier, req);
        scon_event_set_priority(&req->ev, SCON_MSG_PRI);
        scon_event_active(&req->ev, SCON_EV_WRITE, 1);
        scon_output_verbose(5, scon_collectives_base_framework.framework_output,
                            "%s collectives_base_api_barrier - set event done",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        return SCON_SUCCESS;
    }
}

SCON_EXPORT int collectives_base_api_allgather(scon_handle_t scon_handle,
                                   scon_proc_t procs[],
                                   size_t nprocs,
                                   scon_buffer_t *buf,
                                   scon_allgather_cbfunc_t cbfunc,
                                   void *cbdata,
                                   scon_info_t info[],
                                   size_t ninfo)
{
    scon_comm_scon_t *scon;
    scon_coll_req_t *req;
    /* get the scon object*/
    if( NULL == (scon = scon_comm_base_get_scon(scon_handle)))
    {
        scon_output(0, "collectives_base_api_allgather: cannot find the scon with handle %d", scon_handle);
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* create a collectives req and do the rest of the processing in
          an event */
        req = SCON_NEW(scon_coll_req_t);
        req->post.allgather.scon_handle = scon_handle ;
        req->post.allgather.procs = procs;
        req->post.allgather.nprocs = nprocs;
        req->post.allgather.buf = buf;
        req->post.allgather.cbfunc = cbfunc;
        req->post.allgather.cbdata = cbdata;
        req->post.allgather.info = info;
        req->post.allgather.ninfo = ninfo;
        scon_output(0, "collectives_base_api_allgather:%s collectives_base_api_barrier scon %d ",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    scon->handle);
        scon_output_verbose(1, scon_collectives_base_framework.framework_output,
                            "%s collectives_base_api_barrier scon %d ",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            scon->handle);
        /* setup the event for rest of the processing  */
        scon_event_set(scon_globals.evbase, &req->ev, -1, SCON_EV_WRITE, collectives_base_process_allgather, req);
        scon_event_set_priority(&req->ev, SCON_MSG_PRI);
        scon_event_active(&req->ev, SCON_EV_WRITE, 1);
        scon_output(0, "%s collectives_base_api_allgather - set event done",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        scon_output_verbose(5, scon_collectives_base_framework.framework_output,
                            "%s collectives_base_api_allgather - set event done",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        return SCON_SUCCESS;
    }
    return 0;
}

SCON_EXPORT void scon_collectives_base_allgather_send_complete_callback(
    int status, scon_handle_t scon_handle,
    scon_proc_t* peer,
    scon_buffer_t* buffer,
    scon_msg_tag_t tag,
    void* cbdata)
{
    /** TO DO **/
}
