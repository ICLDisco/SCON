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

#include "orte/mca/grpcomm/base/base.h"
#include "orte/mca/rml/base/base.h"
#include "orte/mca/routed/base/base.h"
#include "orte/mca/scon/base/base.h"
#include "orte/mca/scon/scon.h"
#include "orte/mca/scon/native/scon_native.h"


static void orte_scon_native_complete_create_request (orte_scon_create_t *req);

/* internal send callback for handling internal send completions
 * from rml */
static void orte_scon_native_send_callback (int status,
                                            orte_process_name_t *peer,
                                            opal_buffer_t* buffer,
                                            orte_rml_tag_t tag,
                                            void* cbdata)
{
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                         "%s orte_scon_native_send_callback sent msg status = %d"
                         "on scon %d to peer %s \n",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         status,
                         req->post.create.scon->handle,
                         ORTE_NAME_PRINT(peer));
    OBJ_RELEASE(buffer);
    if (ORTE_SUCCESS != status) {
        /* TO DO: retry or error */
        ORTE_ERROR_LOG(status);
    }
}
/**
 * scon master process recv handler for config check msg,  because
 * the master process also receives this message.
 */
static void orte_scon_native_ignore_config_check (int status,
                                                  orte_process_name_t *peer,
                                                  opal_buffer_t* buffer,
                                                  orte_rml_tag_t tag,
                                                  void* cbdata)
{
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                         "%s orte_scon_native_ignore_config_check ignoring my own broadcast msg on scon %d \n",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         req->post.create.scon->handle);
}

/**
 * scon master process create completion handler
 */
static void orte_scon_native_master_create_complete (int status,
                                                  orte_process_name_t *peer,
                                                  opal_buffer_t* buffer,
                                                  orte_rml_tag_t tag,
                                                  void* cbdata)
{
    orte_scon_send_req_t *req = NULL;
    orte_scon_create_t *create_req;
    req = (orte_scon_send_req_t *) cbdata;
    create_req = &req->post.create;
    create_req->status = SCON_SUCCESS;
    orte_scon_native_complete_create_request(create_req);
    OBJ_RELEASE(req);
}

/** create request completion function*/
static void orte_scon_native_complete_create_request (orte_scon_create_t *create_req)
{
     scon_proc_t scon_proc_wildcard;
    /* if request is successful- notify user and return,
       if request failed notify user, delete scon and return */
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "%s orte_scon_native_complete_create_request"
                         "completing create scon req status =%d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         create_req->status);
    if(SCON_SUCCESS == create_req->status) {
        create_req->cbfunc(create_req->status, create_req->scon->handle,
                           create_req->cbdata);
        /* create a wildcard process */
        scon_native_convert_proc_name_orte_to_scon(ORTE_NAME_WILDCARD, &scon_proc_wildcard);
        /* we need to post a rcv for a scon xcast relay message tag
         so we can recv scon xcasts */
        scon_native_recv_nb(create_req->scon->handle,
                            &scon_proc_wildcard,
                            ORTE_RML_TAG_SCON_XCAST,
                            true,
                            scon_xcast_recv, NULL, NULL, 0);
    } else {
        create_req->cbfunc(create_req->status, SCON_HANDLE_INVALID,
                           create_req->cbdata);
        opal_pointer_array_set_item(&orte_scon_base.scons, get_index(create_req->scon->handle), NULL);
        OBJ_RELEASE(create_req->scon);
    }
}

/**
 * The master process config check reply message handler.
 * The master process, ensures that each member has passed config check
 * and compiles the scon ids  from all processes and
 * distributes the scon ids among the members in
 * another xcast operation.
 */
