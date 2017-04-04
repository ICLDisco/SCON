/*
 * Copyright (c) 2014-2016 Intel, Inc.  All rights reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <scon_config.h>
#include <scon_types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <pthread.h>
#include SCON_EVENT_HEADER

#include "src/class/scon_list.h"
#include "src/util/error.h"
#include "src/util/fd.h"

#include "src/runtime/scon_progress_threads.h"




static void ptcon(scon_thread_t *p)
{
    p->t_arg = NULL;
    p->t_handle = (pthread_t) -1;
}
SCON_CLASS_INSTANCE(scon_thread_t,
                  scon_object_t,
                  ptcon, NULL);

static int scon_thread_start(scon_thread_t *t)
{
    int rc;

    if (SCON_ENABLE_DEBUG) {
        if (NULL == t->t_run || t->t_handle != (pthread_t) -1) {
            return SCON_ERR_BAD_PARAM;
        }
    }

    rc = pthread_create(&t->t_handle, NULL, (void*(*)(void*)) t->t_run, t);

    return (rc == 0) ? SCON_SUCCESS : SCON_ERROR;
}


static int scon_thread_join(scon_thread_t *t, void **thr_return)
{
    int rc = pthread_join(t->t_handle, thr_return);
    t->t_handle = (pthread_t) -1;
    return (rc == 0) ? SCON_SUCCESS : SCON_ERROR;
}


/* create a tracking object for progress threads */
typedef struct {
    scon_list_item_t super;

    int refcount;
    char *name;

    scon_event_base_t *ev_base;

    /* This will be set to false when it is time for the progress
       thread to exit */
    volatile bool ev_active;

    /* This event will always be set on the ev_base (so that the
       ev_base is not empty!) */
    scon_event_t block;

    bool engine_constructed;
    scon_thread_t engine;
} scon_progress_tracker_t;

static void tracker_constructor(scon_progress_tracker_t *p)
{
    p->refcount = 1;  // start at one since someone created it
    p->name = NULL;
    p->ev_base = NULL;
    p->ev_active = false;
    p->engine_constructed = false;
}

static void tracker_destructor(scon_progress_tracker_t *p)
{
    scon_event_del(&p->block);

    if (NULL != p->name) {
        free(p->name);
    }
    if (NULL != p->ev_base) {
        scon_event_base_free(p->ev_base);
    }
    if (p->engine_constructed) {
        SCON_DESTRUCT(&p->engine);
    }
}

static SCON_CLASS_INSTANCE(scon_progress_tracker_t,
                          scon_list_item_t,
                          tracker_constructor,
                          tracker_destructor);

static bool inited = false;
static scon_list_t tracking;
static struct timeval long_timeout = {
    .tv_sec = 3600,
    .tv_usec = 0
};
static const char *shared_thread_name = "SCON-wide async progress thread";

/*
 * If this event is fired, just restart it so that this event base
 * continues to have something to block on.
 */
static void dummy_timeout_cb(int fd, short args, void *cbdata)
{
    scon_progress_tracker_t *trk = (scon_progress_tracker_t*)cbdata;

    scon_event_add(&trk->block, &long_timeout);
}

/*
 * Main for the progress thread
 */
static void* progress_engine(scon_object_t *obj)
{
    scon_thread_t *t = (scon_thread_t*)obj;
    scon_progress_tracker_t *trk = (scon_progress_tracker_t*)t->t_arg;

    while (trk->ev_active) {
        scon_event_loop(trk->ev_base, SCON_EVLOOP_ONCE);
    }

    return SCON_THREAD_CANCELLED;
}

static void stop_progress_engine(scon_progress_tracker_t *trk)
{
    assert(trk->ev_active);
    trk->ev_active = false;

    /* break the event loop - this will cause the loop to exit upon
       completion of any current event */
    scon_event_base_loopbreak(trk->ev_base);

    scon_thread_join(&trk->engine, NULL);
}

static int start_progress_engine(scon_progress_tracker_t *trk)
{
    assert(!trk->ev_active);
    trk->ev_active = true;

    /* fork off a thread to progress it */
    trk->engine.t_run = progress_engine;
    trk->engine.t_arg = trk;

    int rc = scon_thread_start(&trk->engine);
    if (SCON_SUCCESS != rc) {
        SCON_ERROR_LOG(rc);
    }

    return rc;
}

