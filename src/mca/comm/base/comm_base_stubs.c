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
#include "scon_globals.h"
#include "src/buffer_ops/buffer_ops.h"
#include "src/util/error.h"
#include "src/mca/comm/base/base.h"
#include "src/util/name_fns.h"
#include "src/mca/pt2pt/base/base.h"
#include "src/mca/collectives/base/base.h"
#include "src/mca/topology/base/base.h"

SCON_EXPORT scon_comm_scon_t * comm_base_create (scon_proc_t procs[],
                                         size_t nprocs,
                                         scon_info_t info[],
                                         size_t ninfo,
                                         scon_req_t *req)
{
    unsigned int i;
    unsigned long sz;
    scon_member_t *mem;
    scon_proc_t *master = NULL;
    char topo[SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN+1];
    char pt2pt_comp[SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN+1];
    char coll_comp[SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN+1];
    uint16_t *temp;
    uint32_t *temp1;
    /* create the local scon object and set its attributes*/
    scon_comm_scon_t *scon = SCON_NEW(scon_comm_scon_t);
    /* process the info array and error if any of the required keys
      or values cannot be supported */
    if(NULL != info && 0 != ninfo) {
        scon_output_verbose (2,  scon_comm_base_framework.framework_output,
                             "%s comm_base_create processing %lu info keys for create req",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME), ninfo);
        for (i = 0; i < ninfo; i++) {
            if(0 == strncmp(info[i].key, SCON_TOPO_TYPE, SCON_MAX_KEYLEN)) {
                /* check if we can support the topo */
                strncpy(topo, info[i].value.data.string, SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN);
                continue;
            }
            /* check for master proc key */
            if(0 == strncmp(info[i].key, SCON_MASTER_PROC, SCON_MAX_KEYLEN)) {
                /* retrieve the master process, the default is rank 0*/
                scon_value_unload(&info[i].value, (void**)&master, &sz, SCON_PROC);
                continue;
            }
            /* check for scon type info key*/
            if (0 == strncmp(info[i].key, SCON_RECV_QUEUE_LENGTH, SCON_MAX_KEYLEN)) {
                /* copy the value and store it */
                scon_value_unload(&info[i].value, (void**)&temp, &sz, SCON_UINT16);
                scon->recv_queue_len = *temp;
                continue;
            }
            if (0 == strncmp(info[i].key, SCON_OPERATIONAL_QUORUM, SCON_MAX_KEYLEN)) {
                /* copy the value and store it */
                scon_value_unload(&info[i].value, (void**)&temp1, &sz, SCON_UINT32);
                req->post.create.quorum = *temp1;
                continue;
            }
            if (0 == strncmp(info[i].key, SCON_CREATE_TIMEOUT, SCON_MAX_KEYLEN)) {
                /* copy the value and store it */
                scon_value_unload(&info[i].value, (void**)&temp, &sz, SCON_UINT16);
                req->post.create.timeout = *temp;
                continue;
            }
        }
    }
    if (NULL == master) {
        /* set default master proc as rank 0 */
        strncpy(scon->master.job_name, SCON_PROC_MY_NAME->job_name,SCON_MAX_JOBLEN);
        scon->master.rank = 0;
    } else {
        strncpy(scon->master.job_name, master->job_name, SCON_MAX_JOBLEN);
        scon->master.rank = master->rank;
    }
    /* populate member info */
    if (NULL == procs) {
         scon->nmembers = SCON_JOB_SIZE;
       /* populate the member list ourselves - we can remove this
         code if we don't use this at all */
        for (i =0; i< scon->nmembers; i++) {
            mem = SCON_NEW(scon_member_t);
            strncpy(mem->name.job_name, SCON_PROC_MY_NAME->job_name, SCON_MAX_JOBLEN);
            mem->name.rank = i;
            scon_list_append(&scon->members, &mem->super);
        }
        /* set type */
        scon->type = SCON_TYPE_MY_JOB_ALL;
    } else {
        /* parse array and add only requested job members */
        /* assume my job but partial scon */
        scon->type = SCON_TYPE_MY_JOB_PARTIAL;
        scon->nmembers = nprocs;
        /* convert the array of pmix_proc_t to the list of procs */
        for (i=0; i < nprocs; i++) {
            mem = SCON_NEW(scon_member_t);
            memcpy(&mem->name, &procs[i], sizeof(scon_proc_t));
            if (SCON_RANK_WILDCARD == procs[i].rank) {
               SCON_ERROR_LOG(SCON_ERR_WILD_CARD_NOT_SUPPORTED);
               goto error;
            }
            scon_list_append(&scon->members, &mem->super);
            if(0 != strncmp(SCON_PROC_MY_NAME->job_name, mem->name.job_name, SCON_MAX_JOBLEN))
            {
                /* multi job scon set type and fail req for now */
                scon->type = SCON_TYPE_MULTI_JOB;
                SCON_ERROR_LOG(SCON_ERR_MULTI_JOB_NOT_SUPPORTED);
                goto error;
            }
        }
    }
    /* Now assign the requested pt2pt, topology and collectives modules */
    /** user can only select the topology module at this pt in time */
    strcpy(pt2pt_comp, SCON_PT2PT_DEFAULT_COMPONENT);
    if (NULL == (scon->pt2pt_module = scon_pt2pt_base_get_module(pt2pt_comp))) {
        SCON_ERROR_LOG(SCON_ERR_SILENT);
        scon_output(0, "%s comm_base_create: fatal error cannot get default pt2pt module %s" ,
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), pt2pt_comp);
        goto error;
    }
    strcpy(coll_comp, SCON_COLLECTIVES_DEFAULT_COMPONENT);
    if (NULL == (scon->collective_module = scon_collectives_base_get_module(coll_comp))) {
        SCON_ERROR_LOG(SCON_ERR_SILENT);
        scon_output(0, "%s comm_base_create: fatal error cannot get default collectives module %s" ,
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), coll_comp);
        goto error;
    }
    strcpy(topo, SCON_TOPOLOGY_DEFAULT_COMPONENT);
    if (NULL == (scon->topology_module = scon_topology_base_get_module(topo))) {
        SCON_ERROR_LOG(SCON_ERR_SILENT);
        scon_output(0, "%s comm_base_create: fatal error cannot get selected topo module %s" ,
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), topo);
        goto error;
    }
    /* copy all the infos for now */
    SCON_INFO_CREATE(scon->info, ninfo);
    for (i = 0; i < ninfo; i++) {
        scon->info[i] = info[i];
    }
    return scon;
