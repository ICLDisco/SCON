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

#include "orte/mca/scon/base/base.h"
#include "orte/mca/scon/scon.h"
#include "orte/mca/scon/native/scon_native.h"
#include "orte/runtime/runtime_internals.h"
#include "orte/runtime/orte_globals.h"
#include "orte/util/proc_info.h"
#include "orte/util/session_dir.h"
#include "orte/mca/rml/base/base.h"
#include "orte/mca/routed/base/base.h"
#include "orte/mca/routed/routed.h"
#include "orte/mca/oob/base/base.h"
#include "orte/mca/grpcomm/grpcomm.h"
#include "orte/mca/grpcomm/base/base.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/dfs/base/base.h"
#include "orte/mca/errmgr/base/base.h"
#include "orte/mca/state/base/base.h"

static int scon_native_start(void);
static void scon_native_shutdown(void);
static scon_module_t* scon_native_get_module();

/**
 * component definition
 */
mca_scon_base_component_t mca_scon_native_component = {
    /* First, the mca_base_component_t struct containing meta
       information about the component itself */

    {
        MCA_SCON_BASE_VERSION_2_0_0,

        "native", /* MCA component name */
        ORTE_MAJOR_VERSION,  /* MCA component major version */
        ORTE_MINOR_VERSION,  /* MCA component minor version */
        ORTE_RELEASE_VERSION,  /* MCA component release version */
        NULL,
        NULL,
    },
    scon_native_start,
    scon_native_shutdown,
    scon_native_get_module
};

scon_native_module_t scon_native_module = {
    {
        scon_native_init,
        scon_native_create,
        scon_native_getinfo,
        scon_native_send_nb,
        scon_native_recv_nb,
        scon_native_recv_cancel,
        scon_native_xcast,
        scon_native_barrier,
        scon_native_delete,
        scon_native_finalize
    }
};

static int scon_native_start()
{
    return SCON_SUCCESS;
}

static void scon_native_shutdown()
{
    return;
}

static scon_module_t* scon_native_get_module()
{
    return &scon_native_module.super;
}

static bool progress_thread_running = false;
/**** SCON NATIVE APIS *****/
/**
 * native (orte) init
 */