SCON_EXPORT scon_event_base_t *scon_progress_thread_init(const char *name)
{
    scon_progress_tracker_t *trk;
    int rc;

    if (!inited) {
        SCON_CONSTRUCT(&tracking, scon_list_t);
        inited = true;
    }

    if (NULL == name) {
        name = shared_thread_name;
    }

    /* check if we already have this thread */
    SCON_LIST_FOREACH(trk, &tracking, scon_progress_tracker_t) {
        if (0 == strcmp(name, trk->name)) {
            /* we do, so up the refcount on it */
            ++trk->refcount;
            /* return the existing base */
            return trk->ev_base;
        }
    }

    trk = SCON_NEW(scon_progress_tracker_t);
    if (NULL == trk) {
        SCON_ERROR_LOG(SCON_ERR_OUT_OF_RESOURCE);
        return NULL;
    }

    trk->name = strdup(name);
    if (NULL == trk->name) {
        SCON_ERROR_LOG(SCON_ERR_OUT_OF_RESOURCE);
        SCON_RELEASE(trk);
        return NULL;
    }

    if (NULL == (trk->ev_base = scon_event_base_create())) {
        SCON_ERROR_LOG(SCON_ERR_OUT_OF_RESOURCE);
        SCON_RELEASE(trk);
        return NULL;
    }

    /* add an event to the new event base (if there are no events,
       scon_event_loop() will return immediately) */
    scon_event_set(trk->ev_base, &trk->block, -1, SCON_EV_PERSIST,
                   dummy_timeout_cb, trk);
    scon_event_add(&trk->block, &long_timeout);

    /* construct the thread object */
    SCON_CONSTRUCT(&trk->engine, scon_thread_t);
    trk->engine_constructed = true;
    if (SCON_SUCCESS != (rc = start_progress_engine(trk))) {
        SCON_ERROR_LOG(rc);
        SCON_RELEASE(trk);
        return NULL;
    }
    scon_list_append(&tracking, &trk->super);

    return trk->ev_base;
}

int scon_progress_thread_finalize(const char *name)
{
    scon_progress_tracker_t *trk;

    if (!inited) {
        /* nothing we can do */
        return SCON_ERR_NOT_FOUND;
    }

    if (NULL == name) {
        name = shared_thread_name;
    }

    /* find the specified engine */
    SCON_LIST_FOREACH(trk, &tracking, scon_progress_tracker_t) {
        if (0 == strcmp(name, trk->name)) {
            /* decrement the refcount */
            --trk->refcount;

            /* If the refcount is still above 0, we're done here */
            if (trk->refcount > 0) {
                return SCON_SUCCESS;
            }

            /* If the progress thread is active, stop it */
            if (trk->ev_active) {
                stop_progress_engine(trk);
            }

            scon_list_remove_item(&tracking, &trk->super);
            SCON_RELEASE(trk);
            return SCON_SUCCESS;
        }
    }

    return SCON_ERR_NOT_FOUND;
}

/*
 * Stop the progress thread, but don't delete the tracker (or event base)
 */
int scon_progress_thread_pause(const char *name)
{
    scon_progress_tracker_t *trk;

    if (!inited) {
        /* nothing we can do */
        return SCON_ERR_NOT_FOUND;
    }

    if (NULL == name) {
        name = shared_thread_name;
    }

    /* find the specified engine */
    SCON_LIST_FOREACH(trk, &tracking, scon_progress_tracker_t) {
        if (0 == strcmp(name, trk->name)) {
            if (trk->ev_active) {
                stop_progress_engine(trk);
            }

            return SCON_SUCCESS;
        }
    }

    return SCON_ERR_NOT_FOUND;
}

int scon_progress_thread_resume(const char *name)
{
    scon_progress_tracker_t *trk;

    if (!inited) {
        /* nothing we can do */
        return SCON_ERR_NOT_FOUND;
    }

    if (NULL == name) {
        name = shared_thread_name;
    }

    /* find the specified engine */
    SCON_LIST_FOREACH(trk, &tracking, scon_progress_tracker_t) {
        if (0 == strcmp(name, trk->name)) {
            if (trk->ev_active) {
                return SCON_ERR_RESOURCE_BUSY;
            }

            return start_progress_engine(trk);
        }
    }

    return SCON_ERR_NOT_FOUND;
}
