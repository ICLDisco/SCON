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
#include "scon_types.h"

#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/runtime/scon_progress_threads.h"
#include "src/class/scon_hash_table.h"
#include "src/buffer_ops/buffer_ops.h"
#include "src/buffer_ops/types.h"
#include "src/mca/pt2pt/base/base.h"
#include "src/mca/if/base/base.h"
#include "src/mca/topology/base/base.h"
#include "src/mca/collectives/base/base.h"
#include "src/mca/comm/comm.h"
#include "src/include/scon_globals.h"
#include "src/util/scon_pmix.h"
#include "src/buffer_ops/types.h"


static int native_open(void);
static int native_close(void);
static int native_query(scon_mca_base_module_t **module, int *priority);
static scon_comm_module_t* comm_native_get_module();
static int native_getinfo ( scon_handle_t scon_handle,
                          scon_info_t info[],
                          size_t *ninfo);
static int native_init(scon_info_t info[], size_t ninfo) ;
static int native_create ( scon_proc_t procs[],
                         size_t nprocs,
                         scon_info_t info[],
                         size_t ninfo,
                         scon_create_cbfunc_t cbfunc,
                         void *cbdata);
static int native_finalize();
static int native_delete (scon_handle_t scon_handle,
                        scon_op_cbfunc_t cbfunc,
                        void *cbdata,
                        scon_info_t info[],
                        size_t ninfo);

/**
 * component definition
 */
SCON_EXPORT scon_comm_base_component_t mca_comm_native_component = {
    /* First, the mca_base_component_t struct containing meta
       information about the component itself */
    .base =
    {
        SCON_COMM_BASE_VERSION_1_0_0,
        .scon_mca_component_name = "native",
         /* Component name and version */
         SCON_MCA_BASE_MAKE_VERSION(component, SCON_MAJOR_VERSION,
                                SCON_MINOR_VERSION,
                               SCON_RELEASE_VERSION),
         /* Component functions */
         .scon_mca_open_component = native_open,
         .scon_mca_close_component = native_close,
         .scon_mca_query_component = native_query,
    },
    .priority = 80,
    .get_module = comm_native_get_module
};

scon_comm_module_t native_module = {
    native_init,
    native_create,
    native_getinfo,
    native_delete,
    native_finalize
};

static int native_open()
{
    return SCON_SUCCESS;
}

static int native_close()
{
    return SCON_SUCCESS;
}

static scon_comm_module_t* comm_native_get_module()
{
    return &native_module;
}

static int native_query(scon_mca_base_module_t **module, int *priority)
{
    /* The priority value is somewhat meaningless here; by
       there's at most one component
       available. */
    *priority = mca_comm_native_component.priority;
    *module = (scon_mca_base_module_t*)&native_module;
     return SCON_SUCCESS;
}

static void native_create_xcast_send_complete (scon_status_t status,
                                             scon_handle_t scon_handle,
                                             scon_proc_t procs[],
                                             size_t nprocs,
                                             scon_buffer_t *buf,
                                             scon_msg_tag_t tag,
                                             scon_info_t info[],
                                             size_t ninfo,
                                             void *cbdata)
{
    scon_req_t *req = (scon_req_t *) cbdata;
    /* Make sure that the xcast was sent successfully to all participants */
    if (SCON_SUCCESS != status)
    {
        scon_output(0, "native_create_xcast_send_complete: scon master %s xcast failed with status=%d",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), status);
        /** To do: determine appropriate error action **/
        SCON_ERROR_LOG(SCON_ERR_CREATE_XCAST_SEND_FAIL);
    }
    /* complete request and release the req object*/
  req->post.create.cbfunc(status, scon_handle,req->post.create.cbdata);
    if(NULL != buf)
        scon_buffer_destruct(buf);
    if((NULL != procs) && nprocs != 0)
        SCON_PROC_FREE(procs, nprocs);
    if((NULL != info) && ninfo != 0)
        SCON_INFO_FREE(info, ninfo);
    SCON_RELEASE(req);

}

