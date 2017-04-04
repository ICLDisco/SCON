/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2009 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2012-2013 Los Alamos National Security, Inc.  All rights reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/** @file:
 *
 */
#include <src/include/scon_config.h>

#include <scon_common.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "src/util/argv.h"
#include "src/util/error.h"
#include "src/buffer_ops/internal.h"

/**
 * globals
 */
bool scon_bfrop_initialized = false;
size_t scon_bfrop_initial_size = 0;
size_t scon_bfrop_threshold_size = 0;
scon_pointer_array_t scon_bfrop_types = {{0}};
scon_data_type_t scon_bfrop_num_reg_types = SCON_UNDEF;
static scon_bfrop_buffer_type_t scon_default_buf_type = SCON_BFROP_BUFFER_NON_DESC;

scon_bfrop_t scon_bfrop = {
    scon_bfrop_pack,
    scon_bfrop_unpack,
    scon_bfrop_copy,
    scon_bfrop_print,
    scon_bfrop_copy_payload,
};

/**
 * Object constructors, destructors, and instantiations
 */
/** Value **/
#if 0
static void scon_buffer_construct (scon_buffer_t* buffer)
{
    /** set the default buffer type */
    buffer->type = scon_default_buf_type;

    /* Make everything NULL to begin with */
    buffer->base_ptr = buffer->pack_ptr = buffer->unpack_ptr = NULL;
    buffer->bytes_allocated = buffer->bytes_used = 0;
}

static void scon_buffer_destruct (scon_buffer_t* buffer)
{
    if (NULL != buffer->base_ptr) {
        free (buffer->base_ptr);
    }
}

SCON_CLASS_INSTANCE(scon_buffer_t,
                   scon_object_t,
                   scon_buffer_construct,
                   scon_buffer_destruct);
#endif

static void scon_bfrop_type_info_construct(scon_bfrop_type_info_t *obj)
{
    obj->odti_name = NULL;
    obj->odti_pack_fn = NULL;
    obj->odti_unpack_fn = NULL;
    obj->odti_copy_fn = NULL;
    obj->odti_print_fn = NULL;
}

static void scon_bfrop_type_info_destruct(scon_bfrop_type_info_t *obj)
{
    if (NULL != obj->odti_name) {
        free(obj->odti_name);
    }
}

SCON_CLASS_INSTANCE(scon_bfrop_type_info_t, scon_object_t,
                   scon_bfrop_type_info_construct,
                   scon_bfrop_type_info_destruct);

/*static void kvcon(scon_kval_t *k)
{
    k->key = NULL;
    k->value = NULL;
}
static void kvdes(scon_kval_t *k)
{
    if (NULL != k->key) {
        free(k->key);
    }
    if (NULL != k->value) {
        SCON_VALUE_RELEASE(k->value);
    }
}
SCON_CLASS_INSTANCE(scon_kval_t,
                   scon_list_item_t,
                   kvcon, kvdes);*/
/*
static void rcon(scon_regex_range_t *p)
{
    p->start = 0;
    p->cnt = 0;
}
SCON_CLASS_INSTANCE(scon_regex_range_t,
                    scon_list_item_t,
                    rcon, NULL);

static void rvcon(scon_regex_value_t *p)
{
    p->prefix = NULL;
    p->suffix = NULL;
    p->num_digits = 0;
    SCON_CONSTRUCT(&p->ranges, scon_list_t);
}
static void rvdes(scon_regex_value_t *p)
{
    if (NULL != p->prefix) {
        free(p->prefix);
    }
    if (NULL != p->suffix) {
        free(p->suffix);
    }
    SCON_LIST_DESTRUCT(&p->ranges);
}
SCON_CLASS_INSTANCE(scon_regex_value_t,
                    scon_list_item_t,
                    rvcon, rvdes); */

