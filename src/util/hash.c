/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2011-2014 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2014-2017 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include <src/include/scon_config.h>

#include <src/include/scon_stdint.h>
#include <src/include/hash_string.h>

#include <string.h>

#include "src/include/scon_globals.h"
#include "src/class/scon_hash_table.h"
#include "src/class/scon_pointer_array.h"
#include "src/buffer_ops/buffer_ops.h"
#include "src/util/error.h"
#include "src/util/output.h"

#include "src/util/hash.h"

/**
 * Data for a particular scon process
 * The name association is maintained in the
 * proc_data hash table.
 */
typedef struct {
    /** Structure can be put on lists (including in hash tables) */
    scon_list_item_t super;
    /* List of scon_kval_t structures containing all data
       received from this process */
    scon_list_t data;
} scon_proc_data_t;
static void pdcon(scon_proc_data_t *p)
{
    SCON_CONSTRUCT(&p->data, scon_list_t);
}
static void pddes(scon_proc_data_t *p)
{
    SCON_LIST_DESTRUCT(&p->data);
}
static SCON_CLASS_INSTANCE(scon_proc_data_t,
                           scon_list_item_t,
                           pdcon, pddes);

static scon_kval_t* lookup_keyval(scon_list_t *data,
                                  const char *key);
static scon_proc_data_t* lookup_proc(scon_hash_table_t *jtable,
                                     uint64_t id, bool create);

scon_status_t scon_hash_store(scon_hash_table_t *table,
                              scon_rank_t rank, scon_kval_t *kin)
{
    scon_proc_data_t *proc_data;
    uint64_t id;
    scon_kval_t *hv;

    scon_output_verbose(10, scon_globals.debug_output,
                        "HASH:STORE rank %d key %s",
                        rank, kin->key);

    id = (uint64_t)rank;

    /* lookup the proc data object for this proc - create
     * it if we don't already have it */
    if (NULL == (proc_data = lookup_proc(table, id, true))) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    /* see if we already have this key-value */
    hv = lookup_keyval(&proc_data->data, kin->key);
    if (NULL != hv) {
        /* yes we do - so remove the current value
         * and replace it */
        scon_list_remove_item(&proc_data->data, &hv->super);
        SCON_RELEASE(hv);
    }
    SCON_RETAIN(kin);
    scon_list_append(&proc_data->data, &kin->super);

    return SCON_SUCCESS;
}

scon_status_t scon_hash_fetch(scon_hash_table_t *table, scon_rank_t rank,
                              const char *key, scon_value_t **kvs)
{
    scon_status_t rc = SCON_SUCCESS;
    scon_proc_data_t *proc_data;
    scon_kval_t *hv;
    uint64_t id;
    char *node;

    scon_output_verbose(10, scon_globals.debug_output,
                        "HASH:FETCH rank %d key %s",
                        rank, (NULL == key) ? "NULL" : key);

    id = (uint64_t)rank;

    /* - SCON_RANK_UNDEF should return following statuses
     * SCON_ERR_PROC_ENTRY_NOT_FOUND | SCON_SUCCESS
     * - specified rank can return following statuses
     * SCON_ERR_PROC_ENTRY_NOT_FOUND | SCON_ERR_NOT_FOUND | SCON_SUCCESS
     * special logic is basing on these statuses on a client and a server */
    if (SCON_RANK_UNDEF == rank) {
        rc = scon_hash_table_get_first_key_uint64(table, &id,
                (void**)&proc_data, (void**)&node);
        if (SCON_SUCCESS != rc) {
            scon_output_verbose(10, scon_globals.debug_output,
                                "HASH:FETCH proc data for rank %d not found",
                                rank);
            return SCON_ERR_PROC_ENTRY_NOT_FOUND;
        }
    }

    while (SCON_SUCCESS == rc) {
        proc_data = lookup_proc(table, id, false);
        if (NULL == proc_data) {
            scon_output_verbose(10, scon_globals.debug_output,
                                "HASH:FETCH proc data for rank %d not found",
                                rank);
            return SCON_ERR_PROC_ENTRY_NOT_FOUND;
        }

        /* if the key is NULL, then the user wants -all- data
         * put by the specified rank */
        if (NULL == key) {
            /* we will return the data as an array of scon_info_t
             * in the kvs scon_value_t */

        } else {
            /* find the value from within this proc_data object */
            hv = lookup_keyval(&proc_data->data, key);
            if (NULL != hv) {
                /* create the copy */
                if (SCON_SUCCESS != (rc = scon_bfrop.copy((void**)kvs, hv->value, SCON_VALUE))) {
                    SCON_ERROR_LOG(rc);
                    return rc;
                }
                break;
            } else if (SCON_RANK_UNDEF != rank) {
                scon_output_verbose(10, scon_globals.debug_output,
                                    "HASH:FETCH data for key %s not found", key);
                return SCON_ERR_NOT_FOUND;
            }
        }

        rc = scon_hash_table_get_next_key_uint64(table, &id,
                (void**)&proc_data, node, (void**)&node);
        if (SCON_SUCCESS != rc) {
            scon_output_verbose(10, scon_globals.debug_output,
                                "HASH:FETCH data for key %s not found", key);
            return SCON_ERR_PROC_ENTRY_NOT_FOUND;
        }
    }

    return rc;
}

