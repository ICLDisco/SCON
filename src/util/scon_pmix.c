/*
 * Copyright (c) 2017 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/** @file : pmix interface implementation */
#include <pmix.h>
#include <pmix_common.h>
#include <scon.h>
#include <scon_common.h>
#include "scon_config.h"
#include "scon_globals.h"
#include "src/util/error.h"
#include "src/util/name_fns.h"
#include "scon_pmix.h"

void scon_pmix_value_load(pmix_value_t *pv,
                       scon_value_t *sv)
{
    switch(sv->type) {
        case SCON_UNDEF:
            pv->type = PMIX_UNDEF;
            break;
        case SCON_BOOL:
            pv->type = PMIX_BOOL;
            memcpy(&(pv->data.flag), &sv->data.flag, 1);
            break;
        case SCON_BYTE:
            pv->type = PMIX_BYTE;
            memcpy(&(pv->data.byte), &sv->data.byte, 1);
            break;
        case SCON_STRING:
            pv->type = PMIX_STRING;
            if (NULL != sv->data.string) {
                pv->data.string = strdup(sv->data.string);
            } else {
                pv->data.string = NULL;
            }
            break;
        case SCON_SIZE:
            pv->type = PMIX_SIZE;
            pv->data.size = (size_t)sv->data.size;
            break;
        case SCON_PID:
            pv->type = PMIX_PID;
            memcpy(&(pv->data.pid), &sv->data.pid, sizeof(pid_t));
            break;
        case SCON_INT:
            pv->type = PMIX_INT;
            memcpy(&(pv->data.integer), &sv->data.integer, sizeof(int));
            break;
        case SCON_INT8:
            pv->type = PMIX_INT8;
            memcpy(&(pv->data.int8), &sv->data.int8, 1);
            break;
        case SCON_INT16:
            pv->type = PMIX_INT16;
            memcpy(&(pv->data.int16), &sv->data.int16, 2);
            break;
        case SCON_INT32:
            pv->type = PMIX_INT32;
            memcpy(&(pv->data.int32), &sv->data.int32, 4);
            break;
        case SCON_INT64:
            pv->type = PMIX_INT64;
            memcpy(&(pv->data.int64), &sv->data.int64, 8);
            break;
        case SCON_UINT:
            pv->type = PMIX_UINT;
            memcpy(&(pv->data.uint), &sv->data.uint, sizeof(int));
            break;
        case SCON_UINT8:
            pv->type = PMIX_UINT8;
            memcpy(&(pv->data.uint8), &sv->data.uint8, 1);
            break;
        case SCON_UINT16:
            pv->type = PMIX_UINT16;
            memcpy(&(pv->data.uint16), &sv->data.uint16, 2);
            break;
        case SCON_UINT32:
            pv->type = PMIX_UINT32;
            memcpy(&(pv->data.uint32), &sv->data.uint32, 4);
            break;
        case SCON_UINT64:
            pv->type = PMIX_UINT64;
            memcpy(&(pv->data.uint64), &sv->data.uint64, 8);
            break;
        case SCON_FLOAT:
            pv->type = PMIX_FLOAT;
            memcpy(&(pv->data.fval), &sv->data.fval, sizeof(float));
            break;
        case SCON_DOUBLE:
            pv->type = PMIX_DOUBLE;
            memcpy(&(pv->data.dval), &sv->data.dval, sizeof(double));
            break;
        case SCON_TIMEVAL:
            pv->type = PMIX_TIMEVAL;
            memcpy(&(pv->data.tv), &sv->data.tv, sizeof(struct timeval));
            break;
        case SCON_TIME:
            pv->type = PMIX_TIME;
            memcpy(&(pv->data.time), &sv->data.time, sizeof(time_t));
            break;
        case SCON_STATUS:
            pv->type = PMIX_STATUS;
            memcpy(&(pv->data.status), &sv->data.status, sizeof(pmix_status_t));
            break;
        case SCON_PROC:
            pv->type = PMIX_PROC;
            /* have to stringify the jobid */
            PMIX_PROC_CREATE(pv->data.proc, 1);
            strncpy(pv->data.proc->nspace,  sv->data.proc->job_name, PMIX_MAX_NSLEN);
            pv->data.proc->rank = sv->data.proc->rank;
            break;
        case SCON_BYTE_OBJECT:
            pv->type = PMIX_BYTE_OBJECT;
            if (NULL != sv->data.bo.bytes) {
                pv->data.bo.bytes = (char*)malloc(sv->data.bo.size);
                memcpy(pv->data.bo.bytes, sv->data.bo.bytes, sv->data.bo.size);
                pv->data.bo.size = (size_t)sv->data.bo.size;
            } else {
                pv->data.bo.bytes = NULL;
                pv->data.bo.size = 0;
            }
            break;
        case SCON_POINTER:
            pv->type = PMIX_POINTER;
            pv->data.ptr = sv->data.ptr;
            break;
        default:
            /* silence warnings */
            break;
    }
}

