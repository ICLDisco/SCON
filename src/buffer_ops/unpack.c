/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2012      Los Alamos National Security, Inc.  All rights reserved.
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

#include "src/util/argv.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/buffer_ops/types.h"
#include "src/buffer_ops/internal.h"
#include "src/include/scon_globals.h"
#include "src/util/name_fns.h"
scon_status_t scon_bfrop_unpack(scon_buffer_t *buffer,
                                void *dst, int32_t *num_vals,
                                scon_data_type_t type)
 {
    scon_status_t rc, ret;
    int32_t local_num, n=1;
    scon_data_type_t local_type;

    /* check for error */
    if (NULL == buffer || NULL == dst || NULL == num_vals) {
        return SCON_ERR_BAD_PARAM;
    }

    /* if user provides a zero for num_vals, then there is no storage allocated
     * so return an appropriate error
     */
     if (0 == *num_vals) {
        scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack: inadequate space ( %p, %p, %lu, %d )\n",
                            (void*)buffer, dst, (long unsigned int)*num_vals, (int)type);
        return SCON_ERR_UNPACK_INADEQUATE_SPACE;
    }

    /** Unpack the declared number of values
     * REMINDER: it is possible that the buffer is corrupted and that
     * the BFROP will *think* there is a proper int32_t variable at the
     * beginning of the unpack region - but that the value is bogus (e.g., just
     * a byte field in a string array that so happens to have a value that
     * matches the int32_t data type flag). Therefore, this error check is
     * NOT completely safe. This is true for ALL unpack functions, not just
     * int32_t as used here.
     */
     if (SCON_BFROP_BUFFER_FULLY_DESC == buffer->type) {
        if (SCON_SUCCESS != (rc = scon_bfrop_get_data_type(buffer, &local_type))) {
            *num_vals = 0;
            /* don't error log here as the user may be unpacking past
             * the end of the buffer, which isn't necessarily an error */
            return rc;
        }
        if (SCON_INT32 != local_type) { /* if the length wasn't first, then error */
            *num_vals = 0;
            return SCON_ERR_UNPACK_FAILURE;
        }
    }

    n=1;
    if (SCON_SUCCESS != (rc = scon_bfrop_unpack_int32(buffer, &local_num, &n, SCON_INT32))) {
        *num_vals = 0;
            /* don't error log here as the user may be unpacking past
             * the end of the buffer, which isn't necessarily an error */
        return rc;
    }

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack: found %d values for %d provided storage",
                        local_num, *num_vals);

    /** if the storage provided is inadequate, set things up
     * to unpack as much as we can and to return an error code
     * indicating that everything was not unpacked - the buffer
     * is left in a state where it can not be further unpacked.
     */
     if (local_num > *num_vals) {
        local_num = *num_vals;
        scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack: inadequate space ( %p, %p, %lu, %d )\n",
                            (void*)buffer, dst, (long unsigned int)*num_vals, (int)type);
        ret = SCON_ERR_UNPACK_INADEQUATE_SPACE;
    } else {  /** enough or more than enough storage */
        *num_vals = local_num;  /** let the user know how many we actually unpacked */
        ret = SCON_SUCCESS;
    }

    /** Unpack the value(s) */
    if (SCON_SUCCESS != (rc = scon_bfrop_unpack_buffer(buffer, dst, &local_num, type))) {
        *num_vals = 0;
        ret = rc;
    }

    return ret;
}

scon_status_t scon_bfrop_unpack_buffer(scon_buffer_t *buffer, void *dst, int32_t *num_vals,
                                       scon_data_type_t type)
{
    scon_status_t rc;
    scon_data_type_t local_type;
    scon_bfrop_type_info_t *info;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_buffer( %p, %p, %lu, %d )\n",
                        (void*)buffer, dst, (long unsigned int)*num_vals, (int)type);

    /** Unpack the declared data type */
    if (SCON_BFROP_BUFFER_FULLY_DESC == buffer->type) {
        if (SCON_SUCCESS != (rc = scon_bfrop_get_data_type(buffer, &local_type))) {
            return rc;
        }
        /* if the data types don't match, then return an error */
        if (type != local_type) {
            scon_output(0, "SCON bfrop:unpack: got type %d when expecting type %d", local_type, type);
            return SCON_ERR_PACK_MISMATCH;
        }
    }

    /* Lookup the unpack function for this type and call it */

    if (NULL == (info = (scon_bfrop_type_info_t*)scon_pointer_array_get_item(&scon_bfrop_types, type))) {
        return SCON_ERR_UNPACK_FAILURE;
    }

    return info->odti_unpack_fn(buffer, dst, num_vals, type);
}


