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
#ifndef COLLECTIVES_DEFAULT_H
#define COLLECTIVES_DEFAULT_H

#include "scon_config.h"


#include "src/mca/collectives/collectives.h"

BEGIN_C_DECLS

/*
 * collectives default interfaces
 */
extern scon_collectives_base_component_t scon_collectives_default_component;
extern scon_collectives_module_t scon_collectives_default_module;

END_C_DECLS

#endif
