/*
 * Copyright (c) 2017 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/** @file : pmix interface definition */

#include <pmix.h>
#include <pmix_common.h>
#include <scon.h>
#include <scon_common.h>

int scon_pmix_init ( scon_proc_t *me,
                     scon_info_t  info[],
                     size_t ninfo);

int scon_pmix_get (const scon_proc_t *proc, const char key[],
                   const scon_info_t info[], size_t ninfo,
                   scon_value_t **val);

int scon_pmix_put (const char key[], scon_value_t *val);

int scon_pmix_put_string (const char key[], char *string_val);

int scon_pmix_fence(const scon_proc_t procs[], size_t nprocs,
                    const scon_info_t info[], size_t ninfo,
                    scon_op_cbfunc_t cbfunc, void *cbdata);

int scon_pmix_finalize (scon_info_t  info[],
                     size_t ninfo);

/* load pmix_value_t with scon_value_t */
void scon_pmix_value_load(pmix_value_t *v,
                          scon_value_t *kv);

/* unload pmix value into scon_value_t */
int scon_pmix_value_unload(scon_value_t *kv,
                            const pmix_value_t *v);

#define SCON_PMIX_STATUS_CONVERT_TO_SCON(status) \
         (SCON_ERR_PMIX_ERR_BASE + status)

/** scon pmix keys **/
#define SCON_PMIX_PROC_URI           "scon.proc.uri"
#define SCON_PMIX_NSPACE              PMIX_NSPACE
#define SCON_PMIX_JOB_SIZE            PMIX_JOB_SIZE
#define SCON_PMIX_RANK                PMIX_RANK

/** SCON PMIX ERROR codes **/
/** just use a different base to identify pmix errors */
#define SCON_ERR_PMIX_ERR_BASE         -2000