static void native_create_cfg_recv_cbfunc(scon_status_t status,
                                        scon_handle_t scon_handle,
                                        scon_proc_t *peer,
                                        scon_buffer_t *buf,
                                        scon_msg_tag_t tag,
                                        void *cbdata)
{
    scon_req_t *req = (scon_req_t *) cbdata;
    if(SCON_SUCCESS == status) {
        /* set the config info received from the master and complete req*/
        scon_output_verbose (1,  scon_comm_base_framework.framework_output,
                             "%s native_create_cfg_recv_cbfunc for scon %d status = %d ",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon_handle, status);
        //comm_base_set_scon_config (scon_comm_base_get_scon (scon_handle), buf);
        req->post.create.cbfunc(status, scon_handle,req->post.create.cbdata);
        if(NULL != buf) {
            scon_buffer_destruct(buf);
        }
        SCON_RELEASE(req);
    }
    /*** TO DO: implement error behavior ***/
}
static void native_create_barrier_complete_callback (scon_status_t status,
                                                   scon_handle_t scon_handle,
                                                   scon_proc_t procs[],
                                                   size_t nprocs,
                                                   scon_buffer_t *buffer,
                                                   scon_info_t info[],
                                                   size_t ninfo,
                                                   void *cbdata)
{
    scon_req_t *req = (scon_req_t *) cbdata;
    scon_buffer_t *buf = malloc(sizeof(scon_buffer_t));
    scon_buffer_construct(buf);
    scon_comm_scon_t *scon = scon_comm_base_get_scon(scon_handle);
    scon_output_verbose (1,  scon_comm_base_framework.framework_output,
                           "%s native_create_barrier_complete_callback on scon %d status = %d ",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon_handle, status);
    if(SCON_SUCCESS == status) {
        /** TO DO : The returned buffer will have  the contact information
           of all the peers, which can then be processed to set the member uris
           TO DO : process member uris and local SCON IDs from the returned all gather buffer
         **/
        /** Master xcasts  config info to all the participants and completes create req */
        scon_buffer_destruct(buffer);

        if(is_master(scon)) {
            /*  prepare the xcast msg and complete create in xcast send callback */
            comm_base_pack_scon_config(scon, buf);
            collectives_base_api_xcast( scon_handle,
                                        NULL, 0,
                                        buf,
                                        SCON_MSG_TAG_CFG_INFO,
                                        native_create_xcast_send_complete,
                                        req, NULL , 0);
        }

        /* wait for config info xcast msg from master */
        pt2pt_base_api_recv_nb (scon_handle,
                                SCON_PROC_WILDCARD,
                                SCON_MSG_TAG_CFG_INFO,
                                SCON_MSG_PERSISTENT,
                                native_create_cfg_recv_cbfunc,
                                req, NULL, 0);

    }
    else {
        scon_output(0, "%s native_create_barrier_complete_callback failed with status =%d",
                     SCON_PRINT_PROC(SCON_PROC_MY_NAME), status);
        /* delete the scon and complete the create request with error */
        scon_comm_base_remove_scon(scon);
        SCON_RELEASE(scon);
        /* call the create complete callback with error info */
        req->post.create.cbfunc(status, SCON_HANDLE_INVALID, req->post.create.cbdata);
        SCON_RELEASE(req);
    }

}

/*** Create Request Processing
 ****** TO DO: We need to fix the default create behavior. For the first version
 * we default to the debug behavior. So the create operation has
 * atleast 2 additional xcasts to verify configuration and to distribute the
 * final configuration.
 * (1) First optimization is to skip config verify operation unless the info key
 * is specified, this eliminates the need for additional xcasts and barrier.
 * (2) Further down the road we can completely eliminate the initial allgather and
 * exchange the scon ids on demand.
 ****** End To Do */

