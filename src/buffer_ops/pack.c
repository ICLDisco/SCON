/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2011-2013 Cisco Systems, Inc.  All rights reserved.
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
 */

#include <scon_config.h>
#include <scon_types.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "src/util/argv.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/buffer_ops/internal.h"
#include "src/include/scon_globals.h"

scon_status_t scon_bfrop_pack(scon_buffer_t *buffer,
                              const void *src, int32_t num_vals,
                              scon_data_type_t type)
 {
    scon_status_t rc;

    /* check for error */
    if (NULL == buffer) {
        return SCON_ERR_BAD_PARAM;
    }

    /* Pack the number of values */
    if (SCON_BFROP_BUFFER_FULLY_DESC == buffer->type) {
        if (SCON_SUCCESS != (rc = scon_bfrop_store_data_type(buffer, SCON_INT32))) {
            return rc;
        }
    }
    if (SCON_SUCCESS != (rc = scon_bfrop_pack_int32(buffer, &num_vals, 1, SCON_INT32))) {
        return rc;
    }

    /* Pack the value(s) */
    return scon_bfrop_pack_buffer(buffer, src, num_vals, type);
}

scon_status_t scon_bfrop_pack_buffer(scon_buffer_t *buffer,
                                     const void *src, int32_t num_vals,
                                     scon_data_type_t type)
{
    scon_status_t rc;
    scon_bfrop_type_info_t *info;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_pack_buffer( %p, %p, %lu, %d )\n",
                        (void*)buffer, src, (long unsigned int)num_vals, (int)type);

    /* Pack the declared data type */
    if (SCON_BFROP_BUFFER_FULLY_DESC == buffer->type) {
        if (SCON_SUCCESS != (rc = scon_bfrop_store_data_type(buffer, type))) {
            return rc;
        }
    }

    /* Lookup the pack function for this type and call it */

    if (NULL == (info = (scon_bfrop_type_info_t*)scon_pointer_array_get_item(&scon_bfrop_types, type))) {
        return SCON_ERR_PACK_FAILURE;
    }

    return info->odti_pack_fn(buffer, src, num_vals, type);
}


/* PACK FUNCTIONS FOR GENERIC SYSTEM TYPES */

/*
 * BOOL
 */
scon_status_t scon_bfrop_pack_bool(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type)
 {
    uint8_t *dst;
    int32_t i;
    bool *s = (bool*)src;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_pack_bool * %d\n", num_vals);
    /* check to see if buffer needs extending */
    if (NULL == (dst = (uint8_t*)scon_bfrop_buffer_extend(buffer, num_vals))) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    /* store the data */
    for (i=0; i < num_vals; i++) {
        if (s[i]) {
            dst[i] = 1;
        } else {
            dst[i] = 0;
        }
    }

    /* update buffer pointers */
    buffer->pack_ptr += num_vals;
    buffer->bytes_used += num_vals;

    return SCON_SUCCESS;
}

/*
 * INT
 */
scon_status_t scon_bfrop_pack_int(scon_buffer_t *buffer, const void *src,
                                  int32_t num_vals, scon_data_type_t type)
 {
    scon_status_t ret;

    /* System types need to always be described so we can properly
       unpack them */
    if (SCON_SUCCESS != (ret = scon_bfrop_store_data_type(buffer, BFROP_TYPE_INT))) {
        return ret;
    }

    /* Turn around and pack the real type */
    return scon_bfrop_pack_buffer(buffer, src, num_vals, BFROP_TYPE_INT);
}

/*
 * SIZE_T
 */
scon_status_t scon_bfrop_pack_sizet(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type)
 {
    scon_status_t ret;

    /* System types need to always be described so we can properly
       unpack them. */
    if (SCON_SUCCESS != (ret = scon_bfrop_store_data_type(buffer, BFROP_TYPE_SIZE_T))) {
        return ret;
    }

    return scon_bfrop_pack_buffer(buffer, src, num_vals, BFROP_TYPE_SIZE_T);
}

