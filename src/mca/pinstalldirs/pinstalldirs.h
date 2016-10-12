/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2006-2015 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_MCA_PINSTALLDIRS_PINSTALLDIRS_H
#define SCON_MCA_PINSTALLDIRS_PINSTALLDIRS_H

#include <src/include/scon_config.h>

#include "src/mca/mca.h"
#include "src/mca/base/base.h"

BEGIN_C_DECLS

/*
 * Most of this file is just for ompi_info.  The only public interface
 * once scon_init has been called is the scon_pinstall_dirs structure
 * and the scon_pinstall_dirs_expand() call */
struct scon_pinstall_dirs_t {
    char* prefix;
    char* exec_prefix;
    char* bindir;
    char* sbindir;
    char* libexecdir;
    char* datarootdir;
    char* datadir;
    char* sysconfdir;
    char* sharedstatedir;
    char* localstatedir;
    char* libdir;
    char* includedir;
    char* infodir;
    char* mandir;

    /* Note that the following fields intentionally have an "ompi"
       prefix, even though they're down in the SCON layer.  This is
       not abstraction break because the "ompi" they're referring to
       is for the build system of the overall software tree -- not an
       individual project within that overall tree.

       Rather than using pkg{data,lib,includedir}, use our own
       ompi{data,lib,includedir}, which is always set to
       {datadir,libdir,includedir}/scon. This will keep us from
       having help files in prefix/share/open-rte when building
       without SCON, but in prefix/share/scon when building
       with SCON.

       Note that these field names match macros set by configure that
       are used in Makefile.am files.  E.g., project help files are
       installed into $(scondatadir). */
    char* scondatadir;
    char* sconlibdir;
    char* sconincludedir;
};
typedef struct scon_pinstall_dirs_t scon_pinstall_dirs_t;

/* Install directories.  Only available after scon_init() */
extern scon_pinstall_dirs_t scon_pinstall_dirs;

/**
 * Expand out path variables (such as ${prefix}) in the input string
 * using the current scon_pinstall_dirs structure */
char * scon_pinstall_dirs_expand(const char* input);


/**
 * Structure for pinstalldirs components.
 */
struct scon_pinstalldirs_base_component_2_0_0_t {
    /** MCA base component */
    scon_mca_base_component_t component;
    /** MCA base data */
    scon_mca_base_component_data_t component_data;
    /** install directories provided by the given component */
    scon_pinstall_dirs_t install_dirs_data;
};
/**
 * Convenience typedef
 */
typedef struct scon_pinstalldirs_base_component_2_0_0_t scon_pinstalldirs_base_component_t;

/*
 * Macro for use in components that are of type pinstalldirs
 */
#define SCON_PINSTALLDIRS_BASE_VERSION_1_0_0 \
    SCON_MCA_BASE_VERSION_1_0_0("pinstalldirs", 1, 0, 0)

END_C_DECLS

#endif /* SCON_MCA_PINSTALLDIRS_PINSTALLDIRS_H */
