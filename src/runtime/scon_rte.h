/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2010-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file **/

#ifndef SCON_RTE_H
#define SCON_RTE_H

#include "scon_config.h"
#include "scon_common.h"
#include "src/class/scon_object.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include SCON_EVENT_HEADER

#include "src/include/scon_globals.h"

BEGIN_C_DECLS

SCON_EXPORT extern int scon_initialized;
SCON_EXPORT extern char* scon_net_private_ipv4;
/** version string of scon */
SCON_EXPORT extern const char scon_version_string[];

/**
 * Initialize the SCON layer, including the MCA system.
 *
 * @retval SCON_SUCCESS Upon success.
 * @retval SCON_ERROR Upon failure.
 *
 */
scon_status_t scon_rte_init(scon_info_t info[], size_t ninfo);

/**
 * Finalize the SCON layer, including the MCA system.
 *
 */
void scon_rte_finalize(void);

/* internal functions do not call */
scon_status_t scon_register_params(void);
scon_status_t scon_deregister_params(void);

/* In a few places, we need to barrier until something happens
 * that changes a flag to indicate we can release - e.g., waiting
 * for a specific message to arrive. If no progress thread is running,
 * we cycle across scon_progress - however, if a progress thread
 * is active, then we need to just nanosleep to avoid cross-thread
 * confusion
 */
#define SCON_WAIT_FOR_COMPLETION(flg)                                   \
    do {                                                                \
        scon_output_verbose(1, scon_globals.debug_output,              \
                            "%s waiting on progress thread at %s:%d",   \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__);                        \
        while ((flg)) {                                                 \
            /* provide a short quiet period so we                       \
             * don't hammer the cpu while waiting                       \
             */                                                         \
            struct timespec tp = {0, 100000};                           \
            nanosleep(&tp, NULL);                                       \
        }                                                               \
    }while(0);

END_C_DECLS

#endif /* SCON_RTE_H */
