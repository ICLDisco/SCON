/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
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
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include "src/util/output.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/base/scon_mca_base_component_repository.h"
#include "scon_common.h"

extern int scon_mca_base_opened;

/*
 * Main MCA shutdown.
 */
int scon_mca_base_close(void)
{
    assert (scon_mca_base_opened);
    if (!--scon_mca_base_opened) {
        /* deregister all MCA base parameters */
        int group_id = scon_mca_base_var_group_find ("scon", "mca", "base");

        if (-1 < group_id) {
            scon_mca_base_var_group_deregister (group_id);
        }

        /* release the default paths */
        if (NULL != scon_mca_base_system_default_path) {
            free(scon_mca_base_system_default_path);
        }
        if (NULL != scon_mca_base_user_default_path) {
            free(scon_mca_base_user_default_path);
        }

        /* Close down the component repository */
        scon_mca_base_component_repository_finalize();

        /* Shut down the dynamic component finder */
        scon_mca_base_component_find_finalize();

        /* Close scon output stream 0 */
        scon_output_close(0);
    }

    /* All done */
    return SCON_SUCCESS;
}