/* UNPACK GENERIC SYSTEM TYPES */

/*
 * BOOL
 */
scon_status_t scon_bfrop_unpack_bool(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type)
 {
    int32_t i;
    uint8_t *src;
    bool *dst;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_bool * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, *num_vals)) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    src = (uint8_t*)buffer->unpack_ptr;
    dst = (bool*)dest;

    for (i=0; i < *num_vals; i++) {
        if (src[i]) {
            dst[i] = true;
        } else {
            dst[i] = false;
        }
    }

    /* update buffer pointer */
    buffer->unpack_ptr += *num_vals;

    return SCON_SUCCESS;
}

/*
 * INT
 */
scon_status_t scon_bfrop_unpack_int(scon_buffer_t *buffer, void *dest,
                                    int32_t *num_vals, scon_data_type_t type)
 {
    scon_status_t ret;
    scon_data_type_t remote_type;

    if (SCON_SUCCESS != (ret = scon_bfrop_get_data_type(buffer, &remote_type))) {
        return ret;
    }

    if (remote_type == BFROP_TYPE_INT) {
        /* fast path it if the sizes are the same */
        /* Turn around and unpack the real type */
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, dest, num_vals, BFROP_TYPE_INT))) {
        }
    } else {
        /* slow path - types are different sizes */
        UNPACK_SIZE_MISMATCH(int, remote_type, ret);
    }

    return ret;
}

/*
 * SIZE_T
 */
scon_status_t scon_bfrop_unpack_sizet(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
 {
    scon_status_t ret;
    scon_data_type_t remote_type;

    if (SCON_SUCCESS != (ret = scon_bfrop_get_data_type(buffer, &remote_type))) {
        return ret;
    }

    if (remote_type == BFROP_TYPE_SIZE_T) {
        /* fast path it if the sizes are the same */
        /* Turn around and unpack the real type */
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, dest, num_vals, BFROP_TYPE_SIZE_T))) {
        }
    } else {
        /* slow path - types are different sizes */
        UNPACK_SIZE_MISMATCH(size_t, remote_type, ret);
    }

    return ret;
}

/*
 * PID_T
 */
scon_status_t scon_bfrop_unpack_pid(scon_buffer_t *buffer, void *dest,
                                    int32_t *num_vals, scon_data_type_t type)
 {
    scon_status_t ret;
    scon_data_type_t remote_type;

    if (SCON_SUCCESS != (ret = scon_bfrop_get_data_type(buffer, &remote_type))) {
        return ret;
    }

    if (remote_type == BFROP_TYPE_PID_T) {
        /* fast path it if the sizes are the same */
        /* Turn around and unpack the real type */
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, dest, num_vals, BFROP_TYPE_PID_T))) {
        }
    } else {
        /* slow path - types are different sizes */
        UNPACK_SIZE_MISMATCH(pid_t, remote_type, ret);
    }

    return ret;
}


/* UNPACK FUNCTIONS FOR NON-GENERIC SYSTEM TYPES */

/*
 * BYTE, CHAR, INT8
 */
