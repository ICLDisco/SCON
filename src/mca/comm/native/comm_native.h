/* -*- C -*-
 *
 * Copyright (c) 2017      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */
#ifndef COMM_NATIVE_H
#define COMM_NATIVE_H

#include "scon_config.h"
#include "src/mca/comm/comm.h"

BEGIN_C_DECLS

/*
 * comm native interfaces
 */
SCON_EXPORT extern scon_comm_base_component_t scon_comm_native_component;
SCON_EXPORT extern scon_comm_module_t scon_comm_native_module;

END_C_DECLS

#endif
