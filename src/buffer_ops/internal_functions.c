/*
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
 * Copyright (c) 2015      Intel, Inc. All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>


#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/class/scon_pointer_array.h"

#include "src/buffer_ops/internal.h"

/**
 * Internal function that resizes (expands) an inuse buffer if
 * necessary.
 */
char* scon_bfrop_buffer_extend(scon_buffer_t *buffer, size_t bytes_to_add)
{
    size_t required, to_alloc;
    size_t pack_offset, unpack_offset;

    /* Check to see if we have enough space already */

    if ((buffer->bytes_allocated - buffer->bytes_used) >= bytes_to_add) {
        return buffer->pack_ptr;
    }

    required = buffer->bytes_used + bytes_to_add;
    if (required >= scon_bfrop_threshold_size) {
        to_alloc = ((required + scon_bfrop_threshold_size - 1)
                    / scon_bfrop_threshold_size) * scon_bfrop_threshold_size;
    } else {
        to_alloc = buffer->bytes_allocated;
        if(0 == to_alloc) {
            to_alloc = scon_bfrop_initial_size;
        }
        while(to_alloc < required) {
            to_alloc <<= 1;
        }
    }

    if (NULL != buffer->base_ptr) {
        pack_offset = ((char*) buffer->pack_ptr) - ((char*) buffer->base_ptr);
        unpack_offset = ((char*) buffer->unpack_ptr) -
            ((char*) buffer->base_ptr);
        buffer->base_ptr = (char*)realloc(buffer->base_ptr, to_alloc);
        memset(buffer->base_ptr + pack_offset, 0, to_alloc - buffer->bytes_allocated);
    } else {
        pack_offset = 0;
        unpack_offset = 0;
        buffer->bytes_used = 0;
        buffer->base_ptr = (char*)malloc(to_alloc);
        memset(buffer->base_ptr, 0, to_alloc);
    }

    if (NULL == buffer->base_ptr) {
        return NULL;
    }
    buffer->pack_ptr = ((char*) buffer->base_ptr) + pack_offset;
    buffer->unpack_ptr = ((char*) buffer->base_ptr) + unpack_offset;
    buffer->bytes_allocated = to_alloc;

    /* All done */

    return buffer->pack_ptr;
}

/*
 * Internal function that checks to see if the specified number of bytes
 * remain in the buffer for unpacking
 */
bool scon_bfrop_too_small(scon_buffer_t *buffer, size_t bytes_reqd)
{
    size_t bytes_remaining_packed;

    if (buffer->pack_ptr < buffer->unpack_ptr) {
        return true;
    }

    bytes_remaining_packed = buffer->pack_ptr - buffer->unpack_ptr;

    if (bytes_remaining_packed < bytes_reqd) {
        /* don't error log this - it could be that someone is trying to
         * simply read until the buffer is empty
         */
        return true;
    }

    return false;
}

scon_status_t scon_bfrop_store_data_type(scon_buffer_t *buffer, scon_data_type_t type)
{
    /* Lookup the pack function for the actual scon_data_type type and call it */
    return scon_bfrop_pack_datatype(buffer, &type, 1, SCON_DATA_TYPE);
}

scon_status_t scon_bfrop_get_data_type(scon_buffer_t *buffer, scon_data_type_t *type)
{
    int32_t n=1;
    return scon_bfrop_unpack_datatype(buffer, type, &n, SCON_DATA_TYPE);
}