scon_status_t scon_bfrop_unpack_byte(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type)
 {
    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_byte * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, *num_vals)) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    memcpy(dest, buffer->unpack_ptr, *num_vals);

    /* update buffer pointer */
    buffer->unpack_ptr += *num_vals;

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_int16(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
{
    int32_t i;
    uint16_t tmp, *desttmp = (uint16_t*) dest;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_int16 * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, (*num_vals)*sizeof(tmp))) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    for (i = 0; i < (*num_vals); ++i) {
        memcpy( &(tmp), buffer->unpack_ptr, sizeof(tmp) );
        tmp = scon_ntohs(tmp);
        memcpy(&desttmp[i], &tmp, sizeof(tmp));
        buffer->unpack_ptr += sizeof(tmp);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_int32(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
{
    int32_t i;
    uint32_t tmp, *desttmp = (uint32_t*) dest;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_int32 * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, (*num_vals)*sizeof(tmp))) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    for (i = 0; i < (*num_vals); ++i) {
        memcpy( &(tmp), buffer->unpack_ptr, sizeof(tmp) );
        tmp = ntohl(tmp);
        memcpy(&desttmp[i], &tmp, sizeof(tmp));
        buffer->unpack_ptr += sizeof(tmp);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_datatype(scon_buffer_t *buffer, void *dest,
                                         int32_t *num_vals, scon_data_type_t type)
{
    return scon_bfrop_unpack_int16(buffer, dest, num_vals, type);
}

scon_status_t scon_bfrop_unpack_int64(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
{
    int32_t i;
    uint64_t tmp, *desttmp = (uint64_t*) dest;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_int64 * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, (*num_vals)*sizeof(tmp))) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    for (i = 0; i < (*num_vals); ++i) {
        memcpy( &(tmp), buffer->unpack_ptr, sizeof(tmp) );
        tmp = scon_ntoh64(tmp);
        memcpy(&desttmp[i], &tmp, sizeof(tmp));
        buffer->unpack_ptr += sizeof(tmp);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_string(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type)
{
    scon_status_t ret;
    int32_t i, len, n=1;
    char **sdest = (char**) dest;

    for (i = 0; i < (*num_vals); ++i) {
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_int32(buffer, &len, &n, SCON_INT32))) {
            return ret;
        }
        if (0 ==  len) {   /* zero-length string - unpack the NULL */
            sdest[i] = NULL;
        } else {
            sdest[i] = (char*)malloc(len);
            if (NULL == sdest[i]) {
                return SCON_ERR_OUT_OF_RESOURCE;
            }
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_byte(buffer, sdest[i], &len, SCON_BYTE))) {
                return ret;
            }
        }
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_float(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
{
    int32_t i, n;
    float *desttmp = (float*) dest, tmp;
    scon_status_t ret;
    char *convert;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_float * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, (*num_vals)*sizeof(float))) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    for (i = 0; i < (*num_vals); ++i) {
        n=1;
        convert = NULL;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_string(buffer, &convert, &n, SCON_STRING))) {
            return ret;
        }
        if (NULL != convert) {
            tmp = strtof(convert, NULL);
            memcpy(&desttmp[i], &tmp, sizeof(tmp));
            free(convert);
        }
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_double(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type)
{
    int32_t i, n;
    double *desttmp = (double*) dest, tmp;
    scon_status_t ret;
    char *convert;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_double * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, (*num_vals)*sizeof(double))) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    for (i = 0; i < (*num_vals); ++i) {
        n=1;
        convert = NULL;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_string(buffer, &convert, &n, SCON_STRING))) {
            return ret;
        }
        if (NULL != convert) {
            tmp = strtod(convert, NULL);
            memcpy(&desttmp[i], &tmp, sizeof(tmp));
            free(convert);
        }
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_timeval(scon_buffer_t *buffer, void *dest,
                                        int32_t *num_vals, scon_data_type_t type)
{
    int32_t i, n;
    int64_t tmp[2];
    struct timeval *desttmp = (struct timeval *) dest, tt;
    scon_status_t ret;

    scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_timeval * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, (*num_vals)*sizeof(struct timeval))) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    for (i = 0; i < (*num_vals); ++i) {
        n=2;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_int64(buffer, tmp, &n, SCON_INT64))) {
            return ret;
        }
        tt.tv_sec = tmp[0];
        tt.tv_usec = tmp[1];
        memcpy(&desttmp[i], &tt, sizeof(tt));
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_time(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type)
{
    int32_t i, n;
    time_t *desttmp = (time_t *) dest, tmp;
    scon_status_t ret;
    uint64_t ui64;

    /* time_t is a system-dependent size, so cast it
     * to uint64_t as a generic safe size
     */

     scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_time * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
     if (scon_bfrop_too_small(buffer, (*num_vals)*(sizeof(uint64_t)))) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    for (i = 0; i < (*num_vals); ++i) {
        n=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_int64(buffer, &ui64, &n, SCON_UINT64))) {
            return ret;
        }
        tmp = (time_t)ui64;
        memcpy(&desttmp[i], &tmp, sizeof(tmp));
    }
    return SCON_SUCCESS;
}


