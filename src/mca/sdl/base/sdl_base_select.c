/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University.
 *                         All rights reserved.
 *
 * Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * Copyright (c) 2015      Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2016-2017 Intel, Inc.  All rights reserved.
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#ifdef HAVE_UNISTD_H
#include "unistd.h"
#endif

#include "scon_common.h"
#include "src/util/output.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/sdl/sdl.h"
#include "src/mca/sdl/base/base.h"


int scon_sdl_base_select(void)
{
    int exit_status = SCON_SUCCESS;
    scon_sdl_base_component_t *best_component = NULL;
    scon_sdl_base_module_t *best_module = NULL;

    /*
     * Select the best component
     */
    if (SCON_SUCCESS != scon_mca_base_select("sdl",
                                             scon_sdl_base_framework.framework_output,
                                             &scon_sdl_base_framework.framework_components,
                                             (scon_mca_base_module_t **) &best_module,
                                             (scon_mca_base_component_t **) &best_component, NULL) ) {
        /* This will only happen if no component was selected */
        exit_status = SCON_ERROR;
        goto cleanup;
    }

    /* Save the winner */
    scon_sdl_base_selected_component = best_component;
    scon_sdl = best_module;

 cleanup:
    return exit_status;
}