int scon_native_init(scon_info_t info[], size_t ninfo) {
    int ret ;
    uint32_t u32, i, *u32ptr;
    uint16_t u16, *u16ptr;
    char *error = NULL;
    scon_topo_type_t topo;
    /* Process info keys */
    for(i = 0; i< ninfo; i++) {
        /* for now we only support topo keys so return error if
           anyother key is specified */
        if(0 == strncmp(info[i].key, SCON_TOPO_TYPE, SCON_MAX_KEYLEN)) {
            topo = info[i].value.data.uint32;
        }
        else {
            /* not supported info key */
            return SCON_ERR_INFO_KEY_UNSUPPORTED;
        }
    }
    opal_output_verbose(1, orte_scon_base_framework.framework_output,
                        "%s scon_native_init - opening required frameworks",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    /****  TO DO  *****
     * Need to process all the info keys and open only requested frameworks and
     * return error if requested modules are not available
     **** END TO DO ****/
    /* set up the env and frameworks for SCON operation */
    /* set up proc info */
    if (ORTE_SUCCESS != (ret = orte_proc_info())) {
        opal_output(0, "scon_init failed : error = orte_proc_info");
        error = "orte_proc_info";
        goto error;
    }

    /** SETUP the env ourselves here without using ess framework */
    /* init data type support */
    if (SCON_SUCCESS != (ret = orte_dt_init())) {
        error = "orte_dt_init";
        goto error;
    }

    /*
     * Initialize the event library
     */
    if (OPAL_SUCCESS != (ret = mca_base_framework_open(&opal_event_base_framework, 0))) {
        error = "opal_event_base_open";
        goto error;
    }

    /*
     * Initialize the general progress engine
     */
    if (OPAL_SUCCESS != (ret = opal_progress_init())) {
        error = "opal_progress_init";
        goto error;
    }
    /* get an async event base - we use the opal_async one so
     * we don't startup extra threads if not needed */
    orte_event_base = opal_progress_thread_init(NULL);
    progress_thread_running = true;
    /* init pmix */
    if (NULL == opal_pmix.initialized) {
        /* open and setup pmix */
        if (OPAL_SUCCESS != (ret = mca_base_framework_open(&opal_pmix_base_framework, 0))) {
            ORTE_ERROR_LOG(ret);
            error = "pmix_open";
            goto error;
        }
        if (OPAL_SUCCESS != (ret = opal_pmix_base_select())) {
            /* don't error log this as it might not be an error at all */
            ORTE_ERROR_LOG(ret);
            error = "pmix_select";
            goto error;
        }
    }
   /* set the opal event base */
    opal_pmix_base_set_evbase(orte_event_base);
    if (!opal_pmix.initialized() && (OPAL_SUCCESS != (ret = opal_pmix.init()))) {
        /* we cannot run */
        ORTE_ERROR_LOG(ret);
        error = "pmix init";
        goto error;
    }
    u32ptr = &u32;
    u16ptr = &u16;

    /****   THE FOLLOWING ARE REQUIRED VALUES   ***/
    /* pmix.init set our process name down in the OPAL layer,
     * so carry it forward here */
    ORTE_PROC_MY_NAME->jobid = OPAL_PROC_MY_NAME.jobid;
    ORTE_PROC_MY_NAME->vpid = OPAL_PROC_MY_NAME.vpid;

    /* get our local rank from PMI */
    OPAL_MODEX_RECV_VALUE(ret, OPAL_PMIX_LOCAL_RANK,
                          ORTE_PROC_MY_NAME, &u16ptr, OPAL_UINT16);
    if (OPAL_SUCCESS != ret) {
        error = "getting local rank";
        goto error;
    }
    orte_process_info.my_local_rank = u16;

    /* get our node rank from PMI */
    OPAL_MODEX_RECV_VALUE(ret, OPAL_PMIX_NODE_RANK,
                          ORTE_PROC_MY_NAME, &u16ptr, OPAL_UINT16);
    if (OPAL_SUCCESS != ret) {
        error = "getting node rank";
        goto error;
    }
    orte_process_info.my_node_rank = u16;

    /* get universe size */
    OPAL_MODEX_RECV_VALUE(ret, OPAL_PMIX_JOB_SIZE,
                          ORTE_PROC_MY_NAME, &u32ptr, OPAL_UINT32);
    if (OPAL_SUCCESS != ret) {
        error = "getting univ size";
        goto error;
    }
    orte_process_info.num_procs = u32;

    orte_process_info.super.proc_name = *(opal_process_name_t*)ORTE_PROC_MY_NAME;
    orte_process_info.super.proc_hostname = orte_process_info.nodename;
    orte_process_info.super.proc_flags = OPAL_PROC_ALL_LOCAL;
    orte_process_info.super.proc_arch = opal_local_arch;
    opal_proc_local_set(&orte_process_info.super);

    /* set ourselves up as a SCON process */
    orte_process_info.proc_type = ORTE_PROC_SCON;

    /* open and setup the state machine */
    if (ORTE_SUCCESS != (ret = mca_base_framework_open(&orte_state_base_framework, 0))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_state_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_state_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_state_base_select";
        goto error;
    }
    /* open the errmgr */
    if (ORTE_SUCCESS != (ret = mca_base_framework_open(&orte_errmgr_base_framework, 0))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_errmgr_base_open";
        goto error;
    }
    /* Setup the communication infrastructure */
    if (ORTE_SUCCESS != (ret = mca_base_framework_open(&orte_oob_base_framework, 0))) {
        ORTE_ERROR_LOG(ret);
        opal_output(0, "error opening oob");
        error = "orte_oob_base_open";
        goto error;
    }
    /****  TO DO  *****
     we need to select the oob/transport during create
     **** END TO DO ****/
    if (ORTE_SUCCESS != (ret = orte_oob_base_select())) {
        ORTE_ERROR_LOG(ret);
        opal_output(0, "error selecting oob ret =%d", ret);
        error = "orte_oob_base_select";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = mca_base_framework_open(&orte_rml_base_framework, 0))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_rml_base_open";
        goto error;
    }
    /****  TO DO  *****
     * we need to select during create when the app specifies the required transports
     **** END TO DO ****/
    if (ORTE_SUCCESS != (ret = orte_rml_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_rml_base_select";
        goto error;
    }
    /****  TO DO  *****
     * we need a errmgr module specific to SCON, we will use the app errmgr for now
     **** END TO DO ****/
    /* setup the errmgr */
    if (ORTE_SUCCESS != (ret = orte_errmgr_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_errmgr_base_select";
        goto error;
    }
    /* Routed system */
    if (ORTE_SUCCESS != (ret = mca_base_framework_open(&orte_routed_base_framework, 0))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_routed_base_open";
        goto error;
    }
   /****  TO DO  *****
     * we need to select during create when the app specifies the required topology
     * we may even need multiple routed components to be active
     **** END TO DO ***/
    if (ORTE_SUCCESS != (ret = orte_routed_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_routed_base_select";
        goto error;
    }
    /*
     * Group communications
     */
    if (ORTE_SUCCESS != (ret = mca_base_framework_open(&orte_grpcomm_base_framework, 0))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_grpcomm_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_grpcomm_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_grpcomm_base_select";
        goto error;
    }
    /* enable communication with the rml */
    if (ORTE_SUCCESS != (ret = orte_rml.enable_comm())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_rml.enable_comm";
        goto error;
    }
    /****  TO DO  *****
     * we may not need to set these fields for SCON
     **** END TO DO ****/
    opal_process_info.job_session_dir  = orte_process_info.job_session_dir;
    opal_process_info.proc_session_dir = orte_process_info.proc_session_dir;
    opal_process_info.num_local_peers  = (int32_t)orte_process_info.num_local_peers;
    opal_process_info.my_local_rank    = (int32_t)orte_process_info.my_local_rank;
    opal_process_info.cpuset           = orte_process_info.cpuset;
error:
    if(OPAL_SUCCESS != ret) {
        opal_output(0, "SCON Init failed error =%s, ret =%d", error, ret);
        return SCON_ERROR;
    }
    return ret;
}

/**
 * native create
 */
int scon_native_create (scon_proc_t procs[],
                        size_t nprocs,
                        scon_info_t info[],
                        size_t ninfo,
                        scon_create_cbfunc_t cbfunc,
                        void *cbdata)
{
    orte_scon_scon_t *scon;
    orte_scon_send_req_t *req;
    /* perform basic input validation and create a scon object*/
    if( NULL == (scon = orte_scon_base_create(procs, nprocs,
                                         info, ninfo, cbfunc, cbdata)))
    {
        return SCON_HANDLE_INVALID;
    }
    else {
       /* create a req object and do the rest of the processing in
         an event */
        opal_output_verbose (1, orte_scon_base_framework.framework_output,
                            "%s scon_native_create creating scon %d",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             scon->handle);
        req = OBJ_NEW(orte_scon_send_req_t);
        req->post.create.scon = scon;
        req->post.create.cbfunc = cbfunc;
        req->post.create.cbdata = cbdata;
        scon->req = req;
        /* setup the event for rest of the processing  */
        opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, orte_scon_native_process_create, req);
        opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
        opal_event_active(&req->ev, OPAL_EV_WRITE, 1);
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                             "%s scon_create_channel_req - set event done",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        return scon->handle;
    }
}



int scon_native_getinfo ( scon_handle_t scon_handle,
                          scon_info_t info[],
                          size_t *ninfo)
{
    return SCON_ERROR;
}

int scon_native_send_nb (scon_handle_t scon_handle,
                          scon_proc_t *peer,
                          scon_buffer_t *buf,
                          scon_msg_tag_t tag,
                          scon_send_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo)
{
    orte_scon_scon_t *scon;
    orte_scon_send_req_t *req;
    /* get the scon object*/
    if( NULL == (scon = (orte_scon_scon_t*) opal_pointer_array_get_item(&orte_scon_base.scons,
                        get_index(scon_handle))))
    {
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* create a send req and do the rest of the processing in
          an event */
        req = OBJ_NEW(orte_scon_send_req_t);
        req->post.send.scon = scon;
        req->post.send.buf = buf;
        req->post.send.tag = tag;
        /* copy the dst process */
        strncpy(req->post.send.dst.job_name, peer->job_name, SCON_MAX_JOBLEN);
        req->post.send.dst.rank = peer->rank;
        req->post.send.cbfunc = cbfunc;
        req->post.send.cbdata = cbdata;
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                    "%s scon_native_send scon %d, dest job %s",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                    scon->handle, (char*)  req->post.send.dst.job_name);
        /* setup the event for rest of the processing  */
        opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, orte_scon_native_process_send, req);
        opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
        opal_event_active(&req->ev, OPAL_EV_WRITE, 1);
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                             "%s scon_native_send_nb - set event done",
                              ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        return SCON_SUCCESS;
    }
}

