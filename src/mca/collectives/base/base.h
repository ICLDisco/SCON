/*
 * Copyright (c) 2016     Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**
 * @file
 *
 * Collectives Framework maintenence interface
 *
 *
 *
 */

#ifndef SCON_COLLECTIVES_BASE_H
#define SCON_COLLECTIVES_BASE_H

#include <scon_config.h>
#include "src/mca/collectives/collectives.h"
#include "src/mca/base/base.h"

extern scon_mca_base_framework_t scon_collectives_base_framework;
extern scon_collectives_base_component_t* scon_collectives_base_selected_component;
extern scon_collectives_base_module_t *scon_collectives;
/* select a component */
int scon_collectives_base_select(void);

END_C_DECLS

#endif /* SCON_COLLECTIVES_BASE_H */
