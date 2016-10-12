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
 * Pt2Pt Framework maintenence interface
 *
 *
 *
 */

#ifndef SCON_PT2PT_BASE_H
#define SCON_PT2PT_BASE_H

#include <scon_config.h>
#include "src/mca/pt2pt/pt2pt.h"
#include "src/mca/base/base.h"

extern scon_mca_base_framework_t scon_pt2pt_base_framework;
extern scon_pt2pt_base_component_t* scon_pt2pt_base_selected_component;
extern scon_pt2pt_base_module_t *scon_pt2pt;
/* select a component */
int scon_pt2pt_base_select(void);

END_C_DECLS

#endif /* SCON_PT2PT_BASE_H */