scon_status_t scon_bfrop_unpack_status(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type)
{
     scon_output_verbose(20, scon_globals.debug_output, "scon_bfrop_unpack_status * %d\n", (int)*num_vals);
    /* check to see if there's enough data in buffer */
    if (scon_bfrop_too_small(buffer, (*num_vals)*(sizeof(scon_status_t)))) {
        return SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    /* unpack the data */
    return scon_bfrop_unpack_int32(buffer, dest, num_vals, SCON_INT32);
}


/* UNPACK FUNCTIONS FOR GENERIC SCON TYPES */

/*
 * SCON_VALUE
 */
 static scon_status_t unpack_val(scon_buffer_t *buffer, scon_value_t *val)
 {
    int32_t m;
    scon_status_t ret;

    m = 1;
    switch (val->type) {
        case SCON_UNDEF:
            break;
        case SCON_BOOL:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.flag, &m, SCON_BOOL))) {
                return ret;
            }
            break;
        case SCON_BYTE:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.byte, &m, SCON_BYTE))) {
                return ret;
            }
            break;
        case SCON_STRING:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.string, &m, SCON_STRING))) {
                return ret;
            }
            break;
        case SCON_SIZE:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.size, &m, SCON_SIZE))) {
                return ret;
            }
            break;
        case SCON_PID:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.pid, &m, SCON_PID))) {
                return ret;
            }
            break;
        case SCON_INT:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.integer, &m, SCON_INT))) {
                return ret;
            }
            break;
        case SCON_INT8:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.int8, &m, SCON_INT8))) {
                return ret;
            }
            break;
        case SCON_INT16:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.int16, &m, SCON_INT16))) {
                return ret;
            }
            break;
        case SCON_INT32:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.int32, &m, SCON_INT32))) {
                return ret;
            }
            break;
        case SCON_INT64:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.int64, &m, SCON_INT64))) {
                return ret;
            }
            break;
        case SCON_UINT:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.uint, &m, SCON_UINT))) {
                return ret;
            }
            break;
        case SCON_UINT8:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.uint8, &m, SCON_UINT8))) {
                return ret;
            }
            break;
        case SCON_UINT16:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.uint16, &m, SCON_UINT16))) {
                return ret;
            }
            break;
        case SCON_UINT32:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.uint32, &m, SCON_UINT32))) {
                return ret;
            }
            break;
        case SCON_UINT64:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.uint64, &m, SCON_UINT64))) {
                return ret;
            }
            break;
        case SCON_FLOAT:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.fval, &m, SCON_FLOAT))) {
                return ret;
            }
            break;
        case SCON_DOUBLE:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.dval, &m, SCON_DOUBLE))) {
                return ret;
            }
            break;
        case SCON_TIMEVAL:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.tv, &m, SCON_TIMEVAL))) {
                return ret;
            }
            break;
        case SCON_TIME:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.time, &m, SCON_TIME))) {
                return ret;
            }
            break;
        case SCON_STATUS:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.status, &m, SCON_STATUS))) {
                return ret;
            }
            break;
        case SCON_PROC:
            /* this field is now a pointer, so we must allocate storage for it */
            SCON_PROC_CREATE(val->data.proc, 1);
            if (NULL == val->data.proc) {
                return SCON_ERR_NOMEM;
            }
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, val->data.proc, &m, SCON_PROC))) {
                return ret;
            }
            break;
        case SCON_INFO:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, val->data.info, &m, SCON_INFO))) {
                return ret;
            }
            break;
        case SCON_BYTE_OBJECT:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.bo, &m, SCON_BYTE_OBJECT))) {
                return ret;
            }
            break;
      /*  case SCON_DATA_RANGE:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, &val->data.range, &m, SCON_DATA_RANGE))) {
                return ret;
            }
            break;*/
        case SCON_DATA_ARRAY:
            /* this is now a pointer, so allocate storage for it */
            val->data.darray = (scon_data_array_t*)malloc(sizeof(scon_data_array_t));
            if (NULL == val->data.darray) {
                return SCON_ERR_NOMEM;
            }
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, val->data.darray, &m, SCON_DATA_ARRAY))) {
                return ret;
            }
            break;
       /* case SCON_QUERY:
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, val->data.darray, &m, SCON_QUERY))) {
                return ret;
            }
            break;*/
        /**** DEPRECATED ****
        case SCON_INFO_ARRAY:
            // this field is now a pointer, so we must allocate storage for it
            val->data.array = (scon_info_array_t*)malloc(sizeof(scon_info_array_t));
            if (NULL == val->data.array) {
                return SCON_ERR_NOMEM;
            }
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, val->data.array, &m, SCON_INFO_ARRAY))) {
                return ret;
            }
            break;
        ********************/
        default:
        scon_output(0, "UNPACK-SCON-VALUE: UNSUPPORTED TYPE %d", (int)val->type);
        return SCON_ERROR;
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_value(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
{
    scon_value_t *ptr;
    int32_t i, n;
    scon_status_t ret;

    ptr = (scon_value_t *) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        /* unpack the type */
        if (SCON_SUCCESS != (ret = scon_bfrop_get_data_type(buffer, &ptr[i].type))) {
            return ret;
        }
        /* unpack value */
        if (SCON_SUCCESS != (ret = unpack_val(buffer, &ptr[i])) ) {
            return ret;
        }
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_info(scon_buffer_t *buffer, void *dest,
                           int32_t *num_vals, scon_data_type_t type)
{
    scon_info_t *ptr;
    int32_t i, n, m;
    scon_status_t ret;
    char *tmp;

    scon_output_verbose(20, scon_globals.debug_output,
                        "scon_bfrop_unpack: %d info", *num_vals);

    ptr = (scon_info_t *) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        memset(ptr[i].key, 0, sizeof(ptr[i].key));
        memset(&ptr[i].value, 0, sizeof(scon_value_t));
        /* unpack key */
        m=1;
        tmp = NULL;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_string(buffer, &tmp, &m, SCON_STRING))) {
            return ret;
        }
        if (NULL == tmp) {
            return SCON_ERROR;
        }
        (void)strncpy(ptr[i].key, tmp, SCON_MAX_KEYLEN);
        free(tmp);
        /* unpack the flags */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_infodirs(buffer, &ptr[i].flags, &m, SCON_INFO_DIRECTIVES))) {
            return ret;
        }
        /* unpack value - since the value structure is statically-defined
         * instead of a pointer in this struct, we directly unpack it to
         * avoid the malloc */
         m=1;
         if (SCON_SUCCESS != (ret = scon_bfrop_unpack_int(buffer, &ptr[i].value.type, &m, SCON_INT))) {
            return ret;
        }
        scon_output_verbose(20, scon_globals.debug_output,
                            "scon_bfrop_unpack: info type %d", ptr[i].value.type);
        m=1;
        if (SCON_SUCCESS != (ret = unpack_val(buffer, &ptr[i].value))) {
            return ret;
        }
    }
    return SCON_SUCCESS;
}


