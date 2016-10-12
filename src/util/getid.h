/*
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_GETID_H
#define SCON_GETID_H

#include <src/include/scon_config.h>
#include "include/scon_common.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

BEGIN_C_DECLS

/* lookup the effective uid and gid of a socket */
scon_status_t scon_util_getid(int sd, uid_t *uid, gid_t *gid);

END_C_DECLS

#endif /* SCON_PRINTF_H */