/*
 * PID_T
 */
scon_status_t scon_bfrop_pack_pid(scon_buffer_t *buffer, const void *src,
                                  int32_t num_vals, scon_data_type_t type)
 {
    scon_status_t ret;

    /* System types need to always be described so we can properly
       unpack them. */
    if (SCON_SUCCESS != (ret = scon_bfrop_store_data_type(buffer, BFROP_TYPE_PID_T))) {
        return ret;
    }

    /* Turn around and pack the real type */
    return scon_bfrop_pack_buffer(buffer, src, num_vals, BFROP_TYPE_PID_T);
}


/* PACK FUNCTIONS FOR NON-GENERIC SYSTEM TYPES */

/*
 * BYTE, CHAR, INT8
 */
scon_status_t scon_bfrop_pack_byte(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type)
 {
    char *dst;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_pack_byte * %d\n", num_vals);
    /* check to see if buffer needs extending */
    if (NULL == (dst = scon_bfrop_buffer_extend(buffer, num_vals))) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    /* store the data */
    memcpy(dst, src, num_vals);

    /* update buffer pointers */
    buffer->pack_ptr += num_vals;
    buffer->bytes_used += num_vals;

    return SCON_SUCCESS;
}

/*
 * INT16
 */
scon_status_t scon_bfrop_pack_int16(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type)
 {
    int32_t i;
    uint16_t tmp, *srctmp = (uint16_t*) src;
    char *dst;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_pack_int16 * %d\n", num_vals);
    /* check to see if buffer needs extending */
    if (NULL == (dst = scon_bfrop_buffer_extend(buffer, num_vals*sizeof(tmp)))) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0; i < num_vals; ++i) {
        tmp = scon_htons(srctmp[i]);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += num_vals * sizeof(tmp);
    buffer->bytes_used += num_vals * sizeof(tmp);

    return SCON_SUCCESS;
}

/*
 * INT32
 */
scon_status_t scon_bfrop_pack_int32(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type)
 {
    int32_t i;
    uint32_t tmp, *srctmp = (uint32_t*) src;
    char *dst;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_pack_int32 * %d\n", num_vals);
    /* check to see if buffer needs extending */
    if (NULL == (dst = scon_bfrop_buffer_extend(buffer, num_vals*sizeof(tmp)))) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0; i < num_vals; ++i) {
        tmp = htonl(srctmp[i]);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += num_vals * sizeof(tmp);
    buffer->bytes_used += num_vals * sizeof(tmp);

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_pack_datatype(scon_buffer_t *buffer, const void *src,
                                       int32_t num_vals, scon_data_type_t type)
{
    return scon_bfrop_pack_int16(buffer, src, num_vals, type);
}

/*
 * INT64
 */
scon_status_t scon_bfrop_pack_int64(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type)
 {
    int32_t i;
    uint64_t tmp, tmp2;
    char *dst;
    size_t bytes_packed = num_vals * sizeof(tmp);

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_pack_int64 * %d\n", num_vals);
    /* check to see if buffer needs extending */
    if (NULL == (dst = scon_bfrop_buffer_extend(buffer, bytes_packed))) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0; i < num_vals; ++i) {
        memcpy(&tmp2, (char *)src+i*sizeof(uint64_t), sizeof(uint64_t));
        tmp = scon_hton64(tmp2);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += bytes_packed;
    buffer->bytes_used += bytes_packed;

    return SCON_SUCCESS;
}

/*
 * STRING
 */
scon_status_t scon_bfrop_pack_string(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type)
 {
    scon_status_t ret = SCON_SUCCESS;
    int32_t i, len;
    char **ssrc = (char**) src;

    for (i = 0; i < num_vals; ++i) {
        if (NULL == ssrc[i]) {  /* got zero-length string/NULL pointer - store NULL */
        len = 0;
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_int32(buffer, &len, 1, SCON_INT32))) {
            return ret;
        }
    } else {
        len = (int32_t)strlen(ssrc[i]) + 1;
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_int32(buffer, &len, 1, SCON_INT32))) {
            return ret;
        }
        if (SCON_SUCCESS != (ret =
            scon_bfrop_pack_byte(buffer, ssrc[i], len, SCON_BYTE))) {
            return ret;
    }
}
}