int scon_pmix_value_unload(scon_value_t *sv,
                        const pmix_value_t *pv)
{
    int rc=SCON_SUCCESS;
    switch(pv->type) {
        case PMIX_UNDEF:
            sv->type = SCON_UNDEF;
            break;
        case PMIX_BOOL:
            sv->type = SCON_BOOL;
            memcpy(&sv->data.flag, &(pv->data.flag), 1);
            break;
        case PMIX_BYTE:
            sv->type = SCON_BYTE;
            memcpy(&sv->data.byte, &(pv->data.byte), 1);
            break;
        case PMIX_STRING:
            sv->type = SCON_STRING;
            if (NULL != pv->data.string) {
                sv->data.string = strdup(pv->data.string);
            }
            break;
        case PMIX_SIZE:
            sv->type = SCON_SIZE;
            sv->data.size = (int)pv->data.size;
            break;
        case PMIX_PID:
            sv->type = SCON_PID;
            memcpy(&sv->data.pid, &(pv->data.pid), sizeof(pid_t));
            break;
        case PMIX_INT:
            sv->type = SCON_INT;
            memcpy(&sv->data.integer, &(pv->data.integer), sizeof(int));
            break;
        case PMIX_INT8:
            sv->type = SCON_INT8;
            memcpy(&sv->data.int8, &(pv->data.int8), 1);
            break;
        case PMIX_INT16:
            sv->type = SCON_INT16;
            memcpy(&sv->data.int16, &(pv->data.int16), 2);
            break;
        case PMIX_INT32:
            sv->type = SCON_INT32;
            memcpy(&sv->data.int32, &(pv->data.int32), 4);
            break;
        case PMIX_INT64:
            sv->type = SCON_INT64;
            memcpy(&sv->data.int64, &(pv->data.int64), 8);
            break;
        case PMIX_UINT:
            sv->type = SCON_UINT;
            memcpy(&sv->data.uint, &(pv->data.uint), sizeof(int));
            break;
        case PMIX_UINT8:
            sv->type = SCON_UINT8;
            memcpy(&sv->data.uint8, &(pv->data.uint8), 1);
            break;
        case PMIX_UINT16:
            sv->type = SCON_UINT16;
            memcpy(&sv->data.uint16, &(pv->data.uint16), 2);
            break;
        case PMIX_UINT32:
            sv->type = SCON_UINT32;
            memcpy(&sv->data.uint32, &(pv->data.uint32), 4);
            break;
        case PMIX_UINT64:
            sv->type = SCON_UINT64;
            memcpy(&sv->data.uint64, &(pv->data.uint64), 8);
            break;
        case PMIX_FLOAT:
            sv->type = SCON_FLOAT;
            memcpy(&sv->data.fval, &(pv->data.fval), sizeof(float));
            break;
        case PMIX_DOUBLE:
            sv->type = SCON_DOUBLE;
            memcpy(&sv->data.dval, &(pv->data.dval), sizeof(double));
            break;
        case PMIX_TIMEVAL:
            sv->type = SCON_TIMEVAL;
            memcpy(&sv->data.tv, &(pv->data.tv), sizeof(struct timeval));
            break;
        case PMIX_TIME:
            sv->type = SCON_TIME;
            memcpy(&sv->data.time, &(pv->data.time), sizeof(time_t));
            break;
        case PMIX_STATUS:
            sv->type = SCON_STATUS;
            sv->data.status = SCON_PMIX_STATUS_CONVERT_TO_SCON(pv->data.status);
            break;
        case PMIX_PROC:
            sv->type = SCON_PROC;
            SCON_PROC_CREATE(sv->data.proc, 1);
            /* copy jobname */
            strncpy(sv->data.proc->job_name,  pv->data.proc->nspace, SCON_MAX_JOBLEN);
            sv->data.proc->rank = pv->data.proc->rank;
            break;
        case PMIX_BYTE_OBJECT:
            sv->type = SCON_BYTE_OBJECT;
            if (NULL != pv->data.bo.bytes && 0 < pv->data.bo.size) {
                sv->data.bo.bytes = (char*)malloc(pv->data.bo.size);
                memcpy(sv->data.bo.bytes, pv->data.bo.bytes, pv->data.bo.size);
                sv->data.bo.size = (int)pv->data.bo.size;
            } else {
                sv->data.bo.bytes = NULL;
                sv->data.bo.size = 0;
            }
            break;
        case PMIX_POINTER:
            sv->type = SCON_POINTER;
            sv->data.ptr = pv->data.ptr;
            break;
        default:
            /* silence warnings */
            rc = SCON_ERROR;
            break;
    }
    return rc;
}


SCON_EXPORT int scon_pmix_init ( scon_proc_t *me,
                     scon_info_t  info[],
                     size_t ninfo)
{
    pmix_proc_t myproc, wildproc;
    int rc;
    pmix_value_t value;
    pmix_value_t *val = &value;
    scon_output_verbose(2, scon_globals.debug_output,
                         "%s scon_pmix_init: initializing pmix client",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME));