scon_status_t scon_hash_fetch_by_key(scon_hash_table_t *table, const char *key,
                                     scon_rank_t *rank, scon_value_t **kvs, void **last)
{
    scon_status_t rc = SCON_SUCCESS;
    scon_proc_data_t *proc_data;
    scon_kval_t *hv;
    uint64_t id;
    char *node;
    static const char *key_r = NULL;

    if (key == NULL && (node = *last) == NULL) {
        return SCON_ERR_PROC_ENTRY_NOT_FOUND;
    }

    if (key == NULL && key_r == NULL) {
        return SCON_ERR_PROC_ENTRY_NOT_FOUND;
    }

    if (key) {
        rc = scon_hash_table_get_first_key_uint64(table, &id,
                (void**)&proc_data, (void**)&node);
        key_r = key;
    } else {
        rc = scon_hash_table_get_next_key_uint64(table, &id,
                (void**)&proc_data, node, (void**)&node);
    }

    scon_output_verbose(10, scon_globals.debug_output,
                        "HASH:FETCH BY KEY rank %d key %s",
                        (int)id, key_r);

    if (SCON_SUCCESS != rc) {
        scon_output_verbose(10, scon_globals.debug_output,
                            "HASH:FETCH proc data for key %s not found",
                            key_r);
        return SCON_ERR_PROC_ENTRY_NOT_FOUND;
    }

    /* find the value from within this proc_data object */
    hv = lookup_keyval(&proc_data->data, key_r);
    if (hv) {
        /* create the copy */
        if (SCON_SUCCESS != (rc = scon_bfrop.copy((void**)kvs, hv->value, SCON_VALUE))) {
            SCON_ERROR_LOG(rc);
            return rc;
        }
    } else {
        return SCON_ERR_NOT_FOUND;
    }

    *rank = (int)id;
    *last = node;

    return SCON_SUCCESS;
}

scon_status_t scon_hash_remove_data(scon_hash_table_t *table,
                                    scon_rank_t rank, const char *key)
{
    scon_status_t rc = SCON_SUCCESS;
    scon_proc_data_t *proc_data;
    scon_kval_t *kv;
    uint64_t id;
    char *node;

    id = (uint64_t)rank;

    /* if the rank is wildcard, we want to apply this to
     * all rank entries */
    if (SCON_RANK_UNDEF == rank) {
        rc = scon_hash_table_get_first_key_uint64(table, &id,
                (void**)&proc_data, (void**)&node);
        while (SCON_SUCCESS == rc) {
            if (NULL != proc_data) {
                if (NULL == key) {
                    SCON_RELEASE(proc_data);
                } else {
                    SCON_LIST_FOREACH(kv, &proc_data->data, scon_kval_t) {
                        if (0 == strcmp(key, kv->key)) {
                            scon_list_remove_item(&proc_data->data, &kv->super);
                            SCON_RELEASE(kv);
                            break;
                        }
                    }
                }
            }
            rc = scon_hash_table_get_next_key_uint64(table, &id,
                    (void**)&proc_data, node, (void**)&node);
        }
    }

    /* lookup the specified proc */
    if (NULL == (proc_data = lookup_proc(table, id, false))) {
        /* no data for this proc */
        return SCON_SUCCESS;
    }

    /* if key is NULL, remove all data for this proc */
    if (NULL == key) {
        while (NULL != (kv = (scon_kval_t*)scon_list_remove_first(&proc_data->data))) {
            SCON_RELEASE(kv);
        }
        /* remove the proc_data object itself from the jtable */
        scon_hash_table_remove_value_uint64(table, id);
        /* cleanup */
        SCON_RELEASE(proc_data);
        return SCON_SUCCESS;
    }

    /* remove this item */
    SCON_LIST_FOREACH(kv, &proc_data->data, scon_kval_t) {
        if (0 == strcmp(key, kv->key)) {
            scon_list_remove_item(&proc_data->data, &kv->super);
            SCON_RELEASE(kv);
            break;
        }
    }

    return SCON_SUCCESS;
}

/**
 * Find data for a given key in a given scon_list_t.
 */
static scon_kval_t* lookup_keyval(scon_list_t *data,
                                  const char *key)
{
    scon_kval_t *kv;

    SCON_LIST_FOREACH(kv, data, scon_kval_t) {
        if (0 == strcmp(key, kv->key)) {
            return kv;
        }
    }
    return NULL;
}


/**
 * Find proc_data_t container associated with given
 * scon_identifier_t.
 */
static scon_proc_data_t* lookup_proc(scon_hash_table_t *jtable,
                                     uint64_t id, bool create)
{
    scon_proc_data_t *proc_data = NULL;

    scon_hash_table_get_value_uint64(jtable, id, (void**)&proc_data);
    if (NULL == proc_data && create) {
        /* The proc clearly exists, so create a data structure for it */
        proc_data = SCON_NEW(scon_proc_data_t);
        if (NULL == proc_data) {
            scon_output(0, "scon:client:hash:lookup_scon_proc: unable to allocate proc_data_t\n");
            return NULL;
        }
        scon_hash_table_set_value_uint64(jtable, id, proc_data);
    }

    return proc_data;
}