return SCON_SUCCESS;
}

/* FLOAT */
scon_status_t scon_bfrop_pack_float(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type)
{
    scon_status_t ret = SCON_SUCCESS;
    int32_t i;
    float *ssrc = (float*)src;
    char *convert;

    for (i = 0; i < num_vals; ++i) {
        if (0 > asprintf(&convert, "%f", ssrc[i])) {
            return SCON_ERR_NOMEM;
        }
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_string(buffer, &convert, 1, SCON_STRING))) {
            free(convert);
            return ret;
        }
        free(convert);
    }

    return SCON_SUCCESS;
}

/* DOUBLE */
scon_status_t scon_bfrop_pack_double(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type)
{
    scon_status_t ret = SCON_SUCCESS;
    int32_t i;
    double *ssrc = (double*)src;
    char *convert;

    for (i = 0; i < num_vals; ++i) {
        if (0 > asprintf(&convert, "%f", ssrc[i])) {
            return SCON_ERR_NOMEM;
        }
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_string(buffer, &convert, 1, SCON_STRING))) {
            free(convert);
            return ret;
        }
        free(convert);
    }

    return SCON_SUCCESS;
}

/* TIMEVAL */
scon_status_t scon_bfrop_pack_timeval(scon_buffer_t *buffer, const void *src,
                                      int32_t num_vals, scon_data_type_t type)
{
    int64_t tmp[2];
    scon_status_t ret = SCON_SUCCESS;
    int32_t i;
    struct timeval *ssrc = (struct timeval *)src;

    for (i = 0; i < num_vals; ++i) {
        tmp[0] = (int64_t)ssrc[i].tv_sec;
        tmp[1] = (int64_t)ssrc[i].tv_usec;
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_int64(buffer, tmp, 2, SCON_INT64))) {
            return ret;
        }
    }

    return SCON_SUCCESS;
}

/* TIME */
scon_status_t scon_bfrop_pack_time(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type)
{
    scon_status_t ret = SCON_SUCCESS;
    int32_t i;
    time_t *ssrc = (time_t *)src;
    uint64_t ui64;

    /* time_t is a system-dependent size, so cast it
     * to uint64_t as a generic safe size
     */
     for (i = 0; i < num_vals; ++i) {
        ui64 = (uint64_t)ssrc[i];
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_int64(buffer, &ui64, 1, SCON_UINT64))) {
            return ret;
        }
    }

    return SCON_SUCCESS;
}

/* STATUS */
scon_status_t scon_bfrop_pack_status(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type)
{
    scon_status_t ret = SCON_SUCCESS;
    int32_t i;
    scon_status_t *ssrc = (scon_status_t *)src;
    int32_t status;

    for (i = 0; i < num_vals; ++i) {
        status = (int32_t)ssrc[i];
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_int32(buffer, &status, 1, SCON_INT32))) {
            return ret;
        }
    }

    return SCON_SUCCESS;
}


