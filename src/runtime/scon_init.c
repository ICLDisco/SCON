/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * @file scon_init.c
 */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <scon.h>
#include <scon_common.h>
#include "src/include/scon_config.h"
#include "src/include/scon_globals.h"
#include "src/util/output.h"
#include "src/util/show_help.h"
#include "src/mca/base/base.h"
#include "src/mca/base/scon_mca_base_var.h"
#include "src/mca/pinstalldirs/base/base.h"
#include "src/mca/common/base/base.h"
#include "src/mca/collectives/base/base.h"
#include "src/mca/pt2pt/base/base.h"
#include "src/util/error.h"
#include "src/util/keyval_parse.h"
#include "src/runtime/scon_progress_threads.h"

#if SCON_CC_USE_PRAGMA_IDENT
#pragma ident SCON_IDENT_STRING
#elif SCON_CC_USE_IDENT
#ident SCON_IDENT_STRING
#endif
const char scon_version_string[] = SCON_IDENT_STRING;

int scon_initialized = 0;
bool scon_init_called = false;
scon_globals_t scon_globals = {0};
bool scon_finalizing = false;

int scon_init(scon_info_t info[],
                 size_t ninfo)
{
    int ret, debug_level;
    char *error = NULL, *evar;
    char *param;
    size_t n;

    if( ++scon_initialized != 1 ) {
        if( scon_initialized < 1 ) {
            return SCON_ERROR;
        }
        return SCON_SUCCESS;
    }

#if SCON_NO_LIB_DESTRUCTOR
    if (scon_init_called) {
        /* can't use show_help here */
        fprintf (stderr, "scon_init: attempted to initialize after finalize without compiler "
                 "support for either __attribute__(destructor) or linker support for -fini -- process "
                 "will likely abort\n");
        return SCON_ERR_NOT_SUPPORTED;
    }
#endif

    scon_init_called = true;

    /* initialize the output system */
    if (!scon_output_init()) {
        return SCON_ERROR;
    }

    /* initialize install dirs code */
    if (SCON_SUCCESS != (ret = scon_mca_base_framework_open(&scon_pinstalldirs_base_framework, 0))) {
        fprintf(stderr, "scon_pinstalldirs_base_open() failed -- process will likely abort (%s:%d, returned %d instead of SCON_SUCCESS)\n",
                __FILE__, __LINE__, ret);
        return ret;
    }

    /* initialize the help system */
    scon_show_help_init();

    /* keyval lex-based parser */
    if (SCON_SUCCESS != (ret = scon_util_keyval_parse_init())) {
        error = "scon_util_keyval_parse_init";
        goto return_error;
    }

    /* Setup the parameter system */
    if (SCON_SUCCESS != (ret = scon_mca_base_var_init())) {
        error = "mca_base_var_init";
        goto return_error;
    }

    /* read any param files that were provided */
    if (SCON_SUCCESS != (ret = scon_mca_base_var_cache_files(false))) {
        error = "failed to cache files";
        goto return_error;
    }
    /* initialize the mca */
    if (SCON_SUCCESS != (ret = scon_mca_base_open())) {
        error = "mca_base_open";
        goto return_error;
    }
    /* setup the globals structure */
    memset(&scon_globals.myid, 0, sizeof(scon_proc_t));
    SCON_CONSTRUCT(&scon_globals.scons, scon_list_t);
    /* get our effective id's */
    scon_globals.uid = geteuid();
    scon_globals.gid = getegid();
    /* see if debug is requested */
    if (NULL != (evar = getenv("SCON_DEBUG"))) {
        debug_level = strtol(evar, NULL, 10);
        scon_globals.debug_output = scon_output_open(NULL);
        scon_output_set_verbosity(scon_globals.debug_output, debug_level);
    }
    /* scan incoming info for directives */
    if (NULL != info) {
        for (n=0; n < ninfo; n++) {
            if (0 == strcmp(SCON_EVENT_BASE, info[n].key)) {
                scon_globals.evbase = (scon_event_base_t*)info[n].value.data.ptr;
                scon_globals.external_evbase = true;
            }
        }
    }
    /* tell libevent that we need thread support */
    scon_event_use_threads();
    if (!scon_globals.external_evbase) {
        /* create an event base and progress thread for us */
        if (NULL == (scon_globals.evbase = scon_progress_thread_init(NULL))) {
            error = "progress thread";
            ret = SCON_ERROR;
            goto return_error;
        }
    }
    /** Open the common framework and select the overall implementation
        selection of other plugins happen will happen inside the common init call***/

    if (SCON_SUCCESS != (ret = scon_mca_base_framework_open(&scon_common_base_framework, 0))) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init failed : error = orte_scon_base_open");
        error = "scon_common_base_open";
        goto return_error;
    }
    if (SCON_SUCCESS != (ret = scon_common_base_select())) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init failed : error = orte_scon_base_select");
        error = "scon_common_base_select";
        goto return_error;
    }

    if (SCON_SUCCESS != (ret = scon_common.init(info, ninfo))) {
        scon_output_verbose(0, scon_globals.debug_output, "scon_init failed : error = orte_scon_init");
        error = "scon_common.init";
        goto return_error;
    }
    /* All done */
    return SCON_SUCCESS;

