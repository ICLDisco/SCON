/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2008 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2010      Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2016-2017 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/** @file
 *
 */

#ifndef SCON_RING_BUFFER_H
#define SCON_RING_BUFFER_H

#include <src/include/scon_config.h>

#include "src/class/scon_object.h"
#include "src/util/output.h"

BEGIN_C_DECLS

/**
 * dynamic pointer ring
 */
struct scon_ring_buffer_t {
    /** base class */
    scon_object_t super;
    /* head/tail indices */
    int head;
    int tail;
    /** size of list, i.e. number of elements in addr */
    int size;
    /** pointer to ring */
    char **addr;
};
/**
 * Convenience typedef
 */
typedef struct scon_ring_buffer_t scon_ring_buffer_t;
/**
 * Class declaration
 */
SCON_CLASS_DECLARATION(scon_ring_buffer_t);

/**
 * Initialize the ring buffer, defining its size.
 *
 * @param ring Pointer to a ring buffer (IN/OUT)
 * @param size The number of elements in the ring (IN)
 *
 * @return SCON_SUCCESS if all initializations were succesful. Otherwise,
 *  the error indicate what went wrong in the function.
 */
int scon_ring_buffer_init(scon_ring_buffer_t* ring, int size);

/**
 * Push an item onto the ring buffer, displacing the oldest
 * item on the ring if the ring is full
 *
 * @param ring Pointer to ring (IN)
 * @param ptr Pointer value (IN)
 *
 * @return Pointer to displaced item, NULL if ring
 *         is not yet full
 */
void* scon_ring_buffer_push(scon_ring_buffer_t *ring, void *ptr);


/**
 * Pop an item off of the ring. The oldest entry on the ring will be
 * returned. If nothing on the ring, NULL is returned.
 *
 * @param ring          Pointer to ring (IN)
 *
 * @return Error code.  NULL indicates an error.
 */

void* scon_ring_buffer_pop(scon_ring_buffer_t *ring);

/*
 * Access an element of the ring, without removing it, indexed
 * starting at the tail - a value of -1 will return the element
 * at the head of the ring
 */
void* scon_ring_buffer_poke(scon_ring_buffer_t *ring, int i);

END_C_DECLS

#endif /* SCON_RING_BUFFER_H */
