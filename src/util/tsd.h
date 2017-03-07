/*
 * Copyright (c) 2007-2013 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2008      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */


#ifndef SCON_THREADS_TSD_H
#define SCON_THREADS_TSD_H

#include "scon_config.h"

#include <pthread.h>

#include "src/util/error.h"

BEGIN_C_DECLS

/**
 * @file
 *
 * Thread Specific Datastore Interface
 *
 * Functions for providing thread-specific datastore capabilities.
 */


/**
 * Prototype for callback when tsd data is being destroyed
 */
typedef void (*scon_tsd_destructor_t)(void *value);

#if defined(DOXYGEN)

/**
 * Typedef for thread-specific data key
 */
typedef void* scon_tsd_key_t;


/**
 * Create thread-specific data key
 *
 * Create a thread-specific data key visible to all threads in the
 * current process.  The returned key is valid in all threads,
 * although the values bound to the key by scon_tsd_setspecific() are
 * allocated on a per-thread basis and persist for the life of the
 * calling thread.
 *
 * Upon key creation, the value NULL is associated with the new key in
 * all active threads.  When a new thread is created, the value NULL
 * is associated with all defined keys in the new thread.
 *
 * The destructor parameter may be NULL.  At thread exit, if
 * destructor is non-NULL AND the thread has a non-NULL value
 * associated with the key, the function is called with the current
 * value as its argument.
 *
 * @param key[out]       The key for accessing thread-specific data
 * @param destructor[in] Cleanup function to call when a thread exits
 *
 * @retval SCON_SUCCESS  Success
 * @retval EAGAIN        The system lacked the necessary resource to
 *                       create another thread specific data key
 * @retval ENOMEM        Insufficient memory exists to create the key
 */
SCON_DECLSPEC int scon_tsd_key_create(scon_tsd_key_t *key,
                                      scon_tsd_destructor_t destructor);


/**
 * Delete a thread-specific data key
 *
 * Delete a thread-specific data key previously returned by
 * scon_tsd_key_create().  The destructor associated with the key is
 * not fired in any thread and memory cleanup is the responsibility of
 * the caller.
 *
 * @note Unlike pthread_key_delete, this function should not be called
 * from within a destructor.  It can not be universally supported at
 * this time.
 *
 * @param key[in]       The key for accessing thread-specific data
 *
 * @retval SCON_SUCCESS Success
 * @retval EINVAL       Invalid key
 */
SCON_DECLSPEC int scon_tsd_key_delete(scon_tsd_key_t key);


/**
 * Set a thread-specific data value
 *
 * Associates value with key in the current thread.  The value for the
 * key in other threads is not changed.  Different threads may assign
 * different values to the same key.
 *
 * @note This function should not be called within
 * scon_tsd_key_delete().
 *
 * @param key[in]       Thread specific data key to modify
 * @param value[in]     Value to associate with key
 *
 * @retval SCON_SUCCESS Success
 * @retval ENOMEM       Insufficient memory exists to associate the
 *                      value with the key
 * @retval EINVAL       Invalid key
 */
SCON_DECLSPEC int scon_tsd_setspecific(scon_tsd_key_t key, void *value);


/**
 * Get a thread-specific data value
 *
 * Get the data associated with the given key, as set by
 * scon_tsd_setspecific().  If scon_tsd_setspecific() hasn't been
 * called in the current thread with the given key, NULL is returned
 * in valuep.
 *
 * @param key[in]       Thread specific data key to modify
 * @param value[out]     Value to associate with key
 *
 * @retval SCON_SUCCESS Success
 * @retval ENOMEM       Insufficient memory exists to associate the
 *                      value with the key
 * @retval EINVAL       Invalid key
 */
SCON_DECLSPEC int scon_tsd_getspecific(scon_tsd_key_t key, void **valuep);

#else

typedef pthread_key_t scon_tsd_key_t;

static inline int
scon_tsd_key_create(scon_tsd_key_t *key,
                    scon_tsd_destructor_t destructor)
{
    return pthread_key_create(key, destructor);
}

static inline int
scon_tsd_key_delete(scon_tsd_key_t key)
{
    return pthread_key_delete(key);
}

static inline int
scon_tsd_setspecific(scon_tsd_key_t key, void *value)
{
    return pthread_setspecific(key, value);
}

static inline int
scon_tsd_getspecific(scon_tsd_key_t key, void **valuep)
{
    *valuep = pthread_getspecific(key);
    return SCON_SUCCESS;
}

#endif

END_C_DECLS

#endif /* SCON_MTHREADS_TSD_H */
