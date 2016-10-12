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
#include "util/error.h"
#include "src/mca/common/base/base.h"
//#include "util/util/proc_info.h"
//#include "orte/runtime/orte_globals.h"

//#include "opal/util/proc.h"

scon_common_scon_t * scon_base_create (scon_proc_t procs[],
                                     size_t nprocs,
                                     scon_info_t info[],
                                     size_t ninfo,
                                     scon_create_cbfunc_t cbfunc,
                                     void *cbdata)
{
    unsigned int i, index;
    int rc;
    scon_member_t *mem;
    scon_value_t *val;
    scon_common_scon_t *scon = SCON_NEW(scon_common_scon_t);
    /* common create code goes here */
    /* we need to bump up the index while setting the handle as handle = 0 is invalid
     */
    index = scon_pointer_array_add (&scon_common_base.scons, scon);
    scon->handle = (index + 1) % MAX_SCONS;
#if 0
    /* set default master proc as rank 0 */
    scon->master.jobid = ORTE_PROC_MY_NAME->jobid;
    scon->master.vpid = 0;
    /* populate member info */
    if (NULL == procs) {
       /* populate the member list ourselves - we can remove this
         code if we don't use this at all */
        for (i =0; i< orte_process_info.num_procs; i++) {
            mem = OBJ_NEW(orte_scon_member_t);
            mem->name.jobid = ORTE_PROC_MY_NAME->jobid;
            mem->name.vpid = i;
            opal_list_append(&scon->members, &mem->super);
        }
        /* set type */
        scon->type = SCON_TYPE_MY_JOB_ALL;


    } else {
        /* parse array and add only requested job members */
        /* assume my job but partial scon */
        scon->type = SCON_TYPE_MY_JOB_PARTIAL;
        /* convert the array of pmix_proc_t to the list of procs */
        for (i=0; i < nprocs; i++) {
            mem = OBJ_NEW(orte_scon_member_t);
            opal_list_append(&scon->members, &mem->super);
            if (OPAL_SUCCESS != (rc = opal_convert_string_to_jobid(&mem->name.jobid, procs[i].job_name))) {
                ORTE_ERROR_LOG(SCON_ERR_BAD_NAMESPACE);
                goto error;
            }
            if (ORTE_VPID_WILDCARD == procs[i].rank) {
               ORTE_ERROR_LOG(SCON_ERR_WILDCARD_NOT_SUPPORTED);
               goto error;
            } else {
                mem->name.vpid = procs[i].rank;
            }
            if(ORTE_PROC_MY_NAME->jobid != mem->name.jobid)
            {
                /* multi job scon set type and fail req for now */
                scon->type = SCON_TYPE_MULTI_JOB;
                ORTE_ERROR_LOG(SCON_ERR_MULTI_JOB_NOT_SUPPORTED);
                goto error;
            }
        }
    }
    scon->num_procs = opal_list_get_size(&scon->members);
    for (i = 0; i < ninfo; i++) {
        val = OBJ_NEW(opal_value_t);
        val->key = strdup(info[i].key);
        opal_list_append(&scon->attributes, &val->super);
        if (OPAL_SUCCESS != (rc = opal_value_load(val, (void**) &info[i].value.data, info[i].value.type))) {
            ORTE_ERROR_LOG(SCON_ERR_SILENT);
            goto error;
        }
        /* set the master proc */
        if (0 == strncmp(val->key, SCON_MASTER_PROC, SCON_MAX_KEYLEN)) {
            if (OPAL_SUCCESS != (rc =opal_value_unload(val, (void**)&scon->master, ORTE_NAME))) {
                ORTE_ERROR_LOG(SCON_ERR_SILENT);
                goto error;
            }
        }

    }
#endif
    return scon;
error:
    scon_pointer_array_set_item(&scon_common_base.scons, index, NULL);
    SCON_RELEASE(scon);
    return NULL;
}

