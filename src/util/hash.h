/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012      Los Alamos National Security, Inc. All rights reserved.
 * Copyright (c) 2014-2015 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_HASH_H
#define SCON_HASH_H

#include <src/include/scon_config.h>


#include "src/buffer_ops/buffer_ops.h"
#include "src/class/scon_hash_table.h"

BEGIN_C_DECLS

/* store a value in the given hash table for the specified
 * rank index.*/
scon_status_t scon_hash_store(scon_hash_table_t *table,
                              scon_rank_t rank, scon_kval_t *kv);

/* Fetch the value for a specified key and rank from within
 * the given hash_table */
scon_status_t scon_hash_fetch(scon_hash_table_t *table, scon_rank_t rank,
                              const char *key, scon_value_t **kvs);

/* Fetch the value for a specified key from within
 * the given hash_table
 * It gets the next portion of data from table, where matching key.
 * To get the first data from table, function is called with key parameter as string.
 * Remaining data from table are obtained by calling function with a null pointer for the key parameter.*/
scon_status_t scon_hash_fetch_by_key(scon_hash_table_t *table, const char *key,
                                     scon_rank_t *rank, scon_value_t **kvs, void **last);

/* remove the specified key-value from the given hash_table.
 * A NULL key will result in removal of all data for the
 * given rank. A rank of SCON_RANK_WILDCARD indicates that
 * the specified key  is to be removed from the data for all
 * ranks in the table. Combining key=NULL with rank=SCON_RANK_WILDCARD
 * will therefore result in removal of all data from the
 * table */
scon_status_t scon_hash_remove_data(scon_hash_table_t *table,
                                    scon_rank_t rank, const char *key);

END_C_DECLS

#endif /* SCON_HASH_H */
