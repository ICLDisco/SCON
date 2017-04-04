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
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * Copyright (c) 2016      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include <src/include/scon_stdint.h>

#include <stdio.h>
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "src/util/error.h"
#include "src/include/scon_globals.h"
#include "src/buffer_ops/internal.h"
#include "src/util/output.h"
#include "src/util/name_fns.h"
 scon_status_t scon_bfrop_print(char **output, char *prefix, void *src, scon_data_type_t type)
 {
    scon_bfrop_type_info_t *info;

    /* check for error */
    if (NULL == output) {
        return SCON_ERR_BAD_PARAM;
    }

    /* Lookup the print function for this type and call it */

    if(NULL == (info = (scon_bfrop_type_info_t*)scon_pointer_array_get_item(&scon_bfrop_types, type))) {
        return SCON_ERR_UNKNOWN_DATA_TYPE;
    }

    return info->odti_print_fn(output, prefix, src, type);
}

/*
 * STANDARD PRINT FUNCTIONS FOR SYSTEM TYPES
 */
 scon_status_t scon_bfrop_print_bool(char **output, char *prefix, bool *src, scon_data_type_t type)
 {
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    }
    else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_BOOL\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_BOOL\tValue: %s", prefix,
                     (*src) ? "TRUE" : "FALSE")) {
        return SCON_ERR_NOMEM;
}
if (prefx != prefix) {
    free(prefx);
}