/* PACK FUNCTIONS FOR GENERIC SCON TYPES */
static scon_status_t pack_val(scon_buffer_t *buffer,
                              scon_value_t *p)
{
    scon_status_t ret;

    switch (p->type) {
        case SCON_UNDEF:
            break;
        case SCON_BOOL:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.flag, 1, SCON_BOOL))) {
                return ret;
            }
            break;
        case SCON_BYTE:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.byte, 1, SCON_BYTE))) {
                return ret;
            }
            break;
        case SCON_STRING:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.string, 1, SCON_STRING))) {
                return ret;
            }
            break;
        case SCON_SIZE:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.size, 1, SCON_SIZE))) {
                return ret;
            }
            break;
        case SCON_PID:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.pid, 1, SCON_PID))) {
                return ret;
            }
            break;
        case SCON_INT:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.integer, 1, SCON_INT))) {
                return ret;
            }
            break;
        case SCON_INT8:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.int8, 1, SCON_INT8))) {
                return ret;
            }
            break;
        case SCON_INT16:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.int16, 1, SCON_INT16))) {
                return ret;
            }
            break;
        case SCON_INT32:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.int32, 1, SCON_INT32))) {
                return ret;
            }
            break;
        case SCON_INT64:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.int64, 1, SCON_INT64))) {
                return ret;
            }
            break;
        case SCON_UINT:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.uint, 1, SCON_UINT))) {
                return ret;
            }
            break;
        case SCON_UINT8:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.uint8, 1, SCON_UINT8))) {
                return ret;
            }
            break;
        case SCON_UINT16:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.uint16, 1, SCON_UINT16))) {
                return ret;
            }
            break;
        case SCON_UINT32:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.uint32, 1, SCON_UINT32))) {
                return ret;
            }
            break;
        case SCON_UINT64:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.uint64, 1, SCON_UINT64))) {
                return ret;
            }
            break;
        case SCON_FLOAT:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.fval, 1, SCON_FLOAT))) {
                return ret;
            }
            break;
        case SCON_DOUBLE:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.dval, 1, SCON_DOUBLE))) {
                return ret;
            }
            break;
        case SCON_TIMEVAL:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.tv, 1, SCON_TIMEVAL))) {
                return ret;
            }
            break;
        case SCON_TIME:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.time, 1, SCON_TIME))) {
                return ret;
            }
            break;
        case SCON_STATUS:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.status, 1, SCON_STATUS))) {
                return ret;
            }
            break;
        case SCON_PROC:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, p->data.proc, 1, SCON_PROC))) {
                return ret;
            }
            break;
        case SCON_INFO:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_info(buffer, p->data.info, 1, SCON_INFO))) {
                return ret;
            }
            break;
        case SCON_BYTE_OBJECT:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.bo, 1, SCON_BYTE_OBJECT))) {
                return ret;
            }
            break;

        case SCON_DATA_RANGE:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, &p->data.range, 1, SCON_DATA_RANGE))) {
                return ret;
            }
            break;
        case SCON_DATA_ARRAY:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, p->data.darray, 1, SCON_DATA_ARRAY))) {
                return ret;
            }
            break;
 /*       case SCON_QUERY:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, p->data.darray, 1, SCON_QUERY))) {
                return ret;
            }
            break;*/
        /**** DEPRECATED ****/
   /*     case SCON_INFO_ARRAY:
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, p->data.array, 1, SCON_INFO_ARRAY))) {
                return ret;
            }
            break;*/
        /********************/
        default:
        scon_output(0, "PACK-SCON-VALUE: UNSUPPORTED TYPE %d", (int)p->type);
        return SCON_ERROR;
    }
    return SCON_SUCCESS;
}

/*
 * SCON_VALUE
 */
 scon_status_t scon_bfrop_pack_value(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type)
 {
    scon_value_t *ptr;
    int32_t i;
    scon_status_t ret;

    ptr = (scon_value_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the type */
        if (SCON_SUCCESS != (ret = scon_bfrop_store_data_type(buffer, ptr[i].type))) {
            return ret;
        }
        /* now pack the right field */
        if (SCON_SUCCESS != (ret = pack_val(buffer, &ptr[i]))) {
            return ret;
        }
    }

    return SCON_SUCCESS;
}