error:
    SCON_RELEASE(scon);
    return NULL;
}


scon_status_t comm_base_pack_scon_config(scon_comm_scon_t *scon,
                                              scon_buffer_t *buffer)
{
    int rc;
    size_t i;
    scon_member_t *mem;
    /* pack the following config info
     * (1) scon attributes
     * (2) scon type
     * (3) scon members if type != SINGLE_JOB_ALL
     * (4) scon id */
    /* pack num infos */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(buffer,
                              &scon->ninfo, 1, SCON_UINT32))) {
        SCON_ERROR_LOG(rc);
        return rc;
    }
    /* pack attributes */
    for(i = 0; i < scon->ninfo; i++)  {
        scon_output_verbose (2,  scon_comm_base_framework.framework_output,
                            "%s comm_base_pack_scon_config packing attribute = %s\n",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                             scon->info[i].key);
        if (SCON_SUCCESS != (rc = scon_bfrop.pack(buffer, (void*)&scon->info[i], 1, SCON_INFO))) {
            SCON_ERROR_LOG(rc);
            return rc;
        }
    }
    /* pack type */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(buffer, &scon->type,
                                             1, SCON_UINT8))) {
        SCON_ERROR_LOG(rc);
        return rc;
    }
    /* pack members if necessary */
    if (SCON_TYPE_MY_JOB_ALL != scon->type) {
        scon_output_verbose (2,  scon_comm_base_framework.framework_output,
                             "%s comm_base_pack_scon_config packing members type = %d\n",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                             scon->type);
        SCON_LIST_FOREACH(mem, &scon->members, scon_member_t) {
            if (SCON_SUCCESS != (rc = scon_bfrop.pack(buffer, (void*)&mem->name, 1, SCON_PROC))) {
                SCON_ERROR_LOG(rc);
                return rc;
            }
        }
    }
    /* pack local id*/
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(buffer, &scon->handle, 1, SCON_INT32))) {
        SCON_ERROR_LOG(rc);
        return rc;
    }
    return SCON_SUCCESS;
}

scon_status_t comm_base_check_config (scon_comm_scon_t *scon,
                                           scon_buffer_t *buffer)
{
    int rc = SCON_SUCCESS;
    int  n , k, i;
    size_t count;
    scon_info_t *info;
    bool match;
    scon_type_t type;
    count = 0;
    n = 1;
    if (SCON_SUCCESS != (rc = scon_bfrop.unpack(buffer, &count,
                                              &n, SCON_UINT32))) {
        SCON_ERROR_LOG(rc);
        return rc;
    }
    scon_output_verbose (2,  scon_comm_base_framework.framework_output,
                         "%s comm_base_check_config num attributes = %lu",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         count);
    SCON_INFO_CREATE(info, count);
    for (k=0; k < (int)count; k++) {
        n=1;
        if (SCON_SUCCESS != (rc = scon_bfrop.unpack(buffer, &info[k],
                                  &n, SCON_INFO)))
        {
            SCON_INFO_FREE(info, count);
            SCON_ERROR_LOG(rc);
            return rc;
        }

        match = false;
        /* now compare this against the local attributes */
        for(i = 0; i < (int) scon->ninfo; i++)
        {
            if((0 == strncmp(info[k].key, scon->info[i].key, SCON_MAX_KEYLEN)) &&
               (scon_value_cmp(&info[k].value, &scon->info[i].value))) {
                match = true;
                break;
            }
        }
        SCON_INFO_FREE(info, count);
        if (!match) {
            SCON_ERROR_LOG(rc);
            return SCON_ERR_CONFIG_MISMATCH;
        }

    }
    n = 1;
    if (SCON_SUCCESS == (rc = scon_bfrop.unpack(buffer, &type, &n, SCON_UINT8))) {
        if (type != scon->type)
           return SCON_ERR_CONFIG_MISMATCH;
    }
    return rc;
}


scon_handle_t scon_base_get_handle(scon_comm_scon_t *scon,
                                   scon_proc_t *member)
{
    scon_member_t *mem;
    /* return local scon id of the member */
    SCON_LIST_FOREACH(mem, &scon->members, scon_member_t) {
        if(SCON_EQUAL == scon_util_compare_name_fields(SCON_NS_CMP_ALL, &mem->name,
                                                       member)) {
            return mem->local_handle;
        }
    }
    return SCON_HANDLE_INVALID;
}

scon_status_t comm_base_set_scon_config(scon_comm_scon_t *scon,
                                          scon_buffer_t *buffer)
{
    return SCON_ERR_NOT_IMPLEMENTED;
}

