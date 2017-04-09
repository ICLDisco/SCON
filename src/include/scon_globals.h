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

#include <scon_config.h>

#include <scon_types.h>

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

typedef enum {
    SCON_PROC_NONE = 0,
    SCON_PROC_ROOT = 1,
    SCON_PROC_INTERIM_NODE = 2,
    SCON_PROC_LEAF = 3
}scon_proc_type_t;

/****    GLOBAL STORAGE    ****/
/* define a global construct that includes values that must be shared
 * between various parts of the code library. */
typedef struct {
    int init_cntr;                       // #times someone called Init - #times called Finalize
    scon_proc_t *myid;                   // my id
    char *my_uri;                        // my complete address  expressed as uri ( my proc name, my network address ip or fabrics)
    uid_t uid;                           // my effective uid
    gid_t gid;                           // my effective gid
    unsigned int num_peers;              // number of peer processes in my job
    scon_event_base_t *evbase;           // the global event base
    bool external_evbase;                // if we are bound to an external event vase
    int debug_output;                    // the global debug level
    scon_list_t scons;                   // list of scons that this process is a member of
    scon_proc_type_t type;                 // the process type (leaf node, root node or interim node)
} scon_globals_t;
SCON_EXPORT extern scon_globals_t scon_globals;

SCON_EXPORT extern scon_proc_t scon_proc_wildcard;

#define SCON_PROC_MY_NAME       (scon_globals.myid)
#define SCON_PROC_WILDCARD      (&scon_proc_wildcard)
#define SCON_JOB_SIZE          (scon_globals.num_peers)
/* max no of scons for a process */
#define MAX_SCONS 10
/* lets make 0 to indicate non existent scon for legacy reasons*/
#define SCON_HANDLE_INVALID        0
#define SCON_HANDLE_NEW            0XFFFFFFFE
#define SCON_INDEX_UNDEFINED       INT_MAX

/* this is specific to scon, will be set during SCON create by setting the appropriate flags,
   lets assume rank 0 is master for now */
#define SCON_PROC_IS_MASTER    ((scon_globals.myid->rank == 0) ||        \
                                (scon_globals.type == SCON_PROC_ROOT))

#define SCON_PROC_IS_INTERIM_NODE  (scon_globals.type == SCON_PROC_INTERIM_NODE)
END_C_DECLS
#endif /* SCON_GLOBALS_H */