scon_status_t scon_bfrop_unpack_buf(scon_buffer_t *buffer, void *dest,
                          int32_t *num_vals, scon_data_type_t type)
{
    scon_buffer_t **ptr;
    int32_t i, n, m;
    scon_status_t ret;
    size_t nbytes;

    ptr = (scon_buffer_t **) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        /* allocate the new object */
        //ptr[i] = SCON_NEW(scon_buffer_t);
        ptr[i] = (scon_buffer_t*) malloc(sizeof(scon_buffer_t));
        if (NULL == ptr[i]) {
            return SCON_ERR_OUT_OF_RESOURCE;
        }
        /* unpack the number of bytes */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_sizet(buffer, &nbytes, &m, SCON_SIZE))) {
            return ret;
        }
        m = nbytes;
        /* setup the buffer's data region */
        if (0 < nbytes) {
            ptr[i]->base_ptr = (char*)malloc(nbytes);
            /* unpack the bytes */
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_byte(buffer, ptr[i]->base_ptr, &m, SCON_BYTE))) {
                return ret;
            }
        }
        ptr[i]->pack_ptr = ptr[i]->base_ptr + m;
        ptr[i]->unpack_ptr = ptr[i]->base_ptr;
        ptr[i]->bytes_allocated = nbytes;
        ptr[i]->bytes_used = m;
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_proc(scon_buffer_t *buffer, void *dest,
                           int32_t *num_vals, scon_data_type_t type)
{
    scon_proc_t *ptr;
    int32_t i, n, m;
    scon_status_t ret;
    char *tmp;

    scon_output_verbose(20, scon_globals.debug_output,
                        "scon_bfrop_unpack: %d procs", *num_vals);

    ptr = (scon_proc_t *) dest;
    n = *num_vals;
    for (i = 0; i < n; ++i) {
        /* unpack job name */
        m=1;
        tmp = NULL;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_string(buffer, &tmp, &m, SCON_STRING))) {
            return ret;
        }
        if (NULL == tmp) {
            return SCON_ERROR;
        }
        (void)strncpy(ptr[i].job_name, tmp, SCON_MAX_NSLEN);
        free(tmp);
        /* unpack the rank */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_rank(buffer, &ptr[i].rank, &m, SCON_PROC_RANK))) {
            return ret;
        }
    }
    return SCON_SUCCESS;
}

