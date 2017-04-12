/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2014 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2013-2017 Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "scon_config.h"
#include <scon_common.h>
#include <scon.h>

#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/pt2pt/base/base.h"
#include "src/mca/pt2pt/pt2pt.h"
#include "src/mca/comm/base/base.h"
#include "src/util/name_fns.h"
#include "src/util/argv.h"

static void process_uri(char *uri);
/**
 * Obtain a uri for initial connection purposes
 *
 * this function will loop across all active pt2pt components/modules,
 * letting each add to the uri string. An error will be returned
 * if NO component can successfully provide a contact.
 *
 * Note: since there is a limit to what an OS will allow on a cmd line, we
 * impose a limit on the length of the resulting uri via an MCA param. The
 * default value of -1 implies unlimited - however, users with large numbers
 * of interfaces on their nodes may wish to restrict the size.
 */
void scon_pt2pt_base_get_contact_info(char **uri)
{
    char *turi, *final=NULL, *tmp;
    size_t len = 0;
    int rc=SCON_SUCCESS;
    bool one_added = false;
    scon_mca_base_component_list_item_t *cli;
    scon_pt2pt_base_component_t *component;

    /* start with our process name */
    if (SCON_SUCCESS != (rc = scon_util_convert_process_name_to_string(&final, SCON_PROC_MY_NAME))) {
        SCON_ERROR_LOG(rc);
        goto unblock;
    }
    len = strlen(final);

    /* loop across all available modules to get their input
     * up to the max length
     */
    SCON_LIST_FOREACH(cli, &scon_pt2pt_base.actives, scon_mca_base_component_list_item_t) {
        component = (scon_pt2pt_base_component_t*)cli->cli_component;
        /* ask the component for its input, obtained when it
         * opened its modules
         */
        if (NULL == component->get_addr) {
            /* doesn't support this ability */
            continue;
        }
        /* the components operate within our event base, so we
         * can directly call their get_uri function to get the
         * pointer to the uri - this is not a copy, so
         * do NOT free it!
         */
        turi = component->get_addr();
        if (NULL != turi) {
            /* check overall length for limits */
            if (0 < scon_pt2pt_base.max_uri_length &&
                scon_pt2pt_base.max_uri_length < (int)(len + strlen(turi))) {
                /* cannot accept the payload */
                continue;
            }
            /* add new value to final one */
            asprintf(&tmp, "%s;%s", final, turi);
            free(turi);
            free(final);
            final = tmp;
            len = strlen(final);
            /* flag that at least one contributed */
            one_added = true;
        }
    }

    if (!one_added) {
        /* nobody could contribute */
        if (NULL != final) {
            free(final);
            final = NULL;
        }
    }

 unblock:
    *uri = final;
}

/**
 * This function will loop
 * across all pt2pt components, letting each look at the uri and extract
 * info from it if it can. An error is to be returned if NO component
 * can successfully extract a contact.
 */

void scon_pt2pt_base_set_contact_info(char *uri)
{
    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s: set_addr to uri %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (NULL == uri) ? "NULL" : uri);

    /* if the request doesn't contain a URI, then we
     * have an error
     */
    if (NULL == uri) {
        scon_output(0, "%s: NULL URI", SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        return;
    }
    process_uri(uri);
}

static void process_uri(char *uri)
{
    scon_proc_t peer;
    char *cptr;
    scon_mca_base_component_list_item_t *cli;
    scon_pt2pt_base_component_t *component;
    char **uris=NULL;
    int rc;
    uint64_t proc_name_ui64;
    scon_pt2pt_base_peer_t *pr;

    /* find the first semi-colon in the string */
    cptr = strchr(uri, ';');
    if (NULL == cptr) {
        /* got a problem - there must be at least two fields,
         * the first containing the process name of our peer
         * and all others containing the OOB contact info
         */
        SCON_ERROR_LOG(SCON_ERR_BAD_PARAM);
        return;
    }
    *cptr = '\0';
    cptr++;

    /* the first field is the process name, so convert it */
    scon_util_convert_string_to_process_name(&peer, uri);

    /* if the peer is us, no need to go further as we already
     * know our own contact info
     */
    if (SCON_EQUAL == scon_util_compare_name_fields(SCON_NS_CMP_ALL, SCON_PROC_MY_NAME, &peer)) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "%s:set contact info peer %s is me",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&peer));
        return;
    }

    /* split the rest of the uri into component parts */
    uris = scon_argv_split(cptr, ';');

    /* get the peer object for this process */
    proc_name_ui64 = scon_util_convert_process_name_to_uint64 (&peer);
    if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_base.peers,
                                                         proc_name_ui64, (void**)&pr) ||
        NULL == pr) {
        pr = SCON_NEW(scon_pt2pt_base_peer_t);
        scon_output(0, "process_uri %s setting hash table %p, key %llu, value %p",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         (void*)&scon_pt2pt_base.peers,
                         proc_name_ui64, (void*) pr);
        if (SCON_SUCCESS != (rc = scon_hash_table_set_value_uint64(&scon_pt2pt_base.peers,
                                  proc_name_ui64, (void*)pr))) {
            SCON_ERROR_LOG(rc);
            scon_argv_free(uris);
            return;
        }
    }

    /* loop across all available components and let them extract
     * whatever piece(s) of the uri they find relevant - they
     * are all operating on our event base, so we can just
     * directly call their functions
     */
    rc = SCON_ERR_UNREACH;
    SCON_LIST_FOREACH(cli, &scon_pt2pt_base.actives, scon_mca_base_component_list_item_t) {
        component = (scon_pt2pt_base_component_t*)cli->cli_component;
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "%s:set_addr checking if peer %s is reachable via component %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&peer), component->base_version.scon_mca_component_name);
        if (NULL != component->set_addr) {
            if (SCON_SUCCESS == component->set_addr(&peer, uris)) {
                /* this component found reachable addresses
                 * in the uris
                 */
                scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                    "%s: peer %s is reachable via component %s",
                                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                    SCON_PRINT_PROC(&peer),
                                    component->base_version.scon_mca_component_name);
                scon_bitmap_set_bit(&pr->addressable, component->idx);
                pr->module = scon_pt2pt_base_get_module(
                                 component->base_version.scon_mca_component_name);
            } else {
                scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                    "%s: peer %s is NOT reachable via component %s",
                                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                    SCON_PRINT_PROC(&peer),
                                    component->base_version.scon_mca_component_name);
            }
        }
    }
    scon_argv_free(uris);
}
