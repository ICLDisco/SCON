/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2014 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006      Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2008-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2009      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2010-2014 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2014      Hochschule Esslingen.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2015      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "scon_config.h"

#include "src/include/scon_types.h"
#include "src/mca/base/scon_mca_base_var.h"
#include "src/runtime/scon_rte.h"
#include "src/util/timings.h"


static bool scon_register_done = false;
char *scon_net_private_ipv4 = NULL;

scon_status_t scon_register_params(void)
{
    int ret;

    if (scon_register_done) {
        return SCON_SUCCESS;
    }

    scon_register_done = true;
    scon_net_private_ipv4 = "10.0.0.0/8;172.16.0.0/12;192.168.0.0/16;169.254.0.0/16";
    ret = scon_mca_base_var_register ("scon", "scon", "net", "private_ipv4",
                                      "Semicolon-delimited list of CIDR notation entries specifying what networks are considered \"private\" (default value based on RFC1918 and RFC3330)",
                                      SCON_MCA_BASE_VAR_TYPE_STRING, NULL, 0, SCON_MCA_BASE_VAR_FLAG_SETTABLE,
                                      SCON_INFO_LVL_3, SCON_MCA_BASE_VAR_SCOPE_ALL_EQ,
                                      &scon_net_private_ipv4);
    if (0 > ret) {
        return ret;
    }

    return SCON_SUCCESS;
}

scon_status_t scon_deregister_params(void)
{
    scon_register_done = false;

    return SCON_SUCCESS;
}
