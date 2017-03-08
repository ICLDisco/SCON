/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015       Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015       Los Alamos National Security, Inc.  All rights
 *                          reserved.
 * Copyright (c) 2017      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "scon_config.h"

#include "scon_common.h"
#include "scon/mca/sdl/sdl.h"
#include "scon/mca/base/scon_mca_base_var.h"
#include "scon/util/argv.h"

#include "sdl_libltdl.h"


/*
 * Public string showing the sysinfo ompi_linux component version number
 */
const char *scon_sdl_slibltsdl_component_version_string =
    "SCON sdl slibltdl MCA component version " SCON_VERSION;


/*
 * Local functions
 */
static int slibltsdl_component_register(void);
static int slibltsdl_component_open(void);
static int slibltsdl_component_close(void);
static int slibltsdl_component_query(mca_base_module_t **module, int *priority);

/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

scon_sdl_slibltsdl_component_t mca_sdl_slibltsdl_component = {

    /* Fill in the mca_sdl_base_component_t */
    .base = {

        /* First, the mca_component_t struct containing meta information
           about the component itself */
        .base_version = {
            SCON_DL_BASE_VERSION_1_0_0,

            /* Component name and version */
            .mca_component_name = "slibltdl",
            MCA_BASE_MAKE_VERSION(component, SCON_MAJOR_VERSION, SCON_MINOR_VERSION,
                                  SCON_RELEASE_VERSION),

            /* Component functions */
            .mca_register_component_params = slibltsdl_component_register,
            .mca_open_component = slibltsdl_component_open,
            .mca_close_component = slibltsdl_component_close,
            .mca_query_component = slibltsdl_component_query,
        },

        .base_data = {
            /* The component is checkpoint ready */
            SCON_MCA_BASE_METADATA_PARAM_CHECKPOINT
        },

        /* The dl framework members */
        .priority = 50
    }

    /* Now fill in the slibltdl component-specific members */
};


static int slibltsdl_component_register(void)
{
    /* Register an info param indicating whether we have lt_dladvise
       support or not */
    bool supported = SCON_INT_TO_BOOL(SCON_DL_LIBLTDL_HAVE_LT_DLADVISE);
    mca_base_component_var_register(&mca_sdl_slibltsdl_component.base.base_version,
                                    "have_lt_dladvise",
                                    "Whether the version of slibltdl that this component is built against supports lt_dladvise functionality or not",
                                    MCA_BASE_VAR_TYPE_BOOL,
                                    NULL,
                                    0,
                                    MCA_BASE_VAR_FLAG_DEFAULT_ONLY,
                                    SCON_INFO_LVL_7,
                                    MCA_BASE_VAR_SCOPE_CONSTANT,
                                    &supported);

    return SCON_SUCCESS;
}

static int slibltsdl_component_open(void)
{
    if (lt_dlinit()) {
        return SCON_ERROR;
    }

#if SCON_DL_LIBLTDL_HAVE_LT_DLADVISE
    scon_sdl_slibltsdl_component_t *c = &mca_sdl_slibltsdl_component;

    if (lt_dladvise_init(&c->advise_private_noext)) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    if (lt_dladvise_init(&c->advise_private_ext) ||
        lt_dladvise_ext(&c->advise_private_ext)) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    if (lt_dladvise_init(&c->advise_public_noext) ||
        lt_dladvise_global(&c->advise_public_noext)) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    if (lt_dladvise_init(&c->advise_public_ext) ||
        lt_dladvise_global(&c->advise_public_ext) ||
        lt_dladvise_ext(&c->advise_public_ext)) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }
#endif

    return SCON_SUCCESS;
}


static int slibltsdl_component_close(void)
{
#if SCON_DL_LIBLTDL_HAVE_LT_DLADVISE
    scon_sdl_slibltsdl_component_t *c = &mca_sdl_slibltsdl_component;

    lt_dladvise_destroy(&c->advise_private_noext);
    lt_dladvise_destroy(&c->advise_private_ext);
    lt_dladvise_destroy(&c->advise_public_noext);
    lt_dladvise_destroy(&c->advise_public_ext);
#endif

    lt_dlexit();

    return SCON_SUCCESS;
}


static int slibltsdl_component_query(mca_base_module_t **module, int *priority)
{
    /* The priority value is somewhat meaningless here; by
       scon/mca/dl/configure.m4, there's at most one component
       available. */
    *priority = mca_sdl_slibltsdl_component.base.priority;
    *module = &scon_sdl_slibltsdl_module.super;

    return SCON_SUCCESS;
}
