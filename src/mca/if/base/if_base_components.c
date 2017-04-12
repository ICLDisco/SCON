/*
 * Copyright (c) 2010-2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015-2017     Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include "scon_config.h"

#include "src/util/error.h"
#include "src/util/output.h"
#include "src/mca/mca.h"
#include "src/mca/if/if.h"
#include "src/mca/if/base/base.h"
#include "src/mca/if/base/static-components.h"

/* instantiate the global list of interfaces */
scon_list_t scon_if_list = {{0}};
bool scon_if_do_not_resolve = false;
bool scon_if_retain_loopback = false;

static int scon_if_base_register (scon_mca_base_register_flag_t flags);
static int scon_if_base_open (scon_mca_base_open_flag_t flags);
static int scon_if_base_close(void);
static void scon_if_construct(scon_if_t *obj);

static bool frameopen = false;

/* instance the scon_if_t object */
SCON_CLASS_INSTANCE(scon_if_t, scon_list_item_t, scon_if_construct, NULL);

SCON_MCA_BASE_FRAMEWORK_DECLARE(scon, if, NULL, scon_if_base_register, scon_if_base_open, scon_if_base_close,
                           mca_if_base_static_components, 0);

static int scon_if_base_register (scon_mca_base_register_flag_t flags)
{
    scon_if_do_not_resolve = false;
    (void) scon_mca_base_framework_var_register (&scon_if_base_framework, "do_not_resolve",
                                            "If nonzero, do not attempt to resolve interfaces",
                                            SCON_MCA_BASE_VAR_TYPE_BOOL, NULL, 0, SCON_MCA_BASE_VAR_FLAG_SETTABLE,
                                            SCON_INFO_LVL_9, SCON_MCA_BASE_VAR_SCOPE_ALL_EQ,
                                            &scon_if_do_not_resolve);

    scon_if_retain_loopback = false;
    (void) scon_mca_base_framework_var_register (&scon_if_base_framework, "retain_loopback",
                                            "If nonzero, retain loopback interfaces",
                                            SCON_MCA_BASE_VAR_TYPE_BOOL, NULL, 0, SCON_MCA_BASE_VAR_FLAG_SETTABLE,
                                            SCON_INFO_LVL_9, SCON_MCA_BASE_VAR_SCOPE_ALL_EQ,
                                            &scon_if_retain_loopback);

    return SCON_SUCCESS;
}


static int scon_if_base_open (scon_mca_base_open_flag_t flags)
{
    if (frameopen) {
        return SCON_SUCCESS;
    }
    frameopen = true;

    /* setup the global list */
    SCON_CONSTRUCT(&scon_if_list, scon_list_t);

    return scon_mca_base_framework_components_open(&scon_if_base_framework, flags);
}


static int scon_if_base_close(void)
{
    scon_list_item_t *item;

    if (!frameopen) {
        return SCON_SUCCESS;
    }

    while (NULL != (item = scon_list_remove_first(&scon_if_list))) {
        SCON_RELEASE(item);
    }
    SCON_DESTRUCT(&scon_if_list);

    return scon_mca_base_framework_components_close(&scon_if_base_framework, NULL);
}

static void scon_if_construct(scon_if_t *obj)
{
    memset(obj->if_name, 0, sizeof(obj->if_name));
    obj->if_index = -1;
    obj->if_kernel_index = (uint16_t) -1;
    obj->af_family = PF_UNSPEC;
    obj->if_flags = 0;
    obj->if_speed = 0;
    memset(&obj->if_addr, 0, sizeof(obj->if_addr));
    obj->if_mask = 0;
    obj->if_bandwidth = 0;
    memset(obj->if_mac, 0, sizeof(obj->if_mac));
    obj->ifmtu = 0;
}
