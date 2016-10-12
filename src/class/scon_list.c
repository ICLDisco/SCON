/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2007 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Voltaire All rights reserved.
 * Copyright (c) 2013-2015 Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>
#include "include/scon.h"
#include "src/class/scon_list.h"

/*
 *  List classes
 */

static void scon_list_item_construct(scon_list_item_t*);
static void scon_list_item_destruct(scon_list_item_t*);

SCON_CLASS_INSTANCE(
    scon_list_item_t,
    scon_object_t,
    scon_list_item_construct,
    scon_list_item_destruct
);

static void scon_list_construct(scon_list_t*);
static void scon_list_destruct(scon_list_t*);

SCON_CLASS_INSTANCE(
    scon_list_t,
    scon_object_t,
    scon_list_construct,
    scon_list_destruct
);


/*
 *
 *      scon_list_link_item_t interface
 *
 */

static void scon_list_item_construct(scon_list_item_t *item)
{
    item->scon_list_next = item->scon_list_prev = NULL;
    item->item_free = 1;
#if SCON_ENABLE_DEBUG
    item->scon_list_item_refcount = 0;
    item->scon_list_item_belong_to = NULL;
#endif
}

static void scon_list_item_destruct(scon_list_item_t *item)
{
#if SCON_ENABLE_DEBUG
    assert( 0 == item->scon_list_item_refcount );
    assert( NULL == item->scon_list_item_belong_to );
#endif  /* SCON_ENABLE_DEBUG */
}


/*
 *
 *      scon_list_list_t interface
 *
 */

static void scon_list_construct(scon_list_t *list)
{
#if SCON_ENABLE_DEBUG
    /* These refcounts should never be used in assertions because they
       should never be removed from this list, added to another list,
       etc.  So set them to sentinel values. */

    SCON_CONSTRUCT( &(list->scon_list_sentinel), scon_list_item_t );
    list->scon_list_sentinel.scon_list_item_refcount  = 1;
    list->scon_list_sentinel.scon_list_item_belong_to = list;
#endif

    list->scon_list_sentinel.scon_list_next = &list->scon_list_sentinel;
    list->scon_list_sentinel.scon_list_prev = &list->scon_list_sentinel;
    list->scon_list_length = 0;
}


/*
 * Reset all the pointers to be NULL -- do not actually destroy
 * anything.
 */
static void scon_list_destruct(scon_list_t *list)
{
    scon_list_construct(list);
}


/*
 * Insert an item at a specific place in a list
 */
bool scon_list_insert(scon_list_t *list, scon_list_item_t *item, long long idx)
{
    /* Adds item to list at index and retains item. */
    int     i;
    volatile scon_list_item_t *ptr, *next;

    if ( idx >= (long long)list->scon_list_length ) {
        return false;
    }

    if ( 0 == idx )
    {
        scon_list_prepend(list, item);
    } else {
#if SCON_ENABLE_DEBUG
        /* Spot check: ensure that this item is previously on no
           lists */

        assert(0 == item->scon_list_item_refcount);
#endif
        /* pointer to element 0 */
        ptr = list->scon_list_sentinel.scon_list_next;
        for ( i = 0; i < idx-1; i++ )
            ptr = ptr->scon_list_next;

        next = ptr->scon_list_next;
        item->scon_list_next = next;
        item->scon_list_prev = ptr;
        next->scon_list_prev = item;
        ptr->scon_list_next = item;

#if SCON_ENABLE_DEBUG
        /* Spot check: ensure this item is only on the list that we
           just insertted it into */

        item->scon_list_item_refcount += 1;
        assert(1 == item->scon_list_item_refcount);
        item->scon_list_item_belong_to = list;
#endif
    }

    list->scon_list_length++;
    return true;
}


static
void
scon_list_transfer(scon_list_item_t *pos, scon_list_item_t *begin,
                   scon_list_item_t *end)
{
    volatile scon_list_item_t *tmp;

    if (pos != end) {
        /* remove [begin, end) */
        end->scon_list_prev->scon_list_next = pos;
        begin->scon_list_prev->scon_list_next = end;
        pos->scon_list_prev->scon_list_next = begin;

        /* splice into new position before pos */
        tmp = pos->scon_list_prev;
        pos->scon_list_prev = end->scon_list_prev;
        end->scon_list_prev = begin->scon_list_prev;
        begin->scon_list_prev = tmp;
#if SCON_ENABLE_DEBUG
        {
            volatile scon_list_item_t* item = begin;
            while( pos != item ) {
                item->scon_list_item_belong_to = pos->scon_list_item_belong_to;
                item = item->scon_list_next;
                assert(NULL != item);
            }
        }
#endif  /* SCON_ENABLE_DEBUG */
    }
}


void
scon_list_join(scon_list_t *thislist, scon_list_item_t *pos,
               scon_list_t *xlist)
{
    if (0 != scon_list_get_size(xlist)) {
        scon_list_transfer(pos, scon_list_get_first(xlist),
                           scon_list_get_end(xlist));

        /* fix the sizes */
        thislist->scon_list_length += xlist->scon_list_length;
        xlist->scon_list_length = 0;
    }
}


void
scon_list_splice(scon_list_t *thislist, scon_list_item_t *pos,
                 scon_list_t *xlist, scon_list_item_t *first,
                 scon_list_item_t *last)
{
    size_t change = 0;
    scon_list_item_t *tmp;

    if (first != last) {
        /* figure out how many things we are going to move (have to do
         * first, since last might be end and then we wouldn't be able
         * to run the loop)
         */
        for (tmp = first ; tmp != last ; tmp = scon_list_get_next(tmp)) {
            change++;
        }

        scon_list_transfer(pos, first, last);

        /* fix the sizes */
        thislist->scon_list_length += change;
        xlist->scon_list_length -= change;
    }
}


int scon_list_sort(scon_list_t* list, scon_list_item_compare_fn_t compare)
{
    scon_list_item_t* item;
    scon_list_item_t** items;
    size_t i, index=0;

    if (0 == list->scon_list_length) {
        return SCON_SUCCESS;
    }
    items = (scon_list_item_t**)malloc(sizeof(scon_list_item_t*) *
                                       list->scon_list_length);

    if (NULL == items) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    while(NULL != (item = scon_list_remove_first(list))) {
        items[index++] = item;
    }

    qsort(items, index, sizeof(scon_list_item_t*),
          (int(*)(const void*,const void*))compare);
    for (i=0; i<index; i++) {
        scon_list_append(list,items[i]);
    }
    free(items);
    return SCON_SUCCESS;
}
