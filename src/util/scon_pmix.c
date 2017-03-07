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
#include "scon_globals.h"
#include "src/util/error.h"
#include "src/util/name_fns.h"
#include "scon_pmix.h"

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
    if (PMIX_SUCCESS != (rc = PMIx_Init(&myproc, NULL, 0))) {
        scon_output(0, "%s pmix init failed with error %d",
                     SCON_PRINT_PROC(SCON_PROC_MY_NAME), rc);
        return SCON_ERR_PMIXINIT_FAILED;
    }
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
    if (PMIX_SUCCESS != (rc = PMIx_Get(&wildproc, PMIX_UNIV_SIZE, NULL, 0, &val))) {
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
    pmix_value_t *pval = (pmix_value_t*) *val;
    if (PMIX_SUCCESS != (rc = PMIx_Get(pproc, key, NULL, 0, &pval))) {
        scon_output(0, "%s scon_pmix_get: PMIx_Get for key %s failed: with status %d\n",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), key, rc);
        return SCON_ERR_PMIXGET_FAILED;
    }
    return SCON_SUCCESS;
}

SCON_EXPORT int scon_pmix_put (const char key[], scon_value_t *val)
{
    pmix_value_t *pval = (pmix_value_t*) val;
    PMIx_Put(PMIX_GLOBAL, key, pval);
    PMIx_Commit();
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
    return PMIx_Finalize(NULL, 0);
}
