/* -*- C -*-
 *
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2011 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012-2013 Los Alamos National Security, Inc. All rights reserved.
 * Copyright (c) 2014-2017 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/**
 * @file
 *
 * Buffer management types.
 */

#ifndef SCON_BFROP_TYPES_H_
#define SCON_BFROP_TYPES_H_

#include <src/include/scon_config.h>


#include "src/class/scon_object.h"
#include "src/class/scon_pointer_array.h"
#include "src/class/scon_list.h"
#include "scon_common.h"

BEGIN_C_DECLS

//SCON_CLASS_DECLARATION (scon_buffer_t);
SCON_CLASS_DECLARATION(scon_regex_range_t);
SCON_CLASS_DECLARATION(scon_regex_value_t);

/* these classes are required by the regex code shared
 * between the client and server implementations - it
 * is put here so that both can access these objects */
typedef struct {
    scon_list_item_t super;
    int start;
    int cnt;
} scon_regex_range_t;

typedef struct {
    /* list object */
    scon_list_item_t super;
    char *prefix;
    char *suffix;
    int num_digits;
    scon_list_t ranges;
} scon_regex_value_t;

SCON_EXPORT static inline void scon_buffer_construct (scon_buffer_t* buffer)
{
    /** set the default buffer type */
    buffer->type = SCON_BFROP_BUFFER_NON_DESC;

    /* Make everything NULL to begin with */
    buffer->base_ptr = buffer->pack_ptr = buffer->unpack_ptr = NULL;
    buffer->bytes_allocated = buffer->bytes_used = 0;
}

SCON_EXPORT static inline void scon_buffer_destruct (scon_buffer_t* buffer)
{
    if (NULL != buffer->base_ptr) {
        free (buffer->base_ptr);
    }
}

END_C_DECLS

#endif /* SCON_BFROP_TYPES_H */