#if 0
scon_status_t orte_scon_base_pack_scon_config(orte_scon_scon_t *scon,
                                              opal_buffer_t *buffer)
{
    int rc, num_attributes;
    opal_value_t *kv;
    orte_scon_member_t *mem;
    /* pack the following config info
     * (1) scon attributes
     * (2) scon type
     * (3) scon members if type != SINGLE_JOB_ALL
     * (4) scon id */
    /* pack num attributes */
    num_attributes = opal_list_get_size(&scon->attributes);
    if (ORTE_SUCCESS != (rc = opal_dss.pack(buffer,
                              &num_attributes, 1, ORTE_STD_CNTR))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    /* pack attributes */
    OPAL_LIST_FOREACH(kv, &scon->attributes, opal_value_t) {
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                            "%s orte_scon_base_pack_scon_config packing attribute = %s\n",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             kv->key);
        if (ORTE_SUCCESS != (rc = opal_dss.pack(buffer, (void*)&kv->key, 1, OPAL_STRING))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        if (ORTE_SUCCESS != (rc = opal_dss.pack(buffer, (void*)&kv->type, 1, OPAL_UINT8))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        if (ORTE_SUCCESS != (rc = opal_dss.pack(buffer, (void*)&kv->data, 1, kv->type))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }
    /* pack type */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(buffer, &scon->type,
                                             1, OPAL_UINT32))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    /* pack members if necessary */
    if (SCON_TYPE_MY_JOB_ALL != scon->type) {
        opal_output_verbose(5, orte_scon_base_framework.framework_output,
                             "%s orte_scon_base_pack_scon_config packing members type = %d\n",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             scon->type);
        OPAL_LIST_FOREACH(mem, &scon->members, orte_scon_member_t) {
            if (ORTE_SUCCESS != (rc = opal_dss.pack(buffer, (void*)&mem->name, 1, ORTE_NAME))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
        }
    }
    /* pack local id*/
    if (ORTE_SUCCESS != (rc = opal_dss.pack(buffer, &scon->handle, 1, OPAL_INT32))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    return ORTE_SUCCESS;
}

scon_status_t orte_scon_base_check_config (orte_scon_scon_t *scon,
                                           opal_buffer_t *buffer)
{
    int rc = ORTE_SUCCESS;
    int count, n , k;
    opal_value_t *kv, *kv1;
    bool match;
    scon_type_t type;
    count = 0;
    n = 1;
    if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &count,
                                              &n, ORTE_STD_CNTR))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    opal_output_verbose(5, orte_scon_base_framework.framework_output,
                         "%s orte_scon_base_check_config num attributes = %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         count);
    for (k=0; k < count; k++) {
        n=1;
        kv = OBJ_NEW(opal_value_t);
        if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &kv->key,
                                  &n, OPAL_STRING)))
        {
            OBJ_RELEASE(kv);
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        n=1;
        if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &kv->type,
                                              &n, OPAL_UINT8)))
        {
            OBJ_RELEASE(kv);
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        n=1;
        if (ORTE_SUCCESS != (rc = opal_dss.unpack(buffer, &kv->data, &n, kv->type)))
        {
            OBJ_RELEASE(kv);
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        match = false;
        /* now compare this against the local attributes */
        OPAL_LIST_FOREACH(kv1, &scon->attributes, opal_value_t) {
            if((0 == strncmp(kv->key, kv1->key, SCON_MAX_KEYLEN)) &&
            (OPAL_EQUAL == opal_dss.compare(&kv->data, &kv1->data, kv->type))) {
                match = true;
                break;
            }
        }
        OBJ_RELEASE(kv);
        if (!match) {
            ORTE_ERROR_LOG(rc);
            return SCON_ERR_CONFIG_MISMATCH;
        }

    }
    n = 1;
    if (ORTE_SUCCESS == (rc = opal_dss.unpack(buffer, &type, &n, OPAL_UINT32))) {
        if (type != scon->type)
           return SCON_ERR_CONFIG_MISMATCH;
    }
    return rc;
}

scon_handle_t orte_scon_base_get_handle(orte_scon_scon_t *scon,
                                        orte_process_name_t *member)
{
    orte_scon_member_t *mem;
    /* return local scon id of the member */
    OPAL_LIST_FOREACH(mem, &scon->members, orte_scon_member_t) {
        if((member->jobid == mem->name.jobid) &&
                (member->vpid == mem->name.vpid)) {
            return mem->local_handle;
        }
    }
    return SCON_HANDLE_INVALID;
}
#endif
