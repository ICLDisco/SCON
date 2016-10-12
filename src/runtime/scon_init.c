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

#include <src/include/scon_config.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/util/output.h"
#include "src/util/show_help.h"
#include "src/mca/base/base.h"
#include "src/mca/base/scon_mca_base_var.h"
#include "src/mca/pinstalldirs/base/base.h"
#include "src/mca/common/base/base.h"
#include "src/event/scon_event.h"
#include "src/include/types.h"
#include "src/util/error.h"
#include "src/util/keyval_parse.h"

#include "src/runtime/scon_rte.h"
#include "src/runtime/scon_progress_threads.h"

#if SCON_CC_USE_PRAGMA_IDENT
#pragma ident SCON_IDENT_STRING
#elif SCON_CC_USE_IDENT
#ident SCON_IDENT_STRING
#endif
const char scon_version_string[] = SCON_IDENT_STRING;

int scon_initialized = 0;
bool scon_init_called = false;
scon_globals_t scon_globals = {
};


int scon_rte_init(scon_info_t info[],
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

    /* register params for scon */
    if (SCON_SUCCESS != (ret = scon_register_params())) {
        error = "scon_register_params";
        goto return_error;
    }

    /* initialize the mca */
    if (SCON_SUCCESS != (ret = scon_mca_base_open())) {
        error = "mca_base_open";
        goto return_error;
    }

    /* setup the globals structure */
    scon_globals.proc_type = type;
    memset(&scon_globals.myid, 0, sizeof(scon_proc_t));
    SCON_CONSTRUCT(&scon_globals.nspaces, scon_list_t);
    SCON_CONSTRUCT(&scon_globals.events, scon_events_t);
    /* get our effective id's */
    scon_globals.uid = geteuid();
    scon_globals.gid = getegid();
    /* see if debug is requested */
    if (NULL != (evar = getenv("SCON_DEBUG"))) {
        debug_level = strtol(evar, NULL, 10);
        scon_globals.debug_output = scon_output_open(NULL);
        scon_output_set_verbosity(scon_globals.debug_output, debug_level);
    }
    /* create our peer object */
    scon_globals.mypeer = SCON_NEW(scon_peer_t);

    /* scan incoming info for directives */
    if (NULL != info) {
        for (n=0; n < ninfo; n++) {
            if (0 == strcmp(SCON_EVENT_BASE, info[n].key)) {
                scon_globals.evbase = (scon_event_base_t*)info[n].value.data.ptr;
                scon_globals.external_evbase = true;
            }
        }
    }


    /* open the psec and select the default module for this environment */
   /* if (SCON_SUCCESS != (ret = scon_mca_base_framework_open(&scon_psec_base_framework, 0))) {
        error = "scon_psec_base_open";
        goto return_error;
    }
    if (SCON_SUCCESS != (ret = scon_psec_base_select())) {
        error = "scon_psec_base_select";
        goto return_error;
    }
    param = getenv("SCON_SEC_MODULE");  // if directive was given, use it
    scon_globals.mypeer->comm.sec = scon_psec_base_assign_module(param);*/

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
    return SCON_SUCCESS;

return_error:
    scon_show_help( "help-scon-runtime.txt",
                    "scon_init:startup:internal-failure", true,
                    error, ret );
    return ret;
}

/** TO DO - need to define SCONs own util functions or use renaming scheme.
      - use #defines  for now **/
#define SCON_NAME_PRINT   ORTE_NAME_PRINT
#define SCON_JOBID_PRINT  ORTE_JOBID_PRINT
#define SCON_VPID_PRINT   ORTE_VPID_PRINT
#define scon_util_compare_name_fields  orte_util_compare_name_fields
#define scon_util_convert_string_to_process_name  orte_util_convert_string_to_process_name
#define scon_util_convert_process_name_to_string  orte_util_convert_process_name_to_string
#define scon_util_convert_string_to_jobid         orte_util_convert_string_to_jobid
#define scon_util_convert_jobid_to_string         orte_util_convert_jobid_to_string
#define scon_locks_init                           orte_locks_init
typedef orte_process_name_t  scon_process_name_t;

/**
 * Static functions used to configure the interactions between the OPAL and
 * the runtime.
 */

static char*
_process_name_print_for_opal(const opal_process_name_t procname)
{
    scon_process_name_t* scon_name = (scon_process_name_t*)&procname;
    return SCON_NAME_PRINT(scon_name);
}

static char*
_jobid_print_for_opal(const opal_jobid_t jobid)
{
    return SCON_JOBID_PRINT(jobid);
}

static char*
_vpid_print_for_opal(const opal_vpid_t vpid)
{
    return SCON_VPID_PRINT(vpid);
}