return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_byte(char **output, char *prefix, uint8_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_BYTE\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_BYTE\tValue: %x", prefix, *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_string(char **output, char *prefix, char *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_STRING\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_STRING\tValue: %s", prefx, src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_size(char **output, char *prefix, size_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_SIZE\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_SIZE\tValue: %lu", prefx, (unsigned long) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_pid(char **output, char *prefix, pid_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_PID\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_PID\tValue: %lu", prefx, (unsigned long) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_int(char **output, char *prefix, int *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_INT\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_INT\tValue: %ld", prefx, (long) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_uint(char **output, char *prefix, uint *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_UINT\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_UINT\tValue: %lu", prefx, (unsigned long) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_uint8(char **output, char *prefix, uint8_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_UINT8\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_UINT8\tValue: %u", prefx, (unsigned int) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_uint16(char **output, char *prefix, uint16_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_UINT16\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_UINT16\tValue: %u", prefx, (unsigned int) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_uint32(char **output, char *prefix,
                                      uint32_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_UINT32\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_UINT32\tValue: %u", prefx, (unsigned int) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_int8(char **output, char *prefix,
                                    int8_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_INT8\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_INT8\tValue: %d", prefx, (int) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_int16(char **output, char *prefix,
                                     int16_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_INT16\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_INT16\tValue: %d", prefx, (int) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_int32(char **output, char *prefix, int32_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_INT32\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_INT32\tValue: %d", prefx, (int) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}
scon_status_t scon_bfrop_print_uint64(char **output, char *prefix,
                                      uint64_t *src,
                                      scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_UINT64\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_UINT64\tValue: %lu", prefx, (unsigned long) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_int64(char **output, char *prefix,
                                     int64_t *src,
                                     scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_INT64\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_INT64\tValue: %ld", prefx, (long) *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_float(char **output, char *prefix,
                                     float *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_FLOAT\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_FLOAT\tValue: %f", prefx, *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_double(char **output, char *prefix,
                                      double *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_DOUBLE\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_DOUBLE\tValue: %f", prefx, *src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_time(char **output, char *prefix,
                                    time_t *src, scon_data_type_t type)
{
    char *prefx;
    char *t;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_TIME\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    t = ctime(src);
    t[strlen(t)-1] = '\0';  // remove trailing newline

    if (0 > asprintf(output, "%sData type: SCON_TIME\tValue: %s", prefx, t)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_timeval(char **output, char *prefix,
                                       struct timeval *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_TIMEVAL\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_TIMEVAL\tValue: %ld.%06ld", prefx,
                     (long)src->tv_sec, (long)src->tv_usec)) {
        return SCON_ERR_NOMEM;
}
if (prefx != prefix) {
    free(prefx);
}

return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_status(char **output, char *prefix,
                                      scon_status_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_STATUS\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_STATUS\tValue: %s", prefx, SCON_Error_string(*src))) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}


/* PRINT FUNCTIONS FOR GENERIC SCON TYPES */

/*
 * SCON_VALUE
 */
 scon_status_t scon_bfrop_print_value(char **output, char *prefix,
                                      scon_value_t *src, scon_data_type_t type)
 {
    char *prefx;
    int rc;
    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_VALUE\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    switch (src->type) {
        case SCON_UNDEF:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_UNDEF", prefx);
        break;
        case SCON_BYTE:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_BYTE\tValue: %x",
                      prefx, src->data.byte);
        break;
        case SCON_STRING:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_STRING\tValue: %s",
                      prefx, src->data.string);
        break;
        case SCON_SIZE:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_SIZE\tValue: %lu",
                      prefx, (unsigned long)src->data.size);
        break;
        case SCON_PID:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_PID\tValue: %lu",
                      prefx, (unsigned long)src->data.pid);
        break;
        case SCON_INT:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_INT\tValue: %d",
                      prefx, src->data.integer);
        break;
        case SCON_INT8:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_INT8\tValue: %d",
                      prefx, (int)src->data.int8);
        break;
        case SCON_INT16:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_INT16\tValue: %d",
                      prefx, (int)src->data.int16);
        break;
        case SCON_INT32:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_INT32\tValue: %d",
                      prefx, src->data.int32);
        break;
        case SCON_INT64:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_INT64\tValue: %ld",
                      prefx, (long)src->data.int64);
        break;
        case SCON_UINT:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_UINT\tValue: %u",
                      prefx, src->data.uint);
        break;
        case SCON_UINT8:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_UINT8\tValue: %u",
                      prefx, (unsigned int)src->data.uint8);
        break;
        case SCON_UINT16:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_UINT16\tValue: %u",
                      prefx, (unsigned int)src->data.uint16);
        break;
        case SCON_UINT32:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_UINT32\tValue: %u",
                      prefx, src->data.uint32);
        break;
        case SCON_UINT64:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_UINT64\tValue: %lu",
                      prefx, (unsigned long)src->data.uint64);
        break;
        case SCON_FLOAT:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_FLOAT\tValue: %f",
                      prefx, src->data.fval);
        break;
        case SCON_DOUBLE:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_DOUBLE\tValue: %f",
                      prefx, src->data.dval);
        break;
        case SCON_TIMEVAL:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_TIMEVAL\tValue: %ld.%06ld", prefx,
                      (long)src->data.tv.tv_sec, (long)src->data.tv.tv_usec);
        break;
        case SCON_TIME:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_TIME\tValue: %s", prefx,
                      ctime(&src->data.time));
        break;
        case SCON_STATUS:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_STATUS\tValue: %s", prefx,
                      SCON_Error_string(src->data.status));
        break;
        case SCON_PROC:
        if (NULL == src->data.proc) {
            rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_PROC\tNULL", prefx);
            scon_output(0, "print proc input is null, printing output=%s", output);
        } else {
            rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_PROC\t%s:%lu",
                          prefx, src->data.proc->job_name, (unsigned long)src->data.proc->rank);
            scon_output(0, "print proc: printing it as output=%s", output);
        }
        break;
        case SCON_BYTE_OBJECT:
        rc = asprintf(output, "%sSCON_VALUE: Data type: BYTE_OBJECT\tSIZE: %ld",
                      prefx, (long)src->data.bo.size);
        break;
       /* case SCON_DATA_RANGE:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_DATA_RANGE\tValue: %s",
                      prefx, SCON_Data_range_string(src->data.range));*
        break;
        case SCON_PROC_INFO:
        rc = asprintf(output, "%sSCON_VALUE: Data type: SCON_PROC_INFO\tProc: %s:%lu\n%s\tHost: %s\tExecutable: %s\tPid: %lu",
                      prefx, src->data.pinfo->proc.nspace, (unsigned long)src->data.pinfo->proc.rank,
                      prefx, src->data.pinfo->hostname, src->data.pinfo->executable_name,
                      (unsigned long)src->data.pinfo->pid);
        break;
        case SCON_DATA_ARRAY:
        rc = asprintf(output, "%sSCON_VALUE: Data type: DATA_ARRAY\tARRAY SIZE: %ld",
                      prefx, (long)src->data.darray->size);
        break;*/
        /**** DEPRECATED ****/
        /*case SCON_INFO_ARRAY:
        rc = asprintf(output, "%sSCON_VALUE: Data type: INFO_ARRAY\tARRAY SIZE: %ld",
                      prefx, (long)src->data.array->size);
        break;*/
        /********************/
        default:
        rc = asprintf(output, "%sSCON_VALUE: Data type: UNKNOWN\tValue: UNPRINTABLE", prefx);
        break;
    }
    if (prefx != prefix) {
        free(prefx);
    }
    if (0 > rc) {
        return SCON_ERR_NOMEM;
    }
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_info(char **output, char *prefix,
                                    scon_info_t *src, scon_data_type_t type)
{
    char *tmp;
    int rc;

    scon_bfrop_print_value(&tmp, NULL, &src->value, SCON_VALUE);
    rc = asprintf(output, "%sKEY: %s DIRECTIVES: %0x %s", prefix, src->key,
                  src->flags, (NULL == tmp) ? "SCON_VALUE: NULL" : tmp);
    if (NULL != tmp) {
        free(tmp);
    }
    if (0 > rc) {
        return SCON_ERR_NOMEM;
    }
    return SCON_SUCCESS;
}