scon_status_t scon_bfrop_open(void)
{
    scon_status_t rc;

    if (scon_bfrop_initialized) {
        return SCON_SUCCESS;
    }

    /** set the default buffer type. If we are in debug mode, then we default
     * to fully described buffers. Otherwise, we default to non-described for brevity
     * and performance
     */
#if SCON_ENABLE_DEBUG
    scon_default_buf_type = SCON_BFROP_BUFFER_FULLY_DESC;
#else
    scon_default_buf_type = SCON_BFROP_BUFFER_NON_DESC;
#endif

    /* Setup the types array */
    SCON_CONSTRUCT(&scon_bfrop_types, scon_pointer_array_t);
    if (SCON_SUCCESS != (rc = scon_pointer_array_init(&scon_bfrop_types, 64, 255, 64))) {
        return rc;
    }
    scon_bfrop_num_reg_types = SCON_UNDEF;
    scon_bfrop_threshold_size = SCON_BFROP_DEFAULT_THRESHOLD_SIZE;
    scon_bfrop_initial_size = SCON_BFROP_DEFAULT_INITIAL_SIZE ;

    /* Register all the supported types */
    SCON_REGISTER_TYPE("SCON_BOOL", SCON_BOOL,
                       scon_bfrop_pack_bool,
                       scon_bfrop_unpack_bool,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_bool);

    SCON_REGISTER_TYPE("SCON_BYTE", SCON_BYTE,
                       scon_bfrop_pack_byte,
                       scon_bfrop_unpack_byte,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_byte);

    SCON_REGISTER_TYPE("SCON_STRING", SCON_STRING,
                       scon_bfrop_pack_string,
                       scon_bfrop_unpack_string,
                       scon_bfrop_copy_string,
                       scon_bfrop_print_string);

    SCON_REGISTER_TYPE("SCON_SIZE", SCON_SIZE,
                       scon_bfrop_pack_sizet,
                       scon_bfrop_unpack_sizet,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_size);

    SCON_REGISTER_TYPE("SCON_PID", SCON_PID,
                       scon_bfrop_pack_pid,
                       scon_bfrop_unpack_pid,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_pid);

    SCON_REGISTER_TYPE("SCON_INT", SCON_INT,
                       scon_bfrop_pack_int,
                       scon_bfrop_unpack_int,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_int);

    SCON_REGISTER_TYPE("SCON_INT8", SCON_INT8,
                       scon_bfrop_pack_byte,
                       scon_bfrop_unpack_byte,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_int8);

    SCON_REGISTER_TYPE("SCON_INT16", SCON_INT16,
                       scon_bfrop_pack_int16,
                       scon_bfrop_unpack_int16,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_int16);

    SCON_REGISTER_TYPE("SCON_INT32", SCON_INT32,
                       scon_bfrop_pack_int32,
                       scon_bfrop_unpack_int32,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_int32);

    SCON_REGISTER_TYPE("SCON_INT64", SCON_INT64,
                       scon_bfrop_pack_int64,
                       scon_bfrop_unpack_int64,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_int64);

    SCON_REGISTER_TYPE("SCON_UINT", SCON_UINT,
                       scon_bfrop_pack_int,
                       scon_bfrop_unpack_int,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_uint);

    SCON_REGISTER_TYPE("SCON_UINT8", SCON_UINT8,
                       scon_bfrop_pack_byte,
                       scon_bfrop_unpack_byte,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_uint8);

    SCON_REGISTER_TYPE("SCON_UINT16", SCON_UINT16,
                       scon_bfrop_pack_int16,
                       scon_bfrop_unpack_int16,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_uint16);

    SCON_REGISTER_TYPE("SCON_UINT32", SCON_UINT32,
                       scon_bfrop_pack_int32,
                       scon_bfrop_unpack_int32,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_uint32);

    SCON_REGISTER_TYPE("SCON_UINT64", SCON_UINT64,
                       scon_bfrop_pack_int64,
                       scon_bfrop_unpack_int64,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_uint64);

    SCON_REGISTER_TYPE("SCON_FLOAT", SCON_FLOAT,
                       scon_bfrop_pack_float,
                       scon_bfrop_unpack_float,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_float);

    SCON_REGISTER_TYPE("SCON_DOUBLE", SCON_DOUBLE,
                       scon_bfrop_pack_double,
                       scon_bfrop_unpack_double,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_double);

    SCON_REGISTER_TYPE("SCON_TIMEVAL", SCON_TIMEVAL,
                       scon_bfrop_pack_timeval,
                       scon_bfrop_unpack_timeval,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_timeval);

    SCON_REGISTER_TYPE("SCON_TIME", SCON_TIME,
                       scon_bfrop_pack_time,
                       scon_bfrop_unpack_time,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_time);

    SCON_REGISTER_TYPE("SCON_STATUS", SCON_STATUS,
                       scon_bfrop_pack_status,
                       scon_bfrop_unpack_status,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_status);

    SCON_REGISTER_TYPE("SCON_VALUE", SCON_VALUE,
                       scon_bfrop_pack_value,
                       scon_bfrop_unpack_value,
                       scon_bfrop_copy_value,
                       scon_bfrop_print_value);

    SCON_REGISTER_TYPE("SCON_PROC", SCON_PROC,
                       scon_bfrop_pack_proc,
                       scon_bfrop_unpack_proc,
                       scon_bfrop_copy_proc,
                       scon_bfrop_print_proc);

    SCON_REGISTER_TYPE("SCON_INFO", SCON_INFO,
                       scon_bfrop_pack_info,
                       scon_bfrop_unpack_info,
                       scon_bfrop_copy_info,
                       scon_bfrop_print_info);

    SCON_REGISTER_TYPE("SCON_BUFFER", SCON_BUFFER,
                       scon_bfrop_pack_buf,
                       scon_bfrop_unpack_buf,
                       scon_bfrop_copy_buf,
                       scon_bfrop_print_buf);

    SCON_REGISTER_TYPE("SCON_BYTE_OBJECT", SCON_BYTE_OBJECT,
                       scon_bfrop_pack_bo,
                       scon_bfrop_unpack_bo,
                       scon_bfrop_copy_bo,
                       scon_bfrop_print_bo);

  /*  SCON_REGISTER_TYPE("SCON_KVAL", SCON_KVAL,
                       scon_bfrop_pack_kval,
                       scon_bfrop_unpack_kval,
                       scon_bfrop_copy_kval,
                       scon_bfrop_print_kval);*/

    SCON_REGISTER_TYPE("SCON_POINTER", SCON_POINTER,
                       scon_bfrop_pack_ptr,
                       scon_bfrop_unpack_ptr,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_ptr);

    SCON_REGISTER_TYPE("SCON_DATA_RANGE", SCON_DATA_RANGE,
                       scon_bfrop_pack_range,
                       scon_bfrop_unpack_range,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_range);

    SCON_REGISTER_TYPE("SCON_INFO_DIRECTIVES", SCON_INFO_DIRECTIVES,
                       scon_bfrop_pack_infodirs,
                       scon_bfrop_unpack_infodirs,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_infodirs);

    /*SCON_REGISTER_TYPE("SCON_PROC_STATE", SCON_PROC_STATE,
                       scon_bfrop_pack_pstate,
                       scon_bfrop_unpack_pstate,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_pstate);*/

   /* SCON_REGISTER_TYPE("SCON_PROC_INFO", SCON_PROC_INFO,
                       scon_bfrop_pack_pinfo,
                       scon_bfrop_unpack_pinfo,
                       scon_bfrop_copy_pinfo,
                       scon_bfrop_print_pinfo);*/

    SCON_REGISTER_TYPE("SCON_DATA_ARRAY", SCON_DATA_ARRAY,
                       scon_bfrop_pack_darray,
                       scon_bfrop_unpack_darray,
                       scon_bfrop_copy_darray,
                       scon_bfrop_print_darray);

    SCON_REGISTER_TYPE("SCON_PROC_RANK", SCON_PROC_RANK,
                       scon_bfrop_pack_rank,
                       scon_bfrop_unpack_rank,
                       scon_bfrop_std_copy,
                       scon_bfrop_print_rank);

    SCON_REGISTER_TYPE("SCON_COLLECTIVES_SIGNATURE", SCON_COLLECTIVES_SIGNATURE,
                       scon_bfrop_pack_coll_sig,
                       scon_bfrop_unpack_coll_sig,
                       scon_bfrop_copy_coll_sig,
                       scon_bfrop_print_coll_sig);

  /*  SCON_REGISTER_TYPE("SCON_QUERY", SCON_QUERY,
                       scon_bfrop_pack_query,
                       scon_bfrop_unpack_query,
                       scon_bfrop_copy_query,
                       scon_bfrop_print_query);
*/
    /**** DEPRECATED ***
    SCON_REGISTER_TYPE("SCON_INFO_ARRAY", SCON_INFO_ARRAY,
                       scon_bfrop_pack_array,
                       scon_bfrop_unpack_array,
                       scon_bfrop_copy_array,
                       scon_bfrop_print_array);
    *******************/

    /* All done */
    scon_bfrop_initialized = true;
    return SCON_SUCCESS;
}