static void orte_scon_native_master_recv_config_verify_reply (int status,
                                                    orte_process_name_t *peer,
                                                    opal_buffer_t* buffer,
                                                    orte_rml_tag_t tag,
                                                    void* cbdata)
{
    int n, rc;
    orte_scon_scon_t *scon;
    scon_handle_t handle;
    int response;
    orte_scon_member_t *mem;
    orte_grpcomm_signature_t *sig;
    opal_buffer_t *send_msg;
    orte_scon_send_req_t *req = NULL;
    orte_scon_create_t *create_req;
    /* unpack my (master) scon handle */
    n = 1;
    if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &handle, &n, OPAL_INT32))) {
        ORTE_ERROR_LOG(SCON_ERR_SILENT);
        return;
    }
    /* retrieve scon */
    if(NULL == (scon = (orte_scon_scon_t*) opal_pointer_array_get_item(
                           &orte_scon_base.scons, get_index(handle)))) {
        ORTE_ERROR_LOG(SCON_ERR_NOT_FOUND);
        return;
    }
    req = (orte_scon_send_req_t *) scon->req;
    create_req = &req->post.create;
    /* unpack response */
    n = 1;
    if(ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &response, &n, OPAL_INT32)))
        goto error;
    if (ORTE_SUCCESS != response)
        goto fail_request;
    /* unpack local id */
    n = 1;
    if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &handle, &n, OPAL_INT32)))
        goto error;
    /* look up the member & store the local id*/
    OPAL_LIST_FOREACH(mem, &scon->members, orte_scon_member_t) {
        if((peer->jobid == mem->name.jobid) &&
                (peer->vpid == mem->name.vpid)) {
            opal_output_verbose(5, orte_scon_base_framework.framework_output,
                      "set local handle for jobid %d rank %d handle =%d",
                       peer->jobid, peer->vpid, handle);
            mem->local_handle = handle;
            scon->num_replied++;
            break;
        }
    }
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                      "orte_scon_native_process_config_reply,"
                      "response=%d members = %d num_replied =%d",
                       response, scon->num_procs, scon->num_replied);
    /* check if all procs have replied */
    if(scon->num_replied == scon->num_procs - 1) {
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                            "orte_scon_native_process_config_reply,"
                            "scon consistent configuration verified"
                            "distributing sconids");
        /* cancel config reply recv and send create complete msg
        * and complete the request locally */
        orte_rml.recv_cancel(ORTE_NAME_WILDCARD,
                             ORTE_RML_TAG_SCON_CONFIG_CHECK_RESP);
        /* pack status, num members, each member proc and its scon id  */
        send_msg = OBJ_NEW(opal_buffer_t);
        if (ORTE_SUCCESS != (rc = opal_dss.pack(send_msg, &response, 1, OPAL_INT32))) {
            OBJ_RELEASE(send_msg);
            goto error;
        }
        if (ORTE_SUCCESS != (rc = opal_dss.pack(send_msg, &scon->num_procs, 1, OPAL_UINT32))) {
            OBJ_RELEASE(send_msg);
            goto error;
        }
        OPAL_LIST_FOREACH(mem, &scon->members, orte_scon_member_t) {
            if(ORTE_SUCCESS != (rc = opal_dss.pack(send_msg, &mem->name, 1, ORTE_NAME))) {
                OBJ_RELEASE(send_msg);
                goto error;
            }
            /* we need to set our own master handle */
            if((!mem->local_handle) && (mem->name.vpid == ORTE_PROC_MY_NAME->vpid))
                mem->local_handle = scon->handle;
            if(ORTE_SUCCESS != (rc = opal_dss.pack(send_msg, &mem->local_handle, 1, OPAL_INT32))) {
                OBJ_RELEASE(send_msg);
                goto error;
            }
        }
        sig = OBJ_NEW(orte_grpcomm_signature_t);
        sig->signature = (orte_process_name_t*)malloc(sizeof(orte_process_name_t));
        sig->signature[0].jobid = ORTE_PROC_MY_NAME->jobid;
        sig->signature[0].vpid = ORTE_VPID_WILDCARD;
        sig->sz = 1;
        orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, ORTE_RML_TAG_SCON_CREATE_COMPLETE,
                                false, orte_scon_native_master_create_complete,
                                req);
        orte_grpcomm.xcast(sig, ORTE_RML_TAG_SCON_CREATE_COMPLETE,
                           send_msg);
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                            " orte_scon_native_process_config_reply,"
                            "xcast sent for distributing scon ids");
    }
    return;
