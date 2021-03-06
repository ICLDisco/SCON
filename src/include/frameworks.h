/*
 * This file is autogenerated by autogen.pl. Do not edit this file by hand.
 */
#ifndef SCON_FRAMEWORKS_H
#define SCON_FRAMEWORKS_H

#include <scon/mca/base/mca_base_framework.h>

extern scon_mca_base_framework_t scon_src_base_framework;
extern scon_mca_base_framework_t scon_collectives_base_framework;
extern scon_mca_base_framework_t scon_comm_base_framework;
extern scon_mca_base_framework_t scon_if_base_framework;
extern scon_mca_base_framework_t scon_pt2pt_base_framework;
extern scon_mca_base_framework_t scon_sdl_base_framework;
extern scon_mca_base_framework_t scon_sinstalldirs_base_framework;
extern scon_mca_base_framework_t scon_topology_base_framework;

static scon_mca_base_framework_t *scon_frameworks[] = {
    &scon_src_base_framework,
    &scon_collectives_base_framework,
    &scon_comm_base_framework,
    &scon_if_base_framework,
    &scon_pt2pt_base_framework,
    &scon_sdl_base_framework,
    &scon_sinstalldirs_base_framework,
    &scon_topology_base_framework,
    NULL
};

#endif /* SCON_FRAMEWORKS_H */