/** The create process
* (1) all members participate in an initial barrier, at this point the end point addresses
* are exchanged. We can eliminate this step in future, and use PMIx get instead
* to send msgs to our peers.
* (2) Master sends config verify xcast message
* (3) All members verify that their local scon configuration matches with that of the master
*     and send their response - ie config check successful or fail and their local scon id.
* (4) master ensures all members have verified config successfully, compiles the scon ids
*     and distributes them in a final xcast
* (5) master completes the create request at its end
* (6) The member processes store the final scon ids and complete the create request*/
static void native_process_create (int fd, short flags, void *cbdata)
{
    int ret;
    scon_comm_scon_t *scon1;
    scon_comm_scon_t *scon = (scon_comm_scon_t *) cbdata;
    scon_req_t *req = (scon_req_t*)scon->req;
    scon_buffer_t *allgather_buf;
    //scon_comm_scon_t *scon = scon_comm_base_get_scon(req->post.create.scon_handle);
    /* check the scon type fail if not single job all ranks */
    if( SCON_TYPE_MY_JOB_ALL != scon->type) {
        scon_output(0, "cannot support partial topology at this time");
        SCON_ERROR_LOG(SCON_ERR_CONFIG_NOT_SUPPORTED);
        ret = SCON_ERR_CONFIG_NOT_SUPPORTED;
        goto error;
    }
    /* Now that we are in an event it is safe to add
     * the scon to the global scon array */
   /* index = scon_pointer_array_add (&comm_base.scons, scon);
    scon->handle = (index + 1) % MAX_SCONS;*/
    scon_comm_base_add_scon(scon);
    /* Get our address and publish it via PMIX */
    scon1 = scon_comm_base_get_scon(scon->handle);
    scon_pt2pt_base_get_contact_info(&scon_globals.my_uri);
    if(SCON_SUCCESS != (ret = scon_pmix_put_string(SCON_PMIX_PROC_URI, scon_globals.my_uri))) {
        scon_output(0, "%s PMIX put of proc uri %s failed ",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon_globals.my_uri);
    }
 /*   scon_value_load(&val, (void*)scon_globals.my_uri,
                   SCON_STRING);*/

    /* setup the topology for the selected topology module**/
    if((NULL == scon->topology_module)) {
        ret = SCON_ERR_TOPO_UNSUPPORTED;
        scon_output( 0, "scon topology module not set correctly");
        SCON_ERROR_LOG(ret);
        goto error;
    }
    scon->topology_module->api.initialize(&scon->topology_module->topology);

    scon->topology_module->api.update_topology (&scon->topology_module->topology,
                                                scon->nmembers);
    allgather_buf = (scon_buffer_t*) malloc (sizeof(scon_buffer_t));
    scon_buffer_construct(allgather_buf);
    if (SCON_SUCCESS != (ret = scon_bfrop.pack(allgather_buf, &scon->handle,
                             1, SCON_UINT32))) {
        SCON_ERROR_LOG(ret);
        goto error;
    }
    scon->collective_module->init(scon->handle);
    /* now wait for all scon participants to get here */
    /*** TO DO **** - fence or allgather */
    collectives_base_api_allgather( scon->handle,
                                    NULL, 0,
                                    allgather_buf, native_create_barrier_complete_callback,
                                    req,
                                    NULL, 0);
    return;
error:
    /* fail the create request back to the caller */
    scon_comm_base_remove_scon(scon);
    SCON_RELEASE(scon);
    req->post.create.cbfunc(ret, SCON_HANDLE_INVALID, req->post.create.cbdata);
    SCON_RELEASE(req);

}


/* delete completion fn - after fence release */
static void native_process_delete_barrier_cbfunc (scon_status_t status,
                                                scon_handle_t scon_handle,
                                                scon_proc_t procs[],
                                                size_t nprocs,
                                                scon_info_t info[],
                                                size_t ninfo,
                                                void *cbdata)
{
    scon_output_verbose(1,  scon_comm_base_framework.framework_output,
                        "%s native_process_delete_barrier_cbfunc on scon %d ",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon_handle);
    scon_req_t *req = (scon_req_t*) cbdata;
    scon_comm_scon_t *scon = scon_comm_base_get_scon(req->post.teardown.scon_handle);
    scon_comm_base_remove_scon(scon);
    /* TO DO: need to call finalize on all the modules associated
       with this scon */
    SCON_RELEASE(scon);
    req->post.teardown.cbfunc(SCON_SUCCESS, req->post.teardown.cbdata);
    SCON_RELEASE(req);
}