return_error:
    scon_initialized--;
    scon_show_help( "help-scon-runtime.txt",
                    "scon_init:startup:internal-failure", true,
                    error, ret );
    return ret;
}

scon_handle_t scon_create(scon_proc_t procs[],
                          size_t nprocs,
                          scon_info_t info[],
                          size_t ninfo,
                          scon_create_cbfunc_t cbfunc,
                          void *cbdata)
{
    return scon_common.create( procs, nprocs, info, ninfo, cbfunc, cbdata);
}

scon_status_t scon_get_info (scon_handle_t scon_handle,
                             scon_info_t **info,
                             size_t *ninfo)
{
    return SCON_ERR_NOT_IMPLEMENTED;
}

scon_status_t scon_send_nb (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_buffer_t *buf,
                            scon_msg_tag_t tag,
                            scon_send_cbfunc_t cbfunc,
                            void *cbdata,
                            scon_info_t info[],
                            size_t ninfo)
{
    return scon_pt2pt.send(scon_handle, peer, buf, tag, cbfunc, cbdata, info, ninfo);
}

scon_status_t scon_recv_nb (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_msg_tag_t tag,
                            bool persistent,
                            scon_recv_cbfunc_t cbfunc,
                            void *cbdata,
                            scon_info_t info[],
                            size_t ninfo)
{
    return scon_pt2pt.recv(scon_handle, peer, tag, persistent, cbfunc, cbdata, info, ninfo);
}

scon_status_t scon_recv_cancel ( scon_handle_t scon_handle,
                                 scon_proc_t *peer,
                                 scon_msg_tag_t tag)
{
    return SCON_ERR_NOT_IMPLEMENTED;
}

scon_status_t scon_xcast (scon_handle_t scon_handle,
                          scon_proc_t procs[],
                          size_t nprocs,
                          scon_buffer_t *buf,
                          scon_msg_tag_t tag,
                          scon_xcast_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo)
{
    return scon_collectives.xcast(scon_handle, procs, nprocs, buf, tag, cbfunc, cbdata,
                            info, ninfo);
}

scon_status_t scon_barrier(scon_handle_t scon_handle,
                           scon_proc_t procs[],
                           size_t nprocs,
                           scon_barrier_cbfunc_t cbfunc,
                           void *cbdata,
                           scon_info_t info[],
                           size_t ninfo)
{
    return scon_collectives.barrier(scon_handle, procs, nprocs, cbfunc, cbdata, info, ninfo);
}

scon_status_t scon_delete(scon_handle_t scon_handle,
                          scon_op_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo)
{
    return scon_common.del(scon_handle, cbfunc, cbdata, info, ninfo);
}

scon_status_t scon_finalize(void)
{
    return scon_common.finalize();
}