scon_status_t scon_bfrop_close(void)
{
    int32_t i;

    if (!scon_bfrop_initialized) {
        return SCON_SUCCESS;
    }
    scon_bfrop_initialized = false;

    for (i = 0 ; i < scon_pointer_array_get_size(&scon_bfrop_types) ; ++i) {
        scon_bfrop_type_info_t *info = (scon_bfrop_type_info_t*)scon_pointer_array_get_item(&scon_bfrop_types, i);
        if (NULL != info) {
            scon_pointer_array_set_item(&scon_bfrop_types, i, NULL);
            SCON_RELEASE(info);
        }
    }

    SCON_DESTRUCT(&scon_bfrop_types);

    return SCON_SUCCESS;
}

/**** UTILITY SUPPORT ****/
SCON_EXPORT void scon_value_load(scon_value_t *v, void *data,
                                 scon_data_type_t type)
{
    scon_byte_object_t *bo;

    v->type = type;
    if (NULL == data) {
        /* just set the fields to zero */
        memset(&v->data, 0, sizeof(v->data));
    } else {
        switch(type) {
        case SCON_UNDEF:
            break;
        case SCON_BOOL:
            memcpy(&(v->data.flag), data, 1);
            break;
        case SCON_BYTE:
            memcpy(&(v->data.byte), data, 1);
            break;
        case SCON_STRING:
            v->data.string = strdup(data);
            break;
        case SCON_SIZE:
            memcpy(&(v->data.size), data, sizeof(size_t));
            break;
        case SCON_PID:
            memcpy(&(v->data.pid), data, sizeof(pid_t));
            break;
        case SCON_INT:
            memcpy(&(v->data.integer), data, sizeof(int));
            break;
        case SCON_INT8:
            memcpy(&(v->data.int8), data, 1);
            break;
        case SCON_INT16:
            memcpy(&(v->data.int16), data, 2);
            break;
        case SCON_INT32:
            memcpy(&(v->data.int32), data, 4);
            break;
        case SCON_INT64:
            memcpy(&(v->data.int64), data, 8);
            break;
        case SCON_UINT:
            memcpy(&(v->data.uint), data, sizeof(int));
            break;
        case SCON_UINT8:
            memcpy(&(v->data.uint8), data, 1);
            break;
        case SCON_UINT16:
            memcpy(&(v->data.uint16), data, 2);
            break;
        case SCON_UINT32:
            memcpy(&(v->data.uint32), data, 4);
            break;
        case SCON_UINT64:
            memcpy(&(v->data.uint64), data, 8);
            break;
        case SCON_FLOAT:
            memcpy(&(v->data.fval), data, sizeof(float));
            break;
        case SCON_DOUBLE:
            memcpy(&(v->data.dval), data, sizeof(double));
            break;
        case SCON_TIMEVAL:
            memcpy(&(v->data.tv), data, sizeof(struct timeval));
            break;
        case SCON_STATUS:
            memcpy(&(v->data.status), data, sizeof(scon_status_t));
            break;
        case SCON_PROC:
            SCON_PROC_CREATE(v->data.proc, 1);
            if (NULL == v->data.proc) {
                SCON_ERROR_LOG(SCON_ERR_NOMEM);
                return;
            }
            memcpy(v->data.proc, data, sizeof(scon_proc_t));
            break;
        case SCON_BYTE_OBJECT:
            bo = (scon_byte_object_t*)data;
            v->data.bo.bytes = bo->bytes;
            memcpy(&(v->data.bo.size), &bo->size, sizeof(size_t));
            break;
        case SCON_POINTER:
            memcpy(&(v->data.ptr), data, sizeof(void*));
            break;
        default:
            /* silence warnings */
            break;
        }
    }
}