#if 0
scon_status_t scon_bfrop_unpack_kval(scon_buffer_t *buffer, void *dest,
                           int32_t *num_vals, scon_data_type_t type)
{
    scon_kval_t *ptr;
    int32_t i, n, m;
    scon_status_t ret;

    scon_output_verbose(20, scon_globals.debug_output,
                        "scon_bfrop_unpack: %d kvals", *num_vals);

    ptr = (scon_kval_t*) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        SCON_CONSTRUCT(&ptr[i], scon_kval_t);
        /* unpack the key */
        m = 1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_string(buffer, &ptr[i].key, &m, SCON_STRING))) {
            SCON_ERROR_LOG(ret);
            return ret;
        }
        /* allocate the space */
        ptr[i].value = (scon_value_t*)malloc(sizeof(scon_value_t));
        /* unpack the value */
        m = 1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_value(buffer, ptr[i].value, &m, SCON_VALUE))) {
            SCON_ERROR_LOG(ret);
            return ret;
        }
    }
    return SCON_SUCCESS;
}
#endif

scon_status_t scon_bfrop_unpack_infodirs(scon_buffer_t *buffer, void *dest,
                                         int32_t *num_vals, scon_data_type_t type)
{
    return scon_bfrop_unpack_int32(buffer, dest, num_vals, SCON_UINT32);
}

scon_status_t scon_bfrop_unpack_bo(scon_buffer_t *buffer, void *dest,
                         int32_t *num_vals, scon_data_type_t type)
{
    scon_byte_object_t *ptr;
    int32_t i, n, m;
    scon_status_t ret;

    scon_output_verbose(20, scon_globals.debug_output,
                        "scon_bfrop_unpack: %d byte_object", *num_vals);

    ptr = (scon_byte_object_t *) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        memset(&ptr[i], 0, sizeof(scon_byte_object_t));
        /* unpack the number of bytes */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_sizet(buffer, &ptr[i].size, &m, SCON_SIZE))) {
            return ret;
        }
        if (0 < ptr[i].size) {
            ptr[i].bytes = (char*)malloc(ptr[i].size * sizeof(char));
            m=ptr[i].size;
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_byte(buffer, ptr[i].bytes, &m, SCON_BYTE))) {
                return ret;
            }
        }
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_ptr(scon_buffer_t *buffer, void *dest,
                          int32_t *num_vals, scon_data_type_t type)
{
    uint8_t foo=1;
    int32_t cnt=1;

    /* it obviously makes no sense to pack a pointer and
     * send it somewhere else, so we just unpack the sentinel */
    return scon_bfrop_unpack_byte(buffer, &foo, &cnt, SCON_UINT8);
}