/**
 * native recv
 */
int scon_native_recv_nb (scon_handle_t scon_handle,
                         scon_proc_t *peer,
                         scon_msg_tag_t tag,
                         bool persistent,
                         scon_recv_cbfunc_t cbfunc,
                         void *cbdata,
                         scon_info_t info[],
                         size_t ninfo)
{
    orte_scon_scon_t *scon;
    orte_scon_recv_req_t *req;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "%s scon_native_recv_nb recv posted message on scon %d tag %d to rank %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         scon_handle, tag, peer->rank);
    /* get the scon object*/
    if( NULL == (scon = (orte_scon_scon_t*) opal_pointer_array_get_item(&orte_scon_base.scons,
                        get_index(scon_handle))))
    {
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* now push the request into the event base so we can add
         * the receive to our list of posted recvs */
        req = OBJ_NEW(orte_scon_recv_req_t);
        req->post = OBJ_NEW(orte_scon_posted_recv_t);
        req->post->scon_handle = scon_handle;
        opal_convert_string_to_jobid(&req->post->peer.jobid, peer->job_name);
        req->post->peer.vpid = peer->rank;
        req->post->tag = tag;
        req->post->persistent = persistent;
        req->post->cbfunc = cbfunc;
        req->post->cbdata = cbdata;
        opal_event_set(orte_event_base, &req->ev, -1,
                       OPAL_EV_WRITE,
                       orte_scon_base_post_recv, req);
        opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
        opal_event_active(&req->ev, OPAL_EV_WRITE, 1);

    }
    return SCON_SUCCESS;
}

