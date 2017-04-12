/*
 * Copyright (c) 2006-2013 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2007-2010 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2016-2017      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#ifndef SCON_SINSTALLDIRS_BASE_H
#define SCON_SINSTALLDIRS_BASE_H

#include <src/include/scon_config.h>
#include "src/mca/base/scon_mca_base_framework.h"
#include "src/mca/sinstalldirs/sinstalldirs.h"

/*
 * Global functions for MCA overall sinstalldirs open and close
 */
BEGIN_C_DECLS

/**
 * Framework structure declaration
 */
extern scon_mca_base_framework_t scon_sinstalldirs_base_framework;

/* Just like scon_sinstall_dirs_expand() (see sinstalldirs.h), but will
   also insert the value of the environment variable $SCON_DESTDIR, if
   it exists/is set.  This function should *only* be used during the
   setup routines of sinstalldirs. */
char * scon_sinstall_dirs_expand_setup(const char* input);

END_C_DECLS

#endif /* SCON_BASE_SINSTALLDIRS_H */