scon_status_t scon_bfrop_unpack_darray(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type)
{
    scon_data_array_t *ptr;
    int32_t i, n, m;
    scon_status_t ret;
    size_t nbytes;

    scon_output_verbose(20, scon_globals.debug_output,
                        "scon_bfrop_unpack: %d data arrays", *num_vals);

    ptr = (scon_data_array_t *) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        memset(&ptr[i], 0, sizeof(scon_data_array_t));
        /* unpack the type */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_datatype(buffer, &ptr[i].type, &m, SCON_DATA_TYPE))) {
            return ret;
        }
        /* unpack the number of array elements */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_sizet(buffer, &ptr[i].size, &m, SCON_SIZE))) {
            return ret;
        }
        if (0 == ptr[i].size || SCON_UNDEF == ptr[i].type) {
            /* nothing else to do */
            continue;
        }
        /* allocate storage for the array and unpack the array elements */
        m = ptr[i].size;
        switch(ptr[i].type) {
            case SCON_BOOL:
                nbytes = sizeof(bool);
                break;
            case SCON_BYTE:
            case SCON_INT8:
            case SCON_UINT8:
                nbytes = sizeof(int8_t);
                break;
            case SCON_INT16:
            case SCON_UINT16:
                nbytes = sizeof(int16_t);
                break;
            case SCON_INT32:
            case SCON_UINT32:
                nbytes = sizeof(int32_t);
                break;
            case SCON_INT64:
            case SCON_UINT64:
                nbytes = sizeof(int64_t);
                break;
            case SCON_STRING:
                nbytes = sizeof(char*);
                break;
            case SCON_SIZE:
                nbytes = sizeof(size_t);
                break;
            case SCON_PID:
                nbytes = sizeof(pid_t);
                break;
            case SCON_INT:
            case SCON_UINT:
                nbytes = sizeof(int);
                break;
            case SCON_FLOAT:
                nbytes = sizeof(float);
                break;
            case SCON_DOUBLE:
                nbytes = sizeof(double);
                break;
            case SCON_TIMEVAL:
                nbytes = sizeof(struct timeval);
                break;
            case SCON_TIME:
                nbytes = sizeof(time_t);
                break;
            case SCON_STATUS:
                nbytes = sizeof(scon_status_t);
                break;
            case SCON_PROC:
                nbytes = sizeof(scon_proc_t);
                break;
            case SCON_BYTE_OBJECT:
                nbytes = sizeof(scon_byte_object_t);
                break;
    /*        case SCON_DATA_RANGE:
                nbytes = sizeof(scon_data_range_t);
                break;
            case SCON_QUERY:
                nbytes = sizeof(scon_query_t);*/
            default:
                return SCON_ERR_NOT_SUPPORTED;
        }
        if (NULL == (ptr[i].array = malloc(m * nbytes))) {
            return SCON_ERR_NOMEM;
        }
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_buffer(buffer, ptr[i].array, &m, ptr[i].type))) {
            return ret;
        }
    }
    return SCON_SUCCESS;
}