/**
 * native recv cancel
 */
int scon_native_recv_cancel (scon_handle_t scon_handle,
                             scon_proc_t *peer,
                             scon_msg_tag_t tag)
{
    /***** TO DO *****
    * need to implement recv cancel
    * END TO DO *****/
    return SCON_ERROR;
}
/**
 * native_xcast
 */
int scon_native_xcast (scon_handle_t scon_handle,
                       scon_proc_t procs[],
                       size_t nprocs,
                       scon_buffer_t *buf,
                       scon_msg_tag_t tag,
                       scon_xcast_cbfunc_t cbfunc,
                       void *cbdata,
                       scon_info_t info[],
                       size_t ninfo)
{
    orte_scon_scon_t *scon;
    orte_scon_send_req_t *req;
    int i;
    opal_output_verbose(5, orte_scon_base_framework.framework_output ,
                        "xcast request on scon %d", scon_handle);
    /* perform basic input validation*/
    if( NULL == (scon = (orte_scon_scon_t*) opal_pointer_array_get_item(&orte_scon_base.scons,
                        get_index(scon_handle))))
    {
        opal_output(0, "%s scon_native_xcast invalid scon handle=%d",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), scon_handle);
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* create a xcast req and do the rest of the processing in
          an event */
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                            "%s scon_native_xcast scon %d to procs %d",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                            scon->handle,
                            (int) nprocs);
        req = OBJ_NEW(orte_scon_send_req_t);
        req->post.xcast.scon = scon;
        req->post.xcast.buf = buf;
        req->post.xcast.tag = tag;
        req->post.xcast.cbfunc = cbfunc;
        req->post.xcast.cbdata = cbdata;
        req->post.xcast.nprocs = nprocs;
        /* check if this is a broadcast to all members of scon */
        if(NULL == procs) {
            /* we don't need to populate the proc array of the request
             as we can get that info from the scon object */
           /*  opal_output(0, "%s scon_native_xcast scon %d received broadcast",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         scon->handle);*/
        }
        else {
            SCON_PROC_CREATE(req->post.xcast.procs, nprocs);
            /* store the procs in the proc array */
            for(i = 0; i< (int) nprocs; i++) {
                strncpy(req->post.xcast.procs[i].job_name, procs[i].job_name,
                        SCON_MAX_JOBLEN);
                req->post.xcast.procs[i].rank = procs[i].rank;
            }
        }
        /* setup the event for rest of the processing  */
        opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, orte_scon_native_process_xcast, req);
        opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
        opal_event_active(&req->ev, OPAL_EV_WRITE, 1);
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                             "%s scon_native_xcast - set event done",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    }
    return SCON_SUCCESS;
}