/** Delete is synchronized among members by default
  ***** TO DO
  (1) Make the final allgather optional.
  (2) Check info keys for silent deletion
  END TO DO *****
*/
static void native_process_delete (int fd, short flags, void *cbdata)
{
    scon_req_t *req = (scon_req_t*) cbdata;
    scon_comm_scon_t *scon = scon_comm_base_get_scon(req->post.teardown.scon_handle);
    scon->state = SCON_STATE_DELETING;
    collectives_base_api_barrier(scon->handle,
                                 NULL,
                                 0,
                                 native_process_delete_barrier_cbfunc,
                                 req,
                                 NULL, 0);
    scon_output_verbose(5, scon_comm_base_framework.framework_output,
                        "%s native_process_delete: delete barrier called for scon %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon->handle);
    return;
}

/**
 * native init
 */
static int native_init(scon_info_t info[], size_t ninfo) {
    int ret, i ;
    char *error = NULL;
    char topo_mod_list[SCON_MAX_STRING_VALUELEN];
    char coll_mod_list[SCON_MAX_STRING_VALUELEN];
    char pt2pt_mod_list[SCON_MAX_SEL_STRING_VALUELEN];
    /* Process info keys */
    /****  TO DO  *****
    * Need to process all the info keys and open only requested frameworks and
    * return error if requested modules are not available
    **** END TO DO ****/
    for(i = 0; i< (int) ninfo; i++) {
        /* for now we only support topo keys so return error if
           anyother key is specified */
        if(0 == strncmp(info[i].key, SCON_TOPO_MOD_LIST, SCON_MAX_KEYLEN)) {
            strncpy(topo_mod_list, info[i].value.data.string, SCON_MAX_STRING_VALUELEN);
            continue;
        }
        if (0 == strncmp(info[i].key, SCON_COLL_MOD_LIST, SCON_MAX_KEYLEN)) {
            strncpy(coll_mod_list, info[i].value.data.string, SCON_MAX_STRING_VALUELEN);
            continue;
        }
        if (0 == strncmp(info[i].key, SCON_PT2PT_MOD_LIST, SCON_MAX_KEYLEN)) {
            strncpy(pt2pt_mod_list, info[i].value.data.string, SCON_MAX_SEL_STRING_VALUELEN);
            continue;
        }
    }
    scon_output_verbose(5, scon_comm_base_framework.framework_output,
                        "comm_native_init - opening required frameworks");
    if (SCON_SUCCESS != (ret = scon_mca_base_framework_open(&scon_if_base_framework, 0))) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init failed : error = if_framework_open");
        error = "if_framework_open";
        goto return_error;
    }
    if (SCON_SUCCESS != (ret = scon_mca_base_framework_open(&scon_pt2pt_base_framework, 0))) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init failed : error = pt2pt_framework_open");
        error = "pt2pt_framework_open";
        goto return_error;
    }

    if (SCON_SUCCESS != (ret = scon_mca_base_framework_open(&scon_topology_base_framework, 0))) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init failed : error = topology_framework_open");
        error = "topology_framework_open";
        goto return_error;
    }
    if (SCON_SUCCESS != (ret = scon_mca_base_framework_open(&scon_collectives_base_framework, 0))) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init failed : error = collectives_framework_open");
        error = "collectives_framework_open";
        goto return_error;
    }
    /** the pt2pt framework is a special case, where we call select to initialize our address **/
    if (SCON_SUCCESS != (ret = scon_pt2pt_base_select())) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init failed : error = pt2pt_framework_select");
        error = "pt2pt_framework_select";
        goto return_error;
    }
    /** initialize PMIx client
    * lets treat it as a fatal error for now */
    if(SCON_SUCCESS != (ret = scon_pmix_init(scon_globals.myid, NULL, 0))) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init pmix_init failed with error =%d", ret);
        error = "pmix_init";
        goto return_error;
    }
