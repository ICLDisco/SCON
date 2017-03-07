/*
 * Copyright (c) 2016      Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/** @file */

#include "scon_config.h"
#include "scon_common.h"

#include "src/util/argv.h"
#include "src/util/output.h"
#include "src/util/error.h"
#include "src/buffer_ops/buffer_ops.h"
#include "src/mca/topology/topology.h"
#include "src/util/name_fns.h"
#include "src/include/scon_globals.h"

#include "src/mca/comm/comm.h"
#include "src/mca/comm/base/comm_base_wireup.h"
#include "src/mca/comm/base/base.h"
#include "src/mca/pt2pt/base/base.h"


int scon_comm_base_get_wireup_info(scon_comm_scon_t *scon, scon_buffer_t *wireup)
{
    int rc;
    /* cycle through all members of the scon, adding each member contact info to the buffer */
    scon_member_t *mem;
    /* pack the number of members first */
    if (SCON_SUCCESS != (rc = scon_bfrop.pack(wireup, &scon->nmembers,
                              1, SCON_STD_CTR))) {
        SCON_ERROR_LOG(rc);
        return rc;
    }
    /* return local scon id of the member */
    SCON_LIST_FOREACH(mem, &scon->members, scon_member_t) {
        /* if this member doesn't have any contact info, ignore it */
        if (NULL == mem->mem_uri) {
            continue;
        }
        if (SCON_SUCCESS != (rc = scon_bfrop.pack(wireup, &mem->mem_uri, 1, SCON_STRING))) {
            SCON_ERROR_LOG(rc);
            return rc;
        }
    }

    return SCON_SUCCESS;
}

int scon_comm_base_update_wireup_info(scon_comm_scon_t *scon,
                                        scon_buffer_t* wireup)
{
    int32_t nmembers, cnt;
    char *mem_uri;
    int rc, i;
    /* unpack the data for each entry */
    cnt = 1;
    if(SCON_SUCCESS != (rc = scon_bfrop.unpack(wireup, &nmembers,
                             &cnt, SCON_STD_CTR))) {
        SCON_ERROR_LOG(rc);
        return rc;
    }
    for(i = 0; i < nmembers; i++) {
        cnt = 1;
        if(SCON_SUCCESS == (rc = scon_bfrop.unpack(wireup, &mem_uri, &cnt, SCON_STRING))) {
            scon_output_verbose(5, scon_comm_base_framework.framework_output,
                             "%s commom:base:update:wireup:info got uri %s",
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                             (NULL == mem_uri) ? "NULL" : mem_uri);

            if (NULL != mem_uri) {
                /* set the contact info into the hash table */
               scon_pt2pt_base_set_contact_info(mem_uri);
               free(mem_uri);
           }
        }
        else {
            SCON_ERROR_LOG(rc);
            return rc;
        }
    }

    /* TO DO we may need to update the topology
     * of the scon if a new member has joined
     */
#ifdef SCON_JOIN_ENABLE
    if (nmembers != scon->nmembers) {
        scon->nmembers =nmembers;
        /* if we changed it, then we better update the topology
         * plans so the  collectives work correctly.
         */
        scon->topology->module->update_topology(NULL);
    }
#endif
    return SCON_SUCCESS;
}
