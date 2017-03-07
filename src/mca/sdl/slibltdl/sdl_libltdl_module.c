/*
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * Copyright (c) 2017      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "scon_config.h"

#include "scon/constants.h"
#include "scon/mca/sdl/sdl.h"

#include "sdl_libltdl.h"


static int slibltsdl_open(const char *fname, bool use_ext, bool private_namespace,
                          scon_sdl_handle_t **handle, char **err_msg)
{
    assert(handle);

    *handle = NULL;
    if (NULL != err_msg) {
        *err_msg = NULL;
    }

    lt_dlhandle local_handle;

#if SCON_DL_LIBLTDL_HAVE_LT_DLADVISE
    scon_sdl_slibltsdl_component_t *c = &mca_sdl_slibltsdl_component;

    if (use_ext && private_namespace) {
        local_handle = lt_dlopenadvise(fname, c->advise_private_ext);
    } else if (use_ext && !private_namespace) {
        local_handle = lt_dlopenadvise(fname, c->advise_public_ext);
    } else if (!use_ext && private_namespace) {
        local_handle = lt_dlopenadvise(fname, c->advise_private_noext);
    } else if (!use_ext && !private_namespace) {
        local_handle = lt_dlopenadvise(fname, c->advise_public_noext);
    }
#else
    if (use_ext) {
        local_handle = lt_dlopenext(fname);
    } else {
        local_handle = lt_dlopen(fname);
    }
#endif

    if (NULL != local_handle) {
        *handle = calloc(1, sizeof(scon_sdl_handle_t));
        (*handle)->ltsdl_handle = local_handle;

#if SCON_ENABLE_DEBUG
        if( NULL != fname ) {
            (*handle)->filename = strdup(fname);
        }
        else {
            (*handle)->filename = strdup("(null)");
        }
#endif

        return SCON_SUCCESS;
    }

    if (NULL != err_msg) {
        *err_msg = (char*) lt_dlerror();
    }
    return SCON_ERROR;
}


static int slibltsdl_lookup(scon_sdl_handle_t *handle, const char *symbol,
                            void **ptr, char **err_msg)
{
    assert(handle);
    assert(handle->ltsdl_handle);
    assert(symbol);
    assert(ptr);

    if (NULL != err_msg) {
        *err_msg = NULL;
    }

    *ptr = lt_dlsym(handle->ltsdl_handle, symbol);
    if (NULL != *ptr) {
        return SCON_SUCCESS;
    }

    if (NULL != err_msg) {
        *err_msg = (char*) lt_dlerror();
    }
    return SCON_ERROR;
}


static int slibltsdl_close(scon_sdl_handle_t *handle)
{
    assert(handle);

    int ret;
    ret = lt_dlclose(handle->ltsdl_handle);

#if SCON_ENABLE_DEBUG
    free(handle->filename);
#endif
    free(handle);

    return ret;
}

static int slibltsdl_foreachfile(const char *search_path,
                                 int (*func)(const char *filename, void *data),
                                 void *data)
{
    assert(search_path);
    assert(func);

    int ret = lt_dlforeachfile(search_path, func, data);
    return (0 == ret) ? SCON_SUCCESS : SCON_ERROR;
}


/*
 * Module definition
 */
scon_sdl_base_module_t scon_sdl_slibltsdl_module = {
    .open = slibltsdl_open,
    .lookup = slibltsdl_lookup,
    .close = slibltsdl_close,
    .foreachfile = slibltsdl_foreachfile
};
