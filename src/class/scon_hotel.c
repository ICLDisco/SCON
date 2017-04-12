/*
 * Copyright (c) 2012-2016 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012      Los Alamos National Security, LLC. All rights reserved
 * Copyright (c) 2015-2017 Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include <stdio.h>
#include <stddef.h>

#include SCON_EVENT_HEADER
#include "src/class/scon_hotel.h"


static void local_eviction_callback(int fd, short flags, void *arg)
{
    scon_hotel_room_eviction_callback_arg_t *eargs =
        (scon_hotel_room_eviction_callback_arg_t*) arg;
    void *occupant = eargs->hotel->rooms[eargs->room_num].occupant;

    /* Remove the occurpant from the room.

       Do not change this logic without also changing the same logic
       in scon_hotel_checkout() and
       scon_hotel_checkout_and_return_occupant(). */
    scon_hotel_t *hotel = eargs->hotel;
    scon_hotel_room_t *room = &(hotel->rooms[eargs->room_num]);
    room->occupant = NULL;
    hotel->last_unoccupied_room++;
    assert(hotel->last_unoccupied_room < hotel->num_rooms);
    hotel->unoccupied_rooms[hotel->last_unoccupied_room] = eargs->room_num;

    /* Invoke the user callback to tell them that they were evicted */
    hotel->evict_callback_fn(hotel,
                             eargs->room_num,
                             occupant);
}


int scon_hotel_init(scon_hotel_t *h, int num_rooms,
                    scon_event_base_t *evbase,
                    uint32_t eviction_timeout,
                    int eviction_event_priority,
                    scon_hotel_eviction_callback_fn_t evict_callback_fn)
{
    int i;

    /* Bozo check */
    if (num_rooms <= 0 ||
        NULL == evict_callback_fn) {
        return SCON_ERR_BAD_PARAM;
    }

    h->num_rooms = num_rooms;
    h->evbase = evbase;
    h->eviction_timeout.tv_usec = eviction_timeout % 1000000;
    h->eviction_timeout.tv_sec = eviction_timeout / 1000000;
    h->evict_callback_fn = evict_callback_fn;
    h->rooms = (scon_hotel_room_t*)malloc(num_rooms * sizeof(scon_hotel_room_t));
    if (NULL != evict_callback_fn) {
        h->eviction_args =
            (scon_hotel_room_eviction_callback_arg_t*)malloc(num_rooms * sizeof(scon_hotel_room_eviction_callback_arg_t));
    }
    h->unoccupied_rooms = (int*) malloc(num_rooms * sizeof(int));
    h->last_unoccupied_room = num_rooms - 1;

    for (i = 0; i < num_rooms; ++i) {
        /* Mark this room as unoccupied */
        h->rooms[i].occupant = NULL;

        /* Setup this room in the unoccupied index array */
        h->unoccupied_rooms[i] = i;

        /* Setup the eviction callback args */
        h->eviction_args[i].hotel = h;
        h->eviction_args[i].room_num = i;

        /* Create this room's event (but don't add it) */
        if (NULL != h->evbase) {
            event_assign(&(h->rooms[i].eviction_timer_event),
                         h->evbase,
                         -1, 0, local_eviction_callback,
                         &(h->eviction_args[i]));
        }
    }

    return SCON_SUCCESS;
}

static void constructor(scon_hotel_t *h)
{
    h->num_rooms = 0;
    h->evbase = NULL;
    h->eviction_timeout.tv_sec = 0;
    h->eviction_timeout.tv_usec = 0;
    h->evict_callback_fn = NULL;
    h->rooms = NULL;
    h->eviction_args = NULL;
    h->unoccupied_rooms = NULL;
    h->last_unoccupied_room = -1;
}

static void destructor(scon_hotel_t *h)
{
    int i;

    /* Go through all occupied rooms and destroy their events */
    if (NULL != h->evbase) {
        for (i = 0; i < h->num_rooms; ++i) {
            if (NULL != h->rooms[i].occupant) {
                event_del(&(h->rooms[i].eviction_timer_event));
            }
        }
    }

    if (NULL != h->rooms) {
        free(h->rooms);
    }
    if (NULL != h->eviction_args) {
        free(h->eviction_args);
    }
    if (NULL != h->unoccupied_rooms) {
        free(h->unoccupied_rooms);
    }
}

SCON_CLASS_INSTANCE(scon_hotel_t,
                    scon_object_t,
                    constructor,
                    destructor);