fail_request:
    /* we get here when we get an error response from one of
       the members, so we don't process the subsequent responses from
       other members, instead we will broadcast the fail msg
       and complete the request locally */
    opal_output(0, "orte_scon_native_process_config_reply : failing"
                    "scon create request because of config verify fail");
    send_msg = OBJ_NEW(opal_buffer_t);
    if (ORTE_SUCCESS != (rc = opal_dss.pack(send_msg, &response, 1, OPAL_INT32))) {
        OBJ_RELEASE(send_msg);
        goto error;
    }
    sig = OBJ_NEW(orte_grpcomm_signature_t);
    sig->signature = (orte_process_name_t*)malloc(sizeof(orte_process_name_t));
    sig->signature[0].jobid = ORTE_PROC_MY_NAME->jobid;
    sig->signature[0].vpid = ORTE_VPID_WILDCARD;
    if (ORTE_SUCCESS != (rc = orte_grpcomm.xcast(sig, ORTE_RML_TAG_SCON_CREATE_COMPLETE,
                              send_msg))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(send_msg);
        OBJ_RELEASE(sig);
        goto error;
    }
    /* now complete the request locally */
    create_req->status = SCON_ERR_CONFIG_MISMATCH;
    orte_scon_native_complete_create_request(create_req);
    OBJ_RELEASE(req);
    return;
error:
    /* something went wrong - lets fail the scon create request for now
       TO DO : we need to revisit this becoz all errors may not
       need this big hammer and we also need to address notifying other
       procs */
    orte_rml.recv_cancel(ORTE_NAME_WILDCARD,
                         ORTE_RML_TAG_SCON_CONFIG_CHECK_RESP);
    create_req->status = SCON_ERR_SILENT;
    opal_output(0, "orte_scon_native_process_config_reply : failing"
                "scon create request because of internal error %d",
                 rc);
    orte_scon_native_complete_create_request(create_req);
    OBJ_RELEASE(req);
}

/* Member process recv handler for recv the final SCON configuration
 * ie the sconids and endpoint info from the master process.
 * The sent config info is stored in the scon object and the create
 * request is completed.
 */
static void orte_scon_native_recv_final_config (int status,
                                               orte_process_name_t *peer,
                                               opal_buffer_t* buffer,
                                               orte_rml_tag_t tag,
                                               void* cbdata)
{
    int rc, req_status, n;
    orte_scon_send_req_t *req;
    uint32_t num_procs, i;
    orte_process_name_t proc;
    scon_handle_t local_handle;
    orte_scon_member_t *mem;
    /* check status and store scon ids if success and
      complete create request */
    n = 1;
    if( NULL == cbdata)
        return;
    req = (orte_scon_send_req_t *) cbdata;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "orte_scon_native_recv_final_config req scon =%d",
                         req->post.create.scon->handle);
    if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &req_status, &n, OPAL_INT32)))
        goto error;
    /* set req status */
    req->post.create.status = req_status;
    if (SCON_SUCCESS == req->post.create.status) {
        /* unpack ids and store them locally */
        n = 1;
        if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &num_procs, &n, OPAL_UINT32)))
            goto error;
        for (i = 0; i < num_procs; i++) {
            n = 1;
            if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &proc, &n, ORTE_NAME)))
                goto error;
            n = 1;
            if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &local_handle, &n, OPAL_INT32)))
                goto error;
            /* search thru the member array and store the handle */
            OPAL_LIST_FOREACH(mem, &req->post.create.scon->members,
                              orte_scon_member_t) {
                if((proc.jobid == mem->name.jobid) &&
                    (proc.vpid == mem->name.vpid)) {
                    mem->local_handle = local_handle;
                    break;
                }
            }
        }
    }
    orte_scon_native_complete_create_request(&req->post.create);
    OBJ_RELEASE(req);
    return;
error:
    /* we get here when we hit an error locally,
      we will fail the request, but this is a local failure
      and the calling process will need to handle it. */
    opal_output(0, "%s failing scon create req because of unpack error %d",
                  ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                  rc);
    req->post.create.status = SCON_ERR_LOCAL;
    orte_scon_native_complete_create_request(&req->post.create);
    OBJ_RELEASE(req);
}

