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
int scon_pmix_fence(const scon_proc_t procs[], size_t nprocs,
                    const scon_info_t info[], size_t ninfo,
                    scon_op_cbfunc_t cbfunc, void *cbdata);
int scon_pmix_finalize (scon_info_t  info[],
                     size_t ninfo);


#define SCON_PMIX_PROC_URI   "scon.proc.uri"
