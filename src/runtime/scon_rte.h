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

#if SCON_ENABLE_TIMING
extern char *scon_timing_sync_file;
extern char *scon_timing_output;
extern bool scon_timing_overhead;
#endif

extern int scon_initialized;

/** version string of scon */
extern const char scon_version_string[];

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



END_C_DECLS

#endif /* SCON_RTE_H */