/*  member process (non master) config check msg recv handler
*/
static void orte_scon_native_recv_config_check (int status,
                                     orte_process_name_t *peer,
                                     opal_buffer_t* buffer,
                                     orte_rml_tag_t tag,
                                     void* cbdata)
{
    int rc, response, n;
    orte_scon_send_req_t *req;
    orte_scon_scon_t *scon;
    opal_buffer_t *reply;
    scon_handle_t master_handle = SCON_HANDLE_INVALID;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "orte_scon_native_recv_config_check");
    if ((NULL == cbdata) || (NULL == ((orte_scon_send_req_t *) cbdata)->post.create.scon)) {
        opal_output(0, "orte_scon_native_recv_config_check  error cbdata is %p", cbdata);
        ORTE_ERROR_LOG(SCON_ERR_NOT_FOUND);
        return;
    }
    req = (orte_scon_send_req_t *) cbdata;
    scon = req->post.create.scon;

    reply = OBJ_NEW(opal_buffer_t);
    if((SCON_STATE_CREATING != scon->state) &&
            (SCON_STATE_VERIFYING != scon->state))  {
        response = SCON_ERR_NOT_FOUND;
    }
    else if (SCON_SUCCESS == (rc = orte_scon_base_check_config (scon, buffer))) {
        response = SCON_SUCCESS;
        scon->state = SCON_STATE_WIRING_UP;
    } else {
        response = SCON_ERR_CONFIG_MISMATCH;
        scon->state = SCON_STATE_ERROR;
        /* TO DO: may be we can locally fail the create request here */
    }
    /* send reply
       reply contains the incoming scon id, config status & the local scon id */
    /* unpack incoming scon id */
    n = 1;
    orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, ORTE_RML_TAG_SCON_CREATE_COMPLETE,
                            false, orte_scon_native_recv_final_config,
                            req);
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "orte_scon_recv_process_config_check response =%d", response);
    if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &master_handle, &n, OPAL_INT32)))
        goto error;
    /* now pack the reply msg */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(reply, &master_handle, 1, OPAL_INT32)))
        goto error;
    if (ORTE_SUCCESS != (rc = opal_dss.pack(reply, &response, 1, OPAL_INT32)))
        goto error;
    if (ORTE_SUCCESS != (rc = opal_dss.pack(reply, &scon->handle, 1, OPAL_INT32)))
        goto error;
    if (ORTE_SUCCESS != (rc = orte_rml.send_buffer_nb (SCON_GET_MASTER(scon), reply,
                                                       ORTE_RML_TAG_SCON_CONFIG_CHECK_RESP,
                                                       orte_scon_native_send_callback,
                                                       req))) {
        goto error;

    }
    /* post recv for create complete msg from master */

    return;
error:
    orte_rml.recv_cancel(ORTE_NAME_WILDCARD, ORTE_RML_TAG_SCON_CREATE_COMPLETE);
    opal_output( 0, "orte_scon_recv_process_config_check error = %d", rc);
    scon->state = SCON_STATE_ERROR;
    ORTE_ERROR_LOG(rc);
    OBJ_RELEASE (reply);
    req->post.create.status = SCON_ERR_CONFIG_MISMATCH;
    orte_scon_native_complete_create_request(&req->post.create);
    OBJ_RELEASE (req);
    return;
}

/**
 * Scon verification process: The master process xcasts the config check msg
 * while other members wait for that msg from the master
 */
static void orte_scon_native_scon_verify (int fd, short flags, void *cbdata)
{
    opal_buffer_t *buf;
    int rc;
    orte_grpcomm_signature_t *sig;
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    orte_scon_scon_t *scon = req->post.create.scon;

    if(orte_scon_is_master(scon))
    {
        /*send check config request to all members */
        buf = OBJ_NEW(opal_buffer_t);
        if(SCON_SUCCESS != (rc = orte_scon_base_pack_scon_config(scon, buf))) {
            opal_output(0, " %s orte_scon_base_pack_scon_config returned error = %d",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), rc);
            ORTE_ERROR_LOG(rc);
            OBJ_RELEASE(buf);
            goto error;
        }
        scon->num_replied = 0;
        sig = OBJ_NEW(orte_grpcomm_signature_t);
        sig->signature = (orte_process_name_t*)malloc(sizeof(orte_process_name_t));
        sig->signature[0].jobid = ORTE_PROC_MY_NAME->jobid;
        sig->signature[0].vpid = ORTE_VPID_WILDCARD;
        sig->sz = 1;
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                            " %s sending scon config check xcast scon handle = %d",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), scon->handle);

        orte_grpcomm.xcast(sig, ORTE_RML_TAG_SCON_CONFIG_CHECK, buf);

        OBJ_RELEASE(buf);
        OBJ_RELEASE(sig);
        /* since this is a broadcast, we will also recv this message, post a recv and ignore it */
        orte_rml.recv_buffer_nb (ORTE_PROC_MY_NAME, ORTE_RML_TAG_SCON_CONFIG_CHECK,
                                 ORTE_RML_PERSISTENT, orte_scon_native_ignore_config_check, req);
        /* post recv for reply*/
        orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, ORTE_RML_TAG_SCON_CONFIG_CHECK_RESP,
                                ORTE_RML_PERSISTENT, orte_scon_native_master_recv_config_verify_reply,
                                NULL);
    } else {
        /* wait for config verify from the master */
        orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, ORTE_RML_TAG_SCON_CONFIG_CHECK,
                                false, orte_scon_native_recv_config_check,
                                req);
        /* TO DO need to have a timer here */

    }
    return;
