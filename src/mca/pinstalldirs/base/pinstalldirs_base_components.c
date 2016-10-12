/*
 * Copyright (c) 2006-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include <src/include/scon_config.h>

#include "scon_common.h"
#include "src/mca/mca.h"
#include "src/mca/pinstalldirs/pinstalldirs.h"
#include "src/mca/pinstalldirs/base/base.h"
#include "src/mca/pinstalldirs/base/static-components.h"

scon_pinstall_dirs_t scon_pinstall_dirs = {0};

#define CONDITIONAL_COPY(target, origin, field)                 \
    do {                                                        \
        if (origin.field != NULL && target.field == NULL) {     \
            target.field = origin.field;                        \
        }                                                       \
    } while (0)

static int
scon_pinstalldirs_base_open(scon_mca_base_open_flag_t flags)
{
    scon_mca_base_component_list_item_t *component_item;
    int ret;

    ret = scon_mca_base_framework_components_open(&scon_pinstalldirs_base_framework, flags);
    if (SCON_SUCCESS != ret) {
        return ret;
    }

    SCON_LIST_FOREACH(component_item, &scon_pinstalldirs_base_framework.framework_components, scon_mca_base_component_list_item_t) {
        const scon_pinstalldirs_base_component_t *component =
            (const scon_pinstalldirs_base_component_t *) component_item->cli_component;

        /* copy over the data, if something isn't already there */
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         prefix);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         exec_prefix);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         bindir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         sbindir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         libexecdir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         datarootdir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         datadir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         sysconfdir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         sharedstatedir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         localstatedir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         libdir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         includedir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         infodir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         mandir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         scondatadir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         sconlibdir);
        CONDITIONAL_COPY(scon_pinstall_dirs, component->install_dirs_data,
                         sconincludedir);
    }

    /* expand out all the fields */
    scon_pinstall_dirs.prefix =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.prefix);
    scon_pinstall_dirs.exec_prefix =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.exec_prefix);
    scon_pinstall_dirs.bindir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.bindir);
    scon_pinstall_dirs.sbindir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.sbindir);
    scon_pinstall_dirs.libexecdir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.libexecdir);
    scon_pinstall_dirs.datarootdir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.datarootdir);
    scon_pinstall_dirs.datadir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.datadir);
    scon_pinstall_dirs.sysconfdir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.sysconfdir);
    scon_pinstall_dirs.sharedstatedir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.sharedstatedir);
    scon_pinstall_dirs.localstatedir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.localstatedir);
    scon_pinstall_dirs.libdir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.libdir);
    scon_pinstall_dirs.includedir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.includedir);
    scon_pinstall_dirs.infodir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.infodir);
    scon_pinstall_dirs.mandir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.mandir);
    scon_pinstall_dirs.scondatadir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.scondatadir);
    scon_pinstall_dirs.sconlibdir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.sconlibdir);
    scon_pinstall_dirs.sconincludedir =
        scon_pinstall_dirs_expand_setup(scon_pinstall_dirs.sconincludedir);

#if 0
    fprintf(stderr, "prefix:         %s\n", scon_pinstall_dirs.prefix);
    fprintf(stderr, "exec_prefix:    %s\n", scon_pinstall_dirs.exec_prefix);
    fprintf(stderr, "bindir:         %s\n", scon_pinstall_dirs.bindir);
    fprintf(stderr, "sbindir:        %s\n", scon_pinstall_dirs.sbindir);
    fprintf(stderr, "libexecdir:     %s\n", scon_pinstall_dirs.libexecdir);
    fprintf(stderr, "datarootdir:    %s\n", scon_pinstall_dirs.datarootdir);
    fprintf(stderr, "datadir:        %s\n", scon_pinstall_dirs.datadir);
    fprintf(stderr, "sysconfdir:     %s\n", scon_pinstall_dirs.sysconfdir);
    fprintf(stderr, "sharedstatedir: %s\n", scon_pinstall_dirs.sharedstatedir);
    fprintf(stderr, "localstatedir:  %s\n", scon_pinstall_dirs.localstatedir);
    fprintf(stderr, "libdir:         %s\n", scon_pinstall_dirs.libdir);
    fprintf(stderr, "includedir:     %s\n", scon_pinstall_dirs.includedir);
    fprintf(stderr, "infodir:        %s\n", scon_pinstall_dirs.infodir);
    fprintf(stderr, "mandir:         %s\n", scon_pinstall_dirs.mandir);
    fprintf(stderr, "pkgdatadir:     %s\n", scon_pinstall_dirs.pkgdatadir);
    fprintf(stderr, "pkglibdir:      %s\n", scon_pinstall_dirs.pkglibdir);
    fprintf(stderr, "pkgincludedir:  %s\n", scon_pinstall_dirs.pkgincludedir);
#endif

    /* NTH: Is it ok not to close the components? If not we can add a flag
       to mca_base_framework_components_close to indicate not to deregister
       variable groups */
    return SCON_SUCCESS;
}


static int
scon_pinstalldirs_base_close(void)
{
    free(scon_pinstall_dirs.prefix);
    free(scon_pinstall_dirs.exec_prefix);
    free(scon_pinstall_dirs.bindir);
    free(scon_pinstall_dirs.sbindir);
    free(scon_pinstall_dirs.libexecdir);
    free(scon_pinstall_dirs.datarootdir);
    free(scon_pinstall_dirs.datadir);
    free(scon_pinstall_dirs.sysconfdir);
    free(scon_pinstall_dirs.sharedstatedir);
    free(scon_pinstall_dirs.localstatedir);
    free(scon_pinstall_dirs.libdir);
    free(scon_pinstall_dirs.includedir);
    free(scon_pinstall_dirs.infodir);
    free(scon_pinstall_dirs.mandir);
    free(scon_pinstall_dirs.scondatadir);
    free(scon_pinstall_dirs.sconlibdir);
    free(scon_pinstall_dirs.sconincludedir);
    memset (&scon_pinstall_dirs, 0, sizeof (scon_pinstall_dirs));

    return scon_mca_base_framework_components_close (&scon_pinstalldirs_base_framework, NULL);
}

/* Declare the pinstalldirs framework */
SCON_MCA_BASE_FRAMEWORK_DECLARE(scon, pinstalldirs, NULL, NULL, scon_pinstalldirs_base_open,
                                scon_pinstalldirs_base_close, mca_pinstalldirs_base_static_components,
                                SCON_MCA_BASE_FRAMEWORK_FLAG_NOREGISTER | SCON_MCA_BASE_FRAMEWORK_FLAG_NO_DSO);
