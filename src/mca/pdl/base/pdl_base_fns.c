/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**
 * This file is a simple set of wrappers around the selected SCON PDL
 * component (it's a compile-time framework with, at most, a single
 * component; see pdl.h for details).
 */

#include <src/include/scon_config.h>

#include "scon_common.h"

#include "src/util/output.h"
#include "src/mca/pdl/base/base.h"


int scon_pdl_open(const char *fname,
                 bool use_ext, bool private_namespace,
                 scon_pdl_handle_t **handle, char **err_msg)
{
    *handle = NULL;

    if (NULL != scon_pdl && NULL != scon_pdl->open) {
        return scon_pdl->open(fname, use_ext, private_namespace,
                             handle, err_msg);
    }

    return SCON_ERR_NOT_SUPPORTED;
}

int scon_pdl_lookup(scon_pdl_handle_t *handle,
                   const char *symbol,
                   void **ptr, char **err_msg)
{
    if (NULL != scon_pdl && NULL != scon_pdl->lookup) {
        return scon_pdl->lookup(handle, symbol, ptr, err_msg);
    }

    return SCON_ERR_NOT_SUPPORTED;
}

int scon_pdl_close(scon_pdl_handle_t *handle)
{
    if (NULL != scon_pdl && NULL != scon_pdl->close) {
        return scon_pdl->close(handle);
    }

    return SCON_ERR_NOT_SUPPORTED;
}

int scon_pdl_foreachfile(const char *search_path,
                        int (*cb_func)(const char *filename, void *context),
                        void *context)
{
    if (NULL != scon_pdl && NULL != scon_pdl->foreachfile) {
       return scon_pdl->foreachfile(search_path, cb_func, context);
    }

    return SCON_ERR_NOT_SUPPORTED;
}
