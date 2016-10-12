/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "scon_config.h"
//#include "src/util/constants.h"


#include "src/mca/mca.h"
//#include "src/util/output.h"
#include "src/mca/base/base.h"
//#include "src/mca/pmix/pmix.h"
//#include "src/mca/pmix/base/base.h"
//#include "opal/util/arch.h" (need to delete all commented out includes)
#include "src/runtime/scon_progress_threads.h"
#include "src/class/scon_hash_table.h"

#include "src/mca/common/base/base.h"
#include "src/mca/common/common.h"
#include "src/mca/common/orte/common_orte.h"
//#include "src/runtime/dt.h"
//include "src/runtime/scon_globals.h"
//#include "src/util/proc_info.h"
//#include "orte/util/proc_info.h"
//#include "orte/util/session_dir.h"
#include "src/mca/pt2pt/pt2pt.h"
//#include "src/mca/topology/base/base.h"
#include "src/mca/topology/topology.h"
//#include "src/mca/pt2pt/base/base.h"
#include "src/mca/collectives/collectives.h"
//#include "src/mca/collectives/base/base.h"
//#include "src/mca/errmgr/errmgr.h"
//#include "src/mca/dfs/base/base.h"
//#include "src/mca/errmgr/base/base.h"
//#include "orte/mca/state/base/base.h"

static int common_orte_open(void);
static void common_orte_close(void);
static scon_common_module_t* common_orte_get_module();

/**
 * component definition
 */
scon_common_base_component_t scon_mca_common_orte_component = {
    /* First, the mca_base_component_t struct containing meta
       information about the component itself */
    .base =
    {
        SCON_COMMON_BASE_VERSION_1_0_0,
        .scon_mca_component_name = "orte",
         /* Component name and version */
         SCON_MCA_BASE_MAKE_VERSION(component, SCON_MAJOR_VERSION,
                                SCON_MINOR_VERSION,
                               SCON_RELEASE_VERSION),
         /* Component functions */
         .scon_mca_open_component = common_orte_open,
         .scon_mca_close_component = common_orte_close,

    },
    .priority = 80,
    .get_module = common_orte_get_module
};

scon_common_orte_module_t common_orte_module = {
    {
        common_orte_init,
        common_orte_create,
        common_orte_getinfo,
        common_orte_delete,
        common_orte_finalize
    }
};

static int common_orte_open()
{
    return SCON_SUCCESS;
}

static void common_orte_close()
{
    return;
}

static scon_common_module_t* common_orte_get_module()
{
    return &common_orte_module.super;
}

static bool progress_thread_running = false;
/**** SCON NATIVE APIS *****/
/**
 * orte (orte) init
 */
int common_orte_init(scon_info_t info[], size_t ninfo) {
#if 0
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
                        "%s scon_orte_init - opening required frameworks",
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
#endif
    return SCON_SUCCESS;
}

/**
 * orte create
 */
int common_orte_create (scon_proc_t procs[],
                        size_t nprocs,
                        scon_info_t info[],
                        size_t ninfo,
                        scon_create_cbfunc_t cbfunc,
                        void *cbdata)
{
#if 0
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
                            "%s scon_orte_create creating scon %d",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             scon->handle);
        req = OBJ_NEW(orte_scon_send_req_t);
        req->post.create.scon = scon;
        req->post.create.cbfunc = cbfunc;
        req->post.create.cbdata = cbdata;
        scon->req = req;
        /* setup the event for rest of the processing  */
        opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, orte_scon_orte_process_create, req);
        opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
        opal_event_active(&req->ev, OPAL_EV_WRITE, 1);
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                             "%s scon_create_channel_req - set event done",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        return scon->handle;
    }
#endif
    return 0;
}



int common_orte_getinfo ( scon_handle_t scon_handle,
                          scon_info_t info[],
                          size_t *ninfo)
{
    return SCON_ERROR;
}

/**
 * orte delete
 */
int common_orte_delete (scon_handle_t scon_handle,
                        scon_op_cbfunc_t cbfunc,
                        void *cbdata,
                        scon_info_t info[],
                        size_t ninfo)
{
#if 0
    opal_output_verbose(1, orte_scon_base_framework.framework_output,
                        "scon_orte_delete deleting scon %d" , scon_handle);
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
                            "%s scon_orte_delete deleting scon %d",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                            scon->handle);
        req = OBJ_NEW(orte_scon_send_req_t);
        req->post.teardown.scon = scon;
        req->post.teardown.cbfunc = cbfunc;
        req->post.teardown.cbdata = cbdata;
        scon->req = req;
        /* setup the event for rest of the processing  */
        opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, orte_scon_orte_process_delete, req);
        opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
        opal_event_active(&req->ev, OPAL_EV_WRITE, 1);
    }
#endif
    return SCON_SUCCESS;
}

/**
 * orte_finalize
 */
int scon_orte_finalize()
{
#if 0
    /* close frameworks */
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                         "%s scon_orte_finalize - finalizing",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    opal_output(0, "%s scon_orte_finalize - finalizing",
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
#endif
    return SCON_SUCCESS;
}
