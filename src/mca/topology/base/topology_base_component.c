/*
 *
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>
#include "src/mca/topology/base/base.h"
#include "src/mca/topology/base/static-components.h"

/*
 * Globals
 */
scon_topology_component_t *scon_topology_base_selected_component = NULL;
int scon_topology_base_open(scon_mca_base_open_flag_t flags);

/*
 * Function for finding and opening either all MCA components,
 * or the one that was specifically requested via a MCA parameter.
 *
 * Note that we really don't need this function -- we could specify a
 * NULL pointer in the framework declare and the base would do this
 * exact same thing.  However, we need to have at least some
 * executable code in this file, or some linkers (cough cough OS X
 * cough cough) may not actually link in this .o file.
 */
int scon_topology_base_open(scon_mca_base_open_flag_t flags)
{
    /* Open up all available components */
    return scon_mca_base_framework_components_open(&scon_topology_base_framework, flags);
}

void scon_topology_base_convert_topoid_to_procid( scon_proc_t *route,
                                                   unsigned int route_rank,
                                                   scon_proc_t *target)
{
    /** TO DO : this translation will be different when the
     SCON topology gets updated and for multi job scons */
    route->rank = route_rank;
    strncpy(route->job_name, target->job_name, SCON_MAX_JOBLEN);
}
/* Framework Declaration */
SCON_MCA_BASE_FRAMEWORK_DECLARE(scon, topology, "Topology framework",
                                NULL /* register */,
                                scon_topology_base_open /* open */,
                                NULL /* close */,
                                mca_topology_base_static_components,
                                0);


static void topo_cons(scon_topo_t *ptr)
{

    SCON_CONSTRUCT(&ptr->relatives, scon_bitmap_t);
    scon_bitmap_init(&ptr->relatives, 8);
    ptr->my_id = SCON_TOPO_ID_INVALID;
    ptr->myparent_id = SCON_TOPO_ID_INVALID;
    ptr->mygrandparent_id = SCON_TOPO_ID_INVALID;
    ptr->mylifeline_id = SCON_TOPO_ID_INVALID;
}
static void topo_des(scon_topo_t *ptr)
{
    SCON_DESTRUCT(&ptr->relatives);
}
SCON_CLASS_INSTANCE(scon_topo_t,
                    scon_object_t,
                    topo_cons, topo_des);

static void tply_cons(scon_topology_t *ptr)
{

    SCON_CONSTRUCT(&ptr->my_topo, scon_topo_t);
    SCON_CONSTRUCT(&ptr->my_peers, scon_list_t);
}
static void tply_des(scon_topology_t *ptr)
{
    SCON_DESTRUCT(&ptr->my_topo);
    SCON_DESTRUCT(&ptr->my_peers);
}
SCON_CLASS_INSTANCE(scon_topology_t,
                    scon_object_t,
                    tply_cons, tply_des);