scon_status_t scon_bfrop_pack_info(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type)
{
    scon_info_t *info;
    int32_t i;
    scon_status_t ret;
    char *foo;

    info = (scon_info_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack key */
        foo = info[i].key;
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_string(buffer, &foo, 1, SCON_STRING))) {
            return ret;
        }
        /* pack info directives flag */
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_infodirs(buffer, &info[i].flags, 1, SCON_INFO_DIRECTIVES))) {
            return ret;
        }
        /* pack the type */
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_int(buffer, &info[i].value.type, 1, SCON_INT))) {
            return ret;
        }
        /* pack value */
        if (SCON_SUCCESS != (ret = pack_val(buffer, &info[i].value))) {
            return ret;
        }
    }
    return SCON_SUCCESS;
}


scon_status_t scon_bfrop_pack_buf(scon_buffer_t *buffer, const void *src,
                                  int32_t num_vals, scon_data_type_t type)
{
    scon_buffer_t **ptr;
    int32_t i;
    scon_status_t ret;

    ptr = (scon_buffer_t **) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the number of bytes */
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_sizet(buffer, &ptr[i]->bytes_used, 1, SCON_SIZE))) {
            return ret;
        }
        /* pack the bytes */
        if (0 < ptr[i]->bytes_used) {
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_byte(buffer, ptr[i]->base_ptr, ptr[i]->bytes_used, SCON_BYTE))) {
                return ret;
            }
        }
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_pack_proc(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type)
{
    scon_proc_t *proc;
    int32_t i;
    scon_status_t ret;

    proc = (scon_proc_t *) src;

    for (i = 0; i < num_vals; ++i) {
        char *ptr = proc[i].job_name;
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_string(buffer, &ptr, 1, SCON_STRING))) {
            return ret;
        }
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_rank(buffer, &proc[i].rank, 1, SCON_PROC_RANK))) {
            return ret;
        }
    }
    return SCON_SUCCESS;
}



#if 0
scon_status_t scon_bfrop_pack_kval(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type)
{
    scon_kval_t *ptr;
    int32_t i;
    scon_status_t ret;
    char *st;

    ptr = (scon_kval_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the key */
       st = ptr[i].key;
       if (SCON_SUCCESS != (ret = scon_bfrop_pack_string(buffer, &st, 1, SCON_STRING))) {
            return ret;
       }
        /* pack the value */
       if (SCON_SUCCESS != (ret = scon_bfrop_pack_value(buffer, ptr[i].value, 1, SCON_VALUE))) {
           return ret;
       }
    }
    return SCON_SUCCESS;
}
#endif

scon_status_t scon_bfrop_pack_range(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type)
{
    return scon_bfrop_pack_byte(buffer, src, num_vals, SCON_UINT8);
}

scon_status_t scon_bfrop_pack_infodirs(scon_buffer_t *buffer, const void *src,
                                       int32_t num_vals, scon_data_type_t type)
{
    return scon_bfrop_pack_int32(buffer, src, num_vals, SCON_UINT32);
}

scon_status_t scon_bfrop_pack_bo(scon_buffer_t *buffer, const void *src,
                                 int32_t num_vals, scon_data_type_t type)
{
    scon_status_t ret;
    int i;
    scon_byte_object_t *bo;

    bo = (scon_byte_object_t*)src;
    for (i=0; i < num_vals; i++) {
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_sizet(buffer, &bo[i].size, 1, SCON_SIZE))) {
            return ret;
        }
        if (0 < bo[i].size) {
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_byte(buffer, bo[i].bytes, bo[i].size, SCON_BYTE))) {
                return ret;
            }
        }
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_pack_ptr(scon_buffer_t *buffer, const void *src,
                        int32_t num_vals, scon_data_type_t type)
{
    uint8_t foo=1;
    /* it obviously makes no sense to pack a pointer and
     * send it somewhere else, so we just pack a sentinel */
    return scon_bfrop_pack_byte(buffer, &foo, 1, SCON_UINT8);
}

