/*
 * Copyright (c) 2006-2007 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include "src/mca/pinstalldirs/pinstalldirs.h"
#include "src/mca/pinstalldirs/config/pinstall_dirs.h"

const scon_pinstalldirs_base_component_t mca_pinstalldirs_config_component = {
    /* First, the mca_component_t struct containing meta information
       about the component itself */
    {
        SCON_PINSTALLDIRS_BASE_VERSION_1_0_0,

        /* Component name and version */
        "config",
        SCON_MAJOR_VERSION,
        SCON_MINOR_VERSION,
        SCON_RELEASE_VERSION,

        /* Component open and close functions */
        NULL,
        NULL
    },
    {
        /* This component is Checkpointable */
        SCON_MCA_BASE_METADATA_PARAM_CHECKPOINT
    },

    {
        SCON_INSTALL_PREFIX,
        SCON_EXEC_PREFIX,
        SCON_BINDIR,
        SCON_SBINDIR,
        SCON_LIBEXECDIR,
        SCON_DATAROOTDIR,
        SCON_DATADIR,
        SCON_SYSCONFDIR,
        SCON_SHAREDSTATEDIR,
        SCON_LOCALSTATEDIR,
        SCON_LIBDIR,
        SCON_INCLUDEDIR,
        SCON_INFODIR,
        SCON_MANDIR,
        SCON_PKGDATADIR,
        SCON_PKGLIBDIR,
        SCON_PKGINCLUDEDIR
    }
};
