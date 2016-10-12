/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_GLOBALS_H
#define SCON_GLOBALS_H

#include <src/include/scon_config.h>

#include <src/include/types.h>

#include <unistd.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include SCON_EVENT_HEADER

#include <scon_common.h>
#include "src/class/scon_hash_table.h"
#include "src/class/scon_list.h"

BEGIN_C_DECLS

/* some limits */
#define SCON_MAX_CRED_SIZE      131072              // set max at 128kbytes
#define SCON_MAX_ERR_CONSTANT   INT_MIN


/****    GLOBAL STORAGE    ****/
/* define a global construct that includes values that must be shared
 * between various parts of the code library. */
typedef struct {
    int init_cntr;                       // #times someone called Init - #times called Finalize
    scon_proc_t myid;
    uid_t uid;                           // my effective uid
    gid_t gid;                           // my effective gid
    scon_event_base_t *evbase;
    bool external_evbase;
    int debug_output;
    scon_list_t scons;                 // list of scons that this process is a member of
} scon_globals_t;


extern scon_globals_t scon_globals;

END_C_DECLS

#endif /* SCON_GLOBALS_H */