error:
    opal_pointer_array_set_item(&orte_scon_base.scons, get_index(scon->handle), NULL);
    OBJ_RELEASE(scon);
    req->post.create.cbfunc(rc, SCON_HANDLE_INVALID, req->post.create.cbdata);
    OBJ_RELEASE(req);
}

static void orte_scon_native_create_fence_cbfunc (int status, void *cbdata)
{
    /* we are here implies all scon participants have called scon_create
     * and have published the oob contact info, the next step is to
     * agree on the scon configuration and complete the create
     * request */
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    OBJ_RETAIN(req);
    orte_scon_scon_t *scon = req->post.create.scon;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                 " %s orte_scon_native_create_fence_cbfunc scon handle = %d members = %d",
                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), scon->handle, scon->num_procs);
    opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, orte_scon_native_scon_verify, req);
    opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
    opal_event_active(&req->ev, OPAL_EV_WRITE, 1);
}

/*** Create Request Processing
 ****** TO DO: We need to fix the default create behavior. For the first version
 * we adopted the debug behavior as default. So the create operation has
 * atleast 2 additional xcasts to verify configuration and to distribute the
 * final configuration.
 * (1) First optimization is to skip config verify operation unless the info key
 * is specified, this eliminates the need for additional xcasts and fence.
 * (2) Further down the road we can completely eliminate the initial allgather and
 * exchange the scon ids on demand.
 ****** End To Do */

 /** The create process
* (1) all members participate in an initial fence, at this point the end point addresses
* are exchanged via modex. We can eliminate this step in future, and use PMIx get instead
* to send msgs to our peers.
* (2) Master sends config verify xcast message
* (3) All members verify their local scon configuration with that of the master and
*     send their response - ie config check successful or fail and their local scon id.
* (4) master ensures all members have verified config successfully, compiles the scon ids
*     and distributes them in a final xcast
* (5) master completes the create request at its end
* (6) The member processes store the final scon ids and complete the create request*/
void orte_scon_native_process_create (int fd, short flags, void *cbdata)
{
    int ret;
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    orte_scon_scon_t *scon = req->post.create.scon;
    orte_proc_t *proc;
    orte_job_t *jdata;
    orte_app_context_t *app;
    /* check the scon type fail if not single job all ranks */
    if( SCON_TYPE_MY_JOB_ALL != scon->type) {
        opal_output(0, "cannot support partial topology at this time");
        ORTE_ERROR_LOG(SCON_ERR_TYPE_UNSUPPORTED);
        ret = SCON_ERR_TYPE_UNSUPPORTED;
        goto error;
    }
    /*setup the job level info for routed & grpcomm */
    /* setup the global job array */
    orte_job_data = OBJ_NEW(opal_hash_table_t);
    if (ORTE_SUCCESS != (ret = opal_hash_table_init(orte_job_data, 128))) {
        ORTE_ERROR_LOG(ret);
        ret = SCON_ERR_SILENT;
        goto error;
    }
    /* Setup the job data object for the daemons */
    /* create and store the job data object */
    jdata = OBJ_NEW(orte_job_t);
    jdata->jobid = ORTE_PROC_MY_NAME->jobid;
    opal_hash_table_set_value_uint32(orte_job_data, jdata->jobid, jdata);
    /* every job requires at least one app */
    app = OBJ_NEW(orte_app_context_t);
    opal_pointer_array_set_item(jdata->apps, 0, app);
    jdata->num_apps++;
    /* create and store a proc object for us */
    proc = OBJ_NEW(orte_proc_t);
    proc->name.jobid = ORTE_PROC_MY_NAME->jobid;
    proc->name.vpid = ORTE_PROC_MY_NAME->vpid;
    proc->pid = orte_process_info.pid;
    proc->rml_uri = orte_rml.get_contact_info();
    proc->state = ORTE_PROC_STATE_RUNNING;
    opal_pointer_array_set_item(jdata->procs, proc->name.vpid, proc);
    OBJ_RETAIN(proc);   /* keep accounting straight */
    /* setup the routed info - the selected routed component
     * will know what to do.
     */
    /** TO DO set orte_routed to the topo key **/
    orte_routed.update_routing_plan();
    if (ORTE_SUCCESS != (ret = orte_routed.init_routes(ORTE_PROC_MY_NAME->jobid, NULL))) {
        ORTE_ERROR_LOG(ret);
        ret = SCON_ERR_TOPO_UNSUPPORTED;
        goto error;
    }
    /** next step is exchanging contact information **/
    OPAL_MODEX_SEND_VALUE(ret, OPAL_PMIX_GLOBAL,
                          OPAL_PMIX_PROC_URI,
                          orte_rml.get_contact_info(), OPAL_STRING);
    /* set the hnp to the master node to
       ensure that collectives go to master as root of topology*/
    orte_process_info.my_hnp = scon->master;
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "%s set hnp to master node %s",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                        ORTE_NAME_PRINT(ORTE_PROC_MY_HNP));
    opal_pmix.commit();
    /* now wait for all scon participants to get here */
    opal_pmix.fence_nb(&scon->members, false, orte_scon_native_create_fence_cbfunc, req);
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                        "%s orte_scon_native_process_create fence called",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    return;
error:
    /* fail the create request back to the caller */
    opal_pointer_array_set_item(&orte_scon_base.scons, get_index(scon->handle), NULL);
    OBJ_RELEASE(scon);
    req->post.create.cbfunc(ret, SCON_HANDLE_INVALID, req->post.create.cbdata);
    OBJ_RELEASE(req);
}


