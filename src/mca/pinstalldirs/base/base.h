/*
 * Copyright (c) 2006-2013 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2007-2010 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#ifndef SCON_PINSTALLDIRS_BASE_H
#define SCON_PINSTALLDIRS_BASE_H

#include <src/include/scon_config.h>
#include "src/mca/base/scon_mca_base_framework.h"
#include "src/mca/pinstalldirs/pinstalldirs.h"

/*
 * Global functions for MCA overall pinstalldirs open and close
 */
BEGIN_C_DECLS

/**
 * Framework structure declaration
 */
extern scon_mca_base_framework_t scon_pinstalldirs_base_framework;

/* Just like scon_pinstall_dirs_expand() (see pinstalldirs.h), but will
   also insert the value of the environment variable $SCON_DESTDIR, if
   it exists/is set.  This function should *only* be used during the
   setup routines of pinstalldirs. */
char * scon_pinstall_dirs_expand_setup(const char* input);

END_C_DECLS

#endif /* SCON_BASE_PINSTALLDIRS_H */