scon_status_t scon_bfrop_pack_darray(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type)
{
    scon_data_array_t *p = (scon_data_array_t*)src;
    scon_status_t ret;
    int32_t i;

    for (i=0; i < num_vals; i++) {
        /* pack the actual type in the array */
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_datatype(buffer, &p[i].type, 1, SCON_DATA_TYPE))) {
            return ret;
        }
        /* pack the number of array elements */
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_sizet(buffer, &p[i].size, 1, SCON_SIZE))) {
            return ret;
        }
        if (0 == p[i].size || SCON_UNDEF == p[i].type) {
            /* nothing left to do */
            continue;
        }
        /* pack the actual elements */
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_buffer(buffer, p[i].array, p[i].size, p[i].type))) {
            return ret;
        }
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_pack_rank(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type)
{
    return scon_bfrop_pack_int32(buffer, src, num_vals, SCON_UINT32);
}

scon_status_t scon_bfrop_pack_coll_sig(scon_buffer_t *buffer,
                                       const void *src, int32_t num_vals,
                                       scon_data_type_t type)
{
    scon_collectives_signature_t **ptr;
    int32_t i;
    int rc;

    ptr = (scon_collectives_signature_t **) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the scon handle  */
        if (SCON_SUCCESS != (rc = scon_bfrop_pack_datatype(buffer, &ptr[i]->scon_handle, 1, SCON_INT32))) {
            return rc;
        }

        /* pack the #procs */
        if (SCON_SUCCESS != (rc = scon_bfrop_pack_datatype(buffer, &ptr[i]->nprocs, 1, SCON_SIZE))) {
            return rc;
        }
        if (0 < ptr[i]->nprocs) {
            /* pack the array */
            if (SCON_SUCCESS != (rc = scon_bfrop_pack_proc(buffer, ptr[i]->procs, ptr[i]->nprocs, SCON_PROC))) {
                return rc;
            }
        }

        /* pack the sequence number */
        if (SCON_SUCCESS != (rc = scon_bfrop_pack_datatype(buffer, &ptr[i]->seq_num, 1, SCON_UINT32))) {
            return rc;
        }
    }

    return SCON_SUCCESS;
}

#if 0
scon_status_t scon_bfrop_pack_query(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type)
{
    scon_query_t *pq = (scon_query_t*)src;
    scon_status_t ret;
    int32_t i;
    int32_t nkeys;

    for (i=0; i < num_vals; i++) {
        /* pack the number of keys */
       nkeys = scon_argv_count(pq[i].keys);
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_int32(buffer, &nkeys, 1, SCON_INT32))) {
            return ret;
        }
        if (0 < nkeys) {
            /* pack the keys */
            if (SCON_SUCCESS != (ret = scon_bfrop_pack_string(buffer, pq[i].keys, nkeys, SCON_STRING))) {
                return ret;
            }
        }
        /* pack the number of qualifiers */
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_sizet(buffer, &pq[i].nqual, 1, SCON_SIZE))) {
            return ret;
        }
        if (0 < pq[i].nqual) {
           /* pack any provided qualifiers */
           if (SCON_SUCCESS != (ret = scon_bfrop_pack_info(buffer, pq[i].qualifiers, pq[i].nqual, SCON_INFO))) {
                return ret;
            }
        }
    }
    return SCON_SUCCESS;
}



/**** DEPRECATED ****/
scon_status_t scon_bfrop_pack_array(scon_buffer_t *buffer, const void *src,
                          int32_t num_vals, scon_data_type_t type)
{
    scon_info_array_t *ptr;
    int32_t i;
    scon_status_t ret;

    ptr = (scon_info_array_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the size */
        if (SCON_SUCCESS != (ret = scon_bfrop_pack_sizet(buffer, &ptr[i].size, 1, SCON_SIZE))) {
            return ret;
        }
        if (0 < ptr[i].size) {
            /* pack the values */
          if (SCON_SUCCESS != (ret = scon_bfrop_pack_info(buffer, ptr[i].array, ptr[i].size, SCON_INFO))) {
                return ret;
            }
        }
    }

    return SCON_SUCCESS;
}
/********************/
#endif