/**
 * native barrier
 */
int scon_native_barrier (scon_handle_t scon_handle,
                         scon_proc_t procs[],
                         size_t nprocs,
                         scon_barrier_cbfunc_t cbfunc,
                         void *cbdata,
                         scon_info_t info[],
                         size_t ninfo)
{
    orte_scon_scon_t *scon;
    orte_scon_send_req_t *req;
    int i;
    /*verify valid scon*/
    if( NULL == (scon = (orte_scon_scon_t*) opal_pointer_array_get_item(&orte_scon_base.scons,
                        get_index(scon_handle))))
    {
        opal_output(0, "%s scon_native_barrier invalid scon handle=%d",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), scon_handle);
        return SCON_ERR_NOT_FOUND;
    }
    /* create a barrier req and do the rest of the processing in
       an event */
    opal_output_verbose(1, orte_scon_base_framework.framework_output,
                        "%s scon_native_barrier scon %d to procs %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         scon->handle,
                         (int)nprocs);
    req = OBJ_NEW(orte_scon_send_req_t);
    req->post.barrier.scon = scon;
    req->post.barrier.cbfunc = cbfunc;
    req->post.barrier.cbdata = cbdata;
    req->post.barrier.nprocs = nprocs;
    /* check if this is a broadcast to all members of scon */
    if(NULL != procs) {
        SCON_PROC_CREATE(req->post.barrier.procs, nprocs);
        /* store the procs in the proc array */
        for(i = 0; i< (int) nprocs; i++) {
            strncpy(req->post.barrier.procs[i].job_name, procs[i].job_name,
                        SCON_MAX_JOBLEN);
            req->post.barrier.procs[i].rank = procs[i].rank;

        }
    }
    /* setup the event for rest of the processing  */
    opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, orte_scon_native_process_barrier, req);
    opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
    opal_event_active(&req->ev, OPAL_EV_WRITE, 1);
    opal_output_verbose(10, orte_scon_base_framework.framework_output,
                             "%s scon_native_barrier - set event done",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    return SCON_SUCCESS;
}
/**
 * native delete
 */
int scon_native_delete (scon_handle_t scon_handle,
                        scon_op_cbfunc_t cbfunc,
                        void *cbdata,
                        scon_info_t info[],
                        size_t ninfo)
{
    opal_output_verbose(1, orte_scon_base_framework.framework_output,
                        "scon_native_delete deleting scon %d" , scon_handle);
    orte_scon_scon_t *scon;
    orte_scon_send_req_t *req;
    /* perform basic input validation */
    if( NULL == (scon = (orte_scon_scon_t*) opal_pointer_array_get_item(&orte_scon_base.scons,
                          get_index(scon_handle))))
    {
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* create a req object and do the rest of the processing in
          an event */
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                            "%s scon_native_delete deleting scon %d",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                            scon->handle);
        req = OBJ_NEW(orte_scon_send_req_t);
        req->post.teardown.scon = scon;
        req->post.teardown.cbfunc = cbfunc;
        req->post.teardown.cbdata = cbdata;
        scon->req = req;
        /* setup the event for rest of the processing  */
        opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, orte_scon_native_process_delete, req);
        opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
        opal_event_active(&req->ev, OPAL_EV_WRITE, 1);
    }
    return SCON_SUCCESS;
}

/**
 * native_finalize
 */
int scon_native_finalize()
{
    /* close frameworks */
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                         "%s scon_native_finalize - finalizing",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    opal_output(0, "%s scon_native_finalize - finalizing",
                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    mca_base_framework_close(&orte_grpcomm_base_framework);
    mca_base_framework_close(&orte_routed_base_framework);
    mca_base_framework_close(&orte_rml_base_framework);
    mca_base_framework_close(&orte_oob_base_framework);
    mca_base_framework_close(&orte_state_base_framework);
    mca_base_framework_close(&orte_errmgr_base_framework);
    /* mark us as finalized */
    if (NULL != opal_pmix.finalize) {
        opal_pmix.finalize();
    }
    mca_base_framework_close(&opal_pmix_base_framework);
    /*release the event base*/
    if (progress_thread_running) {
        opal_progress_thread_finalize(NULL);
        progress_thread_running = false;
    }
    mca_base_framework_close(&opal_event_base_framework);
    return SCON_SUCCESS;
}