SCON_EXPORT scon_status_t scon_value_unload(scon_value_t *kv, void **data,
                                size_t *sz, scon_data_type_t type)
{
    scon_status_t rc;
    scon_proc_t *pc;
    rc = SCON_SUCCESS;
    if (type != kv->type) {
        rc = SCON_ERR_TYPE_MISMATCH;
    } else if (NULL == data ||
               (NULL == *data && SCON_STRING != type && SCON_BYTE_OBJECT != type)) {
        rc = SCON_ERR_BAD_PARAM;
    } else {
        switch(type) {
        case SCON_UNDEF:
            rc = SCON_ERR_UNKNOWN_DATA_TYPE;
            break;
        case SCON_BOOL:
            memcpy(*data, &(kv->data.flag), 1);
            *sz = 1;
            break;
        case SCON_BYTE:
            memcpy(*data, &(kv->data.byte), 1);
            *sz = 1;
            break;
        case SCON_STRING:
            if (NULL != kv->data.string) {
                *data = strdup(kv->data.string);
                *sz = strlen(kv->data.string);
            }
            break;
        case SCON_SIZE:
            memcpy(*data, &(kv->data.size), sizeof(size_t));
            *sz = sizeof(size_t);
            break;
        case SCON_PID:
            memcpy(*data, &(kv->data.pid), sizeof(pid_t));
            *sz = sizeof(pid_t);
            break;
        case SCON_INT:
            memcpy(*data, &(kv->data.integer), sizeof(int));
            *sz = sizeof(int);
            break;
        case SCON_INT8:
            memcpy(*data, &(kv->data.int8), 1);
            *sz = 1;
            break;
        case SCON_INT16:
            memcpy(*data, &(kv->data.int16), 2);
            *sz = 2;
            break;
        case SCON_INT32:
            memcpy(*data, &(kv->data.int32), 4);
            *sz = 4;
            break;
        case SCON_INT64:
            memcpy(*data, &(kv->data.int64), 8);
            *sz = 8;
            break;
        case SCON_UINT:
            memcpy(*data, &(kv->data.uint), sizeof(int));
            *sz = sizeof(int);
            break;
        case SCON_UINT8:
            memcpy(*data, &(kv->data.uint8), 1);
            *sz = 1;
            break;
        case SCON_UINT16:
            memcpy(*data, &(kv->data.uint16), 2);
            *sz = 2;
            break;
        case SCON_UINT32:
            memcpy(*data, &(kv->data.uint32), 4);
            *sz = 4;
            break;
        case SCON_UINT64:
            memcpy(*data, &(kv->data.uint64), 8);
            *sz = 8;
            break;
        case SCON_FLOAT:
            memcpy(*data, &(kv->data.fval), sizeof(float));
            *sz = sizeof(float);
            break;
        case SCON_DOUBLE:
            memcpy(*data, &(kv->data.dval), sizeof(double));
            *sz = sizeof(double);
            break;
        case SCON_TIMEVAL:
            memcpy(*data, &(kv->data.tv), sizeof(struct timeval));
            *sz = sizeof(struct timeval);
            break;
        case SCON_STATUS:
            memcpy(*data, &(kv->data.status), sizeof(scon_status_t));
            *sz = sizeof(scon_status_t);
            break;
        case SCON_PROC:;
            SCON_PROC_CREATE(pc, 1);
            if (NULL == pc) {
                SCON_ERROR_LOG(SCON_ERR_NOMEM);
                rc = SCON_ERR_NOMEM;
                break;
            }
            memcpy(pc, kv->data.proc, sizeof(scon_proc_t));
            *sz = sizeof(scon_proc_t);
            *data = pc;
            break;
        case SCON_BYTE_OBJECT:
            if (NULL != kv->data.bo.bytes && 0 < kv->data.bo.size) {
                *data = kv->data.bo.bytes;
                *sz = kv->data.bo.size;
            } else {
                *data = NULL;
                *sz = 0;
            }
            break;
        case SCON_POINTER:
            memcpy(*data, &(kv->data.ptr), sizeof(void*));
            *sz = sizeof(void*);
            break;
        default:
            /* silence warnings */
            rc = SCON_ERROR;
            break;
        }
    }
    return rc;
}