#if SCON_HAVE_PMIX_VERSION == 2
    if (PMIX_SUCCESS != (rc = PMIx_Init(&myproc, NULL, 0))) {
        scon_output(0, "%s pmix init failed with error %d",
                     SCON_PRINT_PROC(SCON_PROC_MY_NAME), rc);
        return SCON_ERR_PMIXINIT_FAILED;
    }
#else
    if (PMIX_SUCCESS != (rc = PMIx_Init(&myproc))) {
        scon_output(0, "%s pmix init failed with error %d",
                     SCON_PRINT_PROC(SCON_PROC_MY_NAME), rc);
        return SCON_ERR_PMIXINIT_FAILED;
    }
#endif

    scon_output_verbose(2, scon_globals.debug_output,
                        "%s scon_pmix_init: pmix init returned my namespace %s, rank =%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        myproc.nspace,
                        myproc.rank);
    /* now set our identity to what's returned by pmix */
    strncpy(me->job_name, myproc.nspace, SCON_MAX_JOBLEN);
    me->rank = myproc.rank;
    /* get our job size next */
    /* job-related info is found in the nspace, assigned to the
    * wildcard rank as it doesn't relate to a specific rank. Setup
    * a name to retrieve such values */
    PMIX_PROC_CONSTRUCT(&wildproc);
    (void)strncpy(wildproc.nspace, myproc.nspace, PMIX_MAX_NSLEN);
    wildproc.rank = PMIX_RANK_WILDCARD;
    /* get our universe size */
    if (PMIX_SUCCESS != (rc = PMIx_Get(&wildproc, PMIX_JOB_SIZE, NULL, 0, &val))) {
        scon_output(0, "%s scon_pmix_init: PMIx_Get universe size failed: %d\n",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), rc);
        return SCON_ERR_PMIXGET_FAILED;
    }
    scon_globals.num_peers = val->data.uint32;
    PMIX_VALUE_RELEASE(val);
    scon_output_verbose(2, scon_globals.debug_output,
                        "%s scon_pmix_init: PMIx_Get returned my univ size = %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), scon_globals.num_peers);
    return SCON_SUCCESS;
}

SCON_EXPORT int scon_pmix_get (const scon_proc_t *proc, const char key[],
                   const scon_info_t info[], size_t ninfo,
                   scon_value_t **val)
{
    int rc;
    pmix_proc_t *pproc = (pmix_proc_t*) proc;
    pmix_value_t *pval;
    scon_value_t *sval;
    if (PMIX_SUCCESS != (rc = PMIx_Get(pproc, key, NULL, 0, &pval))) {
        scon_output(0, "%s scon_pmix_get: PMIx_Get for key %s failed: with status %d\n",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), key,
                    SCON_PMIX_STATUS_CONVERT_TO_SCON(rc));
        return SCON_ERR_PMIXGET_FAILED;
    }
    SCON_VALUE_CREATE(sval,1);
    scon_pmix_value_unload(sval, pval);
    *val = sval;
    return SCON_SUCCESS;
}
SCON_EXPORT int scon_pmix_put (const char key[], scon_value_t *sval)
{
    pmix_value_t *pval;
    PMIX_VALUE_CREATE(pval,1);
    /* load scon_value_t into pmix_value_t */
    scon_pmix_value_load( pval, sval);
    /* put the data and commit */
    PMIx_Put(PMIX_GLOBAL, key, pval);
    PMIx_Commit();
    PMIX_VALUE_FREE(pval, 1);
    return SCON_SUCCESS;
}

SCON_EXPORT int scon_pmix_put_string (const char key[], char *string_val)
{
    pmix_value_t *pval;
    PMIX_VALUE_CREATE(pval,1);
    pmix_value_load(pval, (void*)string_val, PMIX_STRING);
    PMIx_Put(PMIX_GLOBAL, key, pval);
    PMIx_Commit();
    PMIX_VALUE_FREE(pval, 1);
    return SCON_SUCCESS;
}

SCON_EXPORT int scon_pmix_fence(const scon_proc_t procs[], size_t nprocs,
                    const scon_info_t info[], size_t ninfo,
                    scon_op_cbfunc_t cbfunc, void *cbdata)
{
    /* Not sure if we will need fence - will implemented if needed */
    return SCON_ERR_NOT_IMPLEMENTED;
}

SCON_EXPORT int scon_pmix_finalize (scon_info_t  info[],
                     size_t ninfo)
{
    scon_output_verbose(2, scon_globals.debug_output,
                        "%s scon_pmix_finalize: finalizing pmix client",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));
#if SCON_HAVE_PMIX_VERSION == 2
    return PMIx_Finalize(NULL, 0);
#else
    return PMIx_Finalize();
#endif
}