scon_status_t scon_bfrop_print_buf(char **output, char *prefix,
                                   scon_buffer_t *src, scon_data_type_t type)
{
    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_proc(char **output, char *prefix,
                                    scon_proc_t *src, scon_data_type_t type)
{
    char *prefx;
    int rc;
    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            scon_output(0, "scon_bfrop_print_proc returning SCON_ERR_NOMEM");
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    switch(src->rank) {
        case SCON_RANK_UNDEF:
            rc = asprintf(output,
                          "%sPROC:%s:SCON_RANK_UNDEF", prefx, src->job_name);
            break;
        case SCON_RANK_WILDCARD:
            rc = asprintf(output,
                          "%sPROC:%s:SCON_RANK_WILDCARD", prefx, src->job_name);
            break;
        default:
            rc = asprintf(output,
                          "%sPROC:%s:%lu", prefx, src->job_name,
                          (unsigned long)(src->rank));
    }
    if (prefx != prefix) {
        free(prefx);
    }
    if (0 > rc) {
        scon_output(0, "scon_bfrop_print_proc returning SCON_ERR_NOMEM");
        return SCON_ERR_NOMEM;
    }
    return SCON_SUCCESS;
}



scon_status_t scon_bfrop_print_range(char **output, char *prefix,
                                     scon_member_range_t *src,
                                     scon_data_type_t type)
{
 /*   char *prefx;

 */   /* deal with NULL prefix */
 /*   if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    if (0 > asprintf(output, "%sData type: SCON_DATA_RANGE\tValue: %s",
                     prefx, SCON_Data_range_string(*src))) {
        return SCON_ERR_NOMEM;
}
if (prefx != prefix) {
    free(prefx);
}
*/
return SCON_SUCCESS;
}


scon_status_t scon_bfrop_print_infodirs(char **output, char *prefix,
                                        scon_info_directives_t *src,
                                        scon_data_type_t type)
{
  /*  char *prefx;
*/
    /* deal with NULL prefix */
 /*   if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    if (0 > asprintf(output, "%sData type: SCON_INFO_DIRECTIVES\tValue: %s",
                     prefx, SCON_Info_directives_string(*src))) {
        return SCON_ERR_NOMEM;
}
if (prefx != prefix) {
    free(prefx);
}*/

return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_bo(char **output, char *prefix,
                                  scon_byte_object_t *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        if (0 > asprintf(output, "%sData type: SCON_BYTE_OBJECT\tValue: NULL pointer", prefx)) {
            return SCON_ERR_NOMEM;
        }
        if (prefx != prefix) {
            free(prefx);
        }
        return SCON_SUCCESS;
    }

    if (0 > asprintf(output, "%sData type: SCON_BYTE_OBJECT\tSize: %ld", prefx, (long)src->size)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}

scon_status_t scon_bfrop_print_ptr(char **output, char *prefix,
                                   void *src, scon_data_type_t type)
{
    char *prefx;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    if (0 > asprintf(output, "%sData type: SCON_POINTER\tAddress: %p", prefx, src)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }

    return SCON_SUCCESS;
}


scon_status_t scon_bfrop_print_darray(char **output, char *prefix,
                                      scon_data_array_t *src, scon_data_type_t type)
{
  /*  char *prefx;
*/
    /* deal with NULL prefix */
 /*   if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    if (0 > asprintf(output, "%sData type: SCON_DATA_ARRAY\tSize: %lu",
                     prefx, (unsigned long)src->size)) {
        return SCON_ERR_NOMEM;
    }
    if (prefx != prefix) {
        free(prefx);
    }
*/
    return SCON_SUCCESS;
}

int scon_bfrop_print_coll_sig(char **output, char *prefix,
                              scon_collectives_signature_t *src,
                              scon_data_type_t type)
{
    char *prefx;
    size_t i;
    char *tmp, *tmp2;

    /* deal with NULL prefix */
    if (NULL == prefix) asprintf(&prefx, " ");
    else prefx = strdup(prefix);

    /* if src is NULL, just print data type and return */
    if (NULL == src) {
        asprintf(output, "%sData type: COLLECTIVES_SIG", prefx);
        free(prefx);
        return SCON_SUCCESS;
    }

    if (NULL == src->procs) {
        asprintf(output, "%s COLLECTIVES_SIG  SeqNumber:%d  Procs: NULL", prefx, src->seq_num);
        free(prefx);
        return SCON_SUCCESS;
    }

    /* there must be at least one proc in the signature */
    asprintf(&tmp, "%sCOLLECTIVES_SIG  SeqNumber:%d  Procs: ", prefx, src->seq_num);

    for (i=0; i < src->nprocs; i++) {
        asprintf(&tmp2, "%s%s", tmp, SCON_PRINT_PROC(&src->procs[i]));
        free(tmp);
        tmp = tmp2;
    }
    *output = tmp;
    return SCON_SUCCESS;
}

#if 0
scon_status_t scon_bfrop_print_query(char **output, char *prefix,
                                     scon_query_t *src, scon_data_type_t type)
{
    char *prefx, *p2;
    scon_status_t rc = SCON_SUCCESS;
    char *tmp, *t2, *t3;
    size_t n;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    if (0 > asprintf(&p2, "%s\t", prefx)) {
        rc = SCON_ERR_NOMEM;
        goto done;
    }

    if (0 > asprintf(&tmp,
                   "%sData type: SCON_QUERY\tValue:", prefx)) {
        free(p2);
        rc = SCON_ERR_NOMEM;
        goto done;
    }

    /* print out the keys */
    if (NULL != src->keys) {
        for (n=0; NULL != src->keys[n]; n++) {
            if (0 > asprintf(&t2, "%s\n%sKey: %s", tmp, p2, src->keys[n])) {
                free(p2);
                free(tmp);
                rc = SCON_ERR_NOMEM;
                goto done;
            }
            free(tmp);
            tmp = t2;
        }
    }

    /* now print the qualifiers */
    if (0 < src->nqual) {
        for (n=0; n < src->nqual; n++) {
            if (SCON_SUCCESS != (rc = scon_bfrop_print_info(&t2, p2, &src->qualifiers[n], SCON_PROC))) {
                free(p2);
                goto done;
            }
            if (0 > asprintf(&t3, "%s\n%s", tmp, t2)) {
                free(p2);
                free(tmp);
                free(t2);
                rc = SCON_ERR_NOMEM;
                goto done;
            }
            free(tmp);
            free(t2);
            tmp = t3;
        }
    }
    *output = tmp;

  done:
    if (prefx != prefix) {
        free(prefx);
    }

    return rc;
}
#endif

scon_status_t scon_bfrop_print_rank(char **output, char *prefix,
                                    scon_rank_t *src, scon_data_type_t type)
{
    char *prefx;
    int rc;

    /* deal with NULL prefix */
    if (NULL == prefix) {
        if (0 > asprintf(&prefx, " ")) {
            return SCON_ERR_NOMEM;
        }
    } else {
        prefx = prefix;
    }

    switch(*src) {
        case SCON_RANK_UNDEF:
            rc = asprintf(output,
                          "%sData type: SCON_PROC_RANK\tValue: SCON_RANK_UNDEF",
                          prefx);
            break;
        case SCON_RANK_WILDCARD:
            rc = asprintf(output,
                          "%sData type: SCON_PROC_RANK\tValue: SCON_RANK_WILDCARD",
                          prefx);
            break;

        default:
            rc = asprintf(output, "%sData type: SCON_PROC_RANK\tValue: %lu",
                          prefx, (unsigned long)(*src));
    }
    if (prefx != prefix) {
        free(prefx);
    }
    if (0 > rc) {
        return SCON_ERR_NOMEM;
    }
    return SCON_SUCCESS;
}

#if 0
/**** DEPRECATED ****/
scon_status_t scon_bfrop_print_array(char **output, char *prefix,
                                     scon_info_array_t *src, scon_data_type_t type)
{
    size_t j;
    char *tmp, *tmp2, *tmp3, *pfx;
    scon_info_t *s1;

    if (0 > asprintf(&tmp, "%sARRAY SIZE: %ld", prefix, (long)src->size)) {
        return SCON_ERR_NOMEM;
    }
    if (0 > asprintf(&pfx, "\n%s\t",  (NULL == prefix) ? "" : prefix)) {
        free(tmp);
        return SCON_ERR_NOMEM;
    }
    s1 = (scon_info_t*)src->array;

    for (j=0; j < src->size; j++) {
        scon_bfrop_print_info(&tmp2, pfx, &s1[j], SCON_INFO);
        if (0 > asprintf(&tmp3, "%s%s", tmp, tmp2)) {
            free(tmp);
            free(tmp2);
            return SCON_ERR_NOMEM;
        }
        free(tmp);
        free(tmp2);
        tmp = tmp3;
    }
    *output = tmp;
    return SCON_SUCCESS;
}
/********************/
#endif
