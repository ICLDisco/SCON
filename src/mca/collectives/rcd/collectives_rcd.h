/* -*- C -*-
 *
 * Copyright (c) 2011      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2014-2017      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */
#ifndef COLLECTIVES_RCD_H
#define COLLECTIVES_RCD_H

#include "scon_config.h"


#include "src/mca/collectives/collectives.h"

BEGIN_C_DECLS

/*
 * collectives rcd interfaces
 */
extern scon_collectives_base_component_t mca_collectives_rcd_component;
extern scon_collectives_module_t scon_collectives_rcd_module;

END_C_DECLS

#endif