/* delete completion fn - after fence release */
static void orte_scon_native_delete_fence_cbfunc (int status, void *cbdata)
{
    opal_output_verbose(1, orte_scon_base_framework.framework_output,
                        "%s orte_scon_native_delete_fence_cbfunc cbdata = %p",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), cbdata);
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    orte_scon_scon_t *scon = req->post.teardown.scon;
    opal_pointer_array_set_item(&orte_scon_base.scons, get_index(scon->handle), NULL);
    OBJ_RELEASE(scon);
    req->post.teardown.cbfunc(SCON_SUCCESS, req->post.teardown.cbdata);
    OBJ_RELEASE(req);
}

/** Delete is synchronized among members by default
  ***** TO DO : (1) replace pmix fence with direct all gather request
  (2) Make the final fence optional.
  (3) Check info keys for silent deletion
  END TO DO *****
*/
void orte_scon_native_process_delete (int fd, short flags, void *cbdata)
{
    orte_scon_send_req_t *req = (orte_scon_send_req_t*) cbdata;
    orte_scon_scon_t *scon = req->post.teardown.scon;
    scon->state = SCON_STATE_DELETING;
    opal_pmix.fence_nb(&scon->members, false, orte_scon_native_delete_fence_cbfunc, req);
    opal_output_verbose(1, orte_scon_base_framework.framework_output,
                        "%s orte_scon_native_process_delete fence called scon %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), scon->handle);
    return;
}

/* utility functions */

void scon_native_convert_proc_name_orte_to_scon(orte_process_name_t *orte_proc,
                                                scon_proc_t *scon_proc)
{
    opal_snprintf_jobid(scon_proc->job_name, SCON_MAX_JOBLEN, orte_proc->jobid);
    scon_proc->rank =  orte_proc->vpid;
}