int scon_buffer_unload(scon_buffer_t *buffer, void **payload,
                    int32_t *bytes_used)
{
    /* check that buffer is not null */
    if (!buffer) {
        return SCON_ERR_BAD_PARAM;
    }

    /* were we given someplace to point to the payload */
    if (NULL == payload) {
        return SCON_ERR_BAD_PARAM;
    }

    /* anything in the buffer - if not, nothing to do */
    if (NULL == buffer->base_ptr || 0 == buffer->bytes_used) {
        *payload = NULL;
        *bytes_used = 0;
        return SCON_SUCCESS;
    }

    /* if nothing has been unpacked, we can pass the entire
     * region back and protect it - no need to copy. This is
     * an optimization */
    if (buffer->unpack_ptr == buffer->base_ptr) {
        *payload = buffer->base_ptr;
        *bytes_used = buffer->bytes_used;
        buffer->base_ptr = NULL;
        buffer->unpack_ptr = NULL;
        buffer->pack_ptr = NULL;
        buffer->bytes_used = 0;
        return SCON_SUCCESS;
    }

    /* okay, we have something to provide - pass it back */
    *bytes_used = buffer->bytes_used - (buffer->unpack_ptr - buffer->base_ptr);
    if (0 == (*bytes_used)) {
        *payload = NULL;
    } else {
        /* we cannot just set the pointer as it might be
         * partway in a malloc'd region */
        *payload = (void*)malloc(*bytes_used);
        memcpy(*payload, buffer->unpack_ptr, *bytes_used);
    }

    /* All done */

    return SCON_SUCCESS;
}


int scon_buffer_load(scon_buffer_t *buffer, void *payload,
                  int32_t bytes_used)
{
    /* check to see if the buffer has been initialized */
    if (NULL == buffer) {
        return SCON_ERR_BAD_PARAM;
    }

    /* check if buffer already has payload - free it if so */
    if (NULL != buffer->base_ptr) {
        free(buffer->base_ptr);
    }

    /* if it's a NULL payload, just set things and return */
    if (NULL == payload) {
        buffer->base_ptr = NULL;
        buffer->pack_ptr = buffer->base_ptr;
        buffer->unpack_ptr = buffer->base_ptr;
        buffer->bytes_used = 0;
        buffer->bytes_allocated = 0;
        return SCON_SUCCESS;
    }

    /* populate the buffer */
    buffer->base_ptr = (char*)payload;

    /* set pack/unpack pointers */
    buffer->pack_ptr = ((char*)buffer->base_ptr) + bytes_used;
    buffer->unpack_ptr = buffer->base_ptr;

    /* set counts for size and space */
    buffer->bytes_allocated = buffer->bytes_used = bytes_used;

    /* All done */

    return SCON_SUCCESS;
}