return_error:
    if(SCON_SUCCESS != ret) {
        scon_output_verbose(0, scon_comm_base_framework.framework_output,
                            "comm_native_init Init failed error =%s, ret =%d", error, ret);
        return SCON_ERROR;
    }
    return ret;
}

/**
 * native create
 */
static int native_create ( scon_proc_t procs[],
                  size_t nprocs,
                  scon_info_t info[],
                  size_t ninfo,
                  scon_create_cbfunc_t cbfunc,
                  void *cbdata)
{
    scon_comm_scon_t *scon = NULL;
    scon_req_t *req;
    /* create a req object and do the rest of the processing in
        an event */
    scon_output_verbose(0, scon_comm_base_framework.framework_output,
                        "%s scon_native_create creating scon",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));
    req = SCON_NEW(scon_req_t);
    req->post.create.cbfunc = cbfunc;
    req->post.create.cbdata = cbdata;
    req->post.create.info = info;
    req->post.create.ninfo = ninfo;
    /* perform basic input validation and create scon locally*/
    if( NULL == (scon = comm_base_create(procs, nprocs,
                                         info, ninfo, req)))
    {
        SCON_RELEASE(req);
        return SCON_HANDLE_INVALID;
    }
    else {
        scon->req = req;
        /* setup the event for rest of the processing  */
        scon_event_set(scon_globals.evbase, &req->ev, -1, SCON_EV_WRITE, native_process_create, scon);
        scon_event_set_priority(&req->ev, SCON_MSG_PRI);
        scon_event_active(&req->ev, SCON_EV_WRITE, 1);
        scon_output_verbose(2, scon_comm_base_framework.framework_output,
                            "%s scon_native_create created req %p",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME), (void*)req );
    }
    return SCON_HANDLE_NEW;
}



static int native_getinfo ( scon_handle_t scon_handle,
                   scon_info_t info[],
                   size_t *ninfo)
{
    return SCON_ERROR;
}

/**
 * native delete
 */
static int native_delete (scon_handle_t scon_handle,
                 scon_op_cbfunc_t cbfunc,
                 void *cbdata,
                 scon_info_t info[],
                 size_t ninfo)
{
    scon_output_verbose(0, scon_comm_base_framework.framework_output,
                         "%s scon_native_delete deleting scon %d",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon_handle);
    scon_comm_scon_t *scon;
    scon_req_t *req;
    /* perform basic input validation */
    if( NULL == (scon = scon_comm_base_get_scon(scon_handle)))
    {
        return SCON_ERR_NOT_FOUND;
    }
    else {
        /* create a req object and do the rest of the processing in
          an event */
        scon_output_verbose(0, scon_comm_base_framework.framework_output,
                             "%s scon_native_delete found scon %d deleting",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon_handle);
        req = SCON_NEW(scon_req_t);
        req->post.teardown.scon_handle = scon->handle;
        req->post.teardown.cbfunc = cbfunc;
        req->post.teardown.cbdata = cbdata;
        scon->req = req;
        /* setup the event for rest of the processing  */
        scon_event_set(scon_globals.evbase, &req->ev, -1, SCON_EV_WRITE, native_process_delete, req);
        scon_event_set_priority(&req->ev, SCON_MSG_PRI);
        scon_event_active(&req->ev, SCON_EV_WRITE, 1);
    }
    return SCON_SUCCESS;
}

/**
 * native_finalize
 */
static int native_finalize()
{
    /* close frameworks */
    scon_output_verbose(0, scon_comm_base_framework.framework_output,
                         "%s scon_native_finalize - finalizing",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME));
    scon_mca_base_framework_close(&scon_if_base_framework);
    scon_mca_base_framework_close(&scon_topology_base_framework);
    scon_mca_base_framework_close(&scon_collectives_base_framework);
    scon_mca_base_framework_close(&scon_pt2pt_base_framework);
    return SCON_SUCCESS;
}


