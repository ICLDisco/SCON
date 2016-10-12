/*
 * Copyright (c) 2006-2007 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include <stdlib.h>
#include <string.h>

#include "scon_common.h"
#include "src/mca/pinstalldirs/pinstalldirs.h"

static int pinstalldirs_env_open(void);


scon_pinstalldirs_base_component_t mca_pinstalldirs_env_component = {
    /* First, the mca_component_t struct containing meta information
       about the component itself */
    {
        SCON_PINSTALLDIRS_BASE_VERSION_1_0_0,

        /* Component name and version */
        "env",
        SCON_MAJOR_VERSION,
        SCON_MINOR_VERSION,
        SCON_RELEASE_VERSION,

        /* Component open and close functions */
        pinstalldirs_env_open,
        NULL
    },
    {
        /* This component is checkpointable */
        SCON_MCA_BASE_METADATA_PARAM_CHECKPOINT
    },

    /* Next the scon_pinstall_dirs_t install_dirs_data information */
    {
        NULL,
    },
};


#define SET_FIELD(field, envname)                                         \
    do {                                                                  \
        char *tmp = getenv(envname);                                      \
         if (NULL != tmp && 0 == strlen(tmp)) {                           \
             tmp = NULL;                                                  \
         }                                                                \
         mca_pinstalldirs_env_component.install_dirs_data.field = tmp;     \
    } while (0)


static int
pinstalldirs_env_open(void)
{
    SET_FIELD(prefix, "SCON_INSTALL_PREFIX");
    SET_FIELD(exec_prefix, "SCON_EXEC_PREFIX");
    SET_FIELD(bindir, "SCON_BINDIR");
    SET_FIELD(sbindir, "SCON_SBINDIR");
    SET_FIELD(libexecdir, "SCON_LIBEXECDIR");
    SET_FIELD(datarootdir, "SCON_DATAROOTDIR");
    SET_FIELD(datadir, "SCON_DATADIR");
    SET_FIELD(sysconfdir, "SCON_SYSCONFDIR");
    SET_FIELD(sharedstatedir, "SCON_SHAREDSTATEDIR");
    SET_FIELD(localstatedir, "SCON_LOCALSTATEDIR");
    SET_FIELD(libdir, "SCON_LIBDIR");
    SET_FIELD(includedir, "SCON_INCLUDEDIR");
    SET_FIELD(infodir, "SCON_INFODIR");
    SET_FIELD(mandir, "SCON_MANDIR");
    SET_FIELD(scondatadir, "SCON_PKGDATADIR");
    SET_FIELD(sconlibdir, "SCON_PKGLIBDIR");
    SET_FIELD(sconincludedir, "SCON_PKGINCLUDEDIR");

    return SCON_SUCCESS;
}