int scon_bfrop_unpack_coll_sig(scon_buffer_t *buffer, void *dest, int32_t *num_vals,
                       scon_data_type_t type)
{
    scon_collectives_signature_t **ptr;
    int32_t i, n, cnt;
    int rc;

    ptr = (scon_collectives_signature_t **) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        /* allocate the new object */
        ptr[i] = SCON_NEW(scon_collectives_signature_t);
        if (NULL == ptr[i]) {
            return SCON_ERR_OUT_OF_RESOURCE;
        }
        /* unpack the scon handle  */
        cnt = 1;
        if (SCON_SUCCESS != (rc = scon_bfrop_unpack_datatype(buffer, &ptr[i]->scon_handle, &cnt, SCON_INT32))) {
            return rc;
        }
        /* unpack the #procs */
        cnt = 1;
        if (SCON_SUCCESS != (rc = scon_bfrop_unpack_datatype(buffer, &ptr[i]->nprocs, &cnt, SCON_SIZE))) {
            return rc;
        }
        if (0 < ptr[i]->nprocs) {
            /* allocate space for the array */
            /* unpack the array - the array is our signature for the collective */
            cnt = ptr[i]->nprocs;
            SCON_PROC_CREATE(ptr[i]->procs, cnt);
            if (SCON_SUCCESS != (rc = scon_bfrop_unpack_proc(buffer, ptr[i]->procs, &cnt, SCON_PROC))) {
                SCON_RELEASE(ptr[i]);
                return rc;
            }
        }
        cnt = 1;
        if (SCON_SUCCESS != (rc = scon_bfrop_unpack_datatype(buffer, &ptr[i]->seq_num, &cnt, SCON_UINT32))) {
            return rc;
        }

    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_unpack_range(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
{
    return scon_bfrop_unpack_byte(buffer, dest, num_vals, SCON_UINT8);
}

scon_status_t scon_bfrop_unpack_rank(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type)
{
    return scon_bfrop_unpack_int32(buffer, dest, num_vals, SCON_UINT32);
}

#if 0
scon_status_t scon_bfrop_unpack_query(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
{
    scon_query_t *ptr;
    int32_t i, n, m;
    scon_status_t ret;
    int32_t nkeys;

    scon_output_verbose(20, scon_globals.debug_output,
                        "scon_bfrop_unpack: %d queries", *num_vals);

    ptr = (scon_query_t *) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        SCON_QUERY_CONSTRUCT(&ptr[i]);
        /* unpack the number of keys */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_int32(buffer, &nkeys, &m, SCON_INT32))) {
            return ret;
        }
        if (0 < nkeys) {
            /* unpack the keys */
            if (NULL == (ptr[i].keys = (char**)calloc(nkeys+1, sizeof(char*)))) {
                return SCON_ERR_NOMEM;
            }
            /* unpack keys */
            m=nkeys;
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_string(buffer, ptr[i].keys, &m, SCON_STRING))) {
                return ret;
            }
        }
        /* unpack the number of qualifiers */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_sizet(buffer, &ptr[i].nqual, &m, SCON_SIZE))) {
            return ret;
        }
        if (0 < ptr[i].nqual) {
            /* unpack the qualifiers */
            SCON_INFO_CREATE(ptr[i].qualifiers, ptr[i].nqual);
            m =  ptr[i].nqual;
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_info(buffer, ptr[i].qualifiers, &m, SCON_INFO))) {
                return ret;
            }
        }
    }
    return SCON_SUCCESS;
}

/**** DEPRECATED ****/
scon_status_t scon_bfrop_unpack_array(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type)
{
    scon_info_array_t *ptr;
    int32_t i, n, m;
    scon_status_t ret;

    scon_output_verbose(20, scon_globals.debug_output,
                        "scon_bfrop_unpack: %d info arrays", *num_vals);

    ptr = (scon_info_array_t*) dest;
    n = *num_vals;

    for (i = 0; i < n; ++i) {
        scon_output_verbose(20, scon_globals.debug_output,
                            "scon_bfrop_unpack: init array[%d]", i);
        memset(&ptr[i], 0, sizeof(scon_info_array_t));
        /* unpack the size of this array */
        m=1;
        if (SCON_SUCCESS != (ret = scon_bfrop_unpack_sizet(buffer, &ptr[i].size, &m, SCON_SIZE))) {
            return ret;
        }
        if (0 < ptr[i].size) {
            ptr[i].array = (scon_info_t*)malloc(ptr[i].size * sizeof(scon_info_t));
            m=ptr[i].size;
            if (SCON_SUCCESS != (ret = scon_bfrop_unpack_value(buffer, ptr[i].array, &m, SCON_INFO))) {
                return ret;
            }
        }
    }
    return SCON_SUCCESS;
}
/********************/
#endif