static int
_process_name_compare(const opal_process_name_t p1, const opal_process_name_t p2)
{
    return scon_util_compare_name_fields(ORTE_NS_CMP_ALL, &p1, &p2);
}

static int _convert_string_to_process_name(opal_process_name_t *name,
                                           const char* name_string)
{
    return scon_util_convert_string_to_process_name(name, name_string);
}

static int _convert_process_name_to_string(char** name_string,
                                          const opal_process_name_t *name)
{
    return scon_util_convert_process_name_to_string(name_string, name);
}

static int
_convert_string_to_jobid(opal_jobid_t *jobid, const char *jobid_string)
{
    return scon_util_convert_string_to_jobid(jobid, jobid_string);
}

/* init globals */
int scon_initialized = 0;
bool scon_finalizing = false;

scon_status_t scon_init( scon_info_t info[],
                          size_t ninfo)
{
    /* open and select scon framework */
    int ret;
    char *error = NULL;

    if (0 < scon_initialized) {
        /* track number of times we have been called */
        scon_initialized++;
        return SCON_ALREADY_INITED;
    }
    scon_initialized++;

    /* Convince OPAL to use our naming scheme */
    opal_process_name_print = _process_name_print_for_opal;
    opal_vpid_print = _vpid_print_for_opal;
    opal_jobid_print = _jobid_print_for_opal;
    opal_compare_proc = _process_name_compare;
    opal_convert_string_to_process_name = _convert_string_to_process_name;
    opal_convert_process_name_to_string = _convert_process_name_to_string;
    opal_snprintf_jobid = orte_util_snprintf_jobid;
    opal_convert_string_to_jobid = _convert_string_to_jobid;

    /* initialize the opal layer */
    if (SCON_SUCCESS != (ret = opal_init(NULL, NULL))) {
        opal_output(0, "scon_init failed : error = opal_init");
        error = "opal_init";
        goto error;
    }

    if (SCON_SUCCESS != (ret = scon_locks_init())) {
        opal_output(0, "scon_init failed : error = scon_locks_init");
        error = "scon_locks_init";
        goto error;
    }
    /*** Lets just open and select and init scon framework and let scon deal
         with the rest of the setup ***/

    if (SCON_SUCCESS != (ret = mca_base_framework_open(&orte_scon_base_framework, 0))) {
       // ORTE_ERROR_LOG(ret);
        opal_output(0, "scon_init failed : error = orte_scon_base_open");
        error = "orte_scon_base_open";
        goto error;
    }
    if (SCON_SUCCESS != (ret = orte_scon_base_select())) {
        opal_output(0, "scon_init failed : error = orte_scon_base_select");
        error = "orte_scon_base_select";
        goto error;
    }

    if (SCON_SUCCESS != (ret = orte_scon.init(info, ninfo))) {
        opal_output(0, "scon_init failed : error = orte_scon_init");
        error = "orte_scon_init";
        goto error;
    }
    /* All done */
    return SCON_SUCCESS;

error:
    scon_initialized--;
    if (SCON_ERR_SILENT != ret) {
        orte_show_help("help-scon-runtime",
                       "scon_init:startup:internal-failure",
                       true, error, ORTE_ERROR_NAME(ret), ret);
    }

    return ret;
}

scon_handle_t scon_create(scon_proc_t procs[],
                          size_t nprocs,
                          scon_info_t info[],
                          size_t ninfo,
                          scon_create_cbfunc_t cbfunc,
                          void *cbdata)
{
    return orte_scon.create( procs, nprocs, info, ninfo, cbfunc, cbdata);
}

scon_status_t scon_get_info (scon_handle_t scon_handle,
                             scon_info_t info[],
                             size_t *ninfo)
{
    return SCON_ERR_OP_NOT_SUPPORTED;
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
    return orte_scon.send(scon_handle, peer, buf, tag, cbfunc, cbdata, info, ninfo);
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
    return orte_scon.recv(scon_handle, peer, tag, persistent, cbfunc, cbdata, info, ninfo);
}

scon_status_t scon_recv_cancel ( scon_handle_t scon_handle,
                                 scon_proc_t *peer,
                                 scon_msg_tag_t tag)
{
    return SCON_ERR_OP_NOT_SUPPORTED;
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
    return orte_scon.xcast(scon_handle, procs, nprocs, buf, tag, cbfunc, cbdata,
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
    return orte_scon.barrier(scon_handle, procs, nprocs, cbfunc, cbdata, info, ninfo);
}

scon_status_t scon_delete(scon_handle_t scon_handle,
                          scon_op_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo)
{
    return orte_scon.del(scon_handle, cbfunc, cbdata, info, ninfo);
}

scon_status_t scon_finalize(void)
{
    return orte_scon.finalize();
}
