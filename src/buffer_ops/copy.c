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
 * Copyright (c) 2014-2017 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include "scon_common.h"
#include "src/util/argv.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/buffer_ops/buffer_ops.h"
#include "src/buffer_ops/internal.h"
#include "src/buffer_ops/types.h"

 scon_status_t scon_bfrop_copy(void **dest, void *src, scon_data_type_t type)
{
    scon_bfrop_type_info_t *info;

    /* check for error */
    if (NULL == dest) {
        SCON_ERROR_LOG(SCON_ERR_BAD_PARAM);
        return SCON_ERR_BAD_PARAM;
    }
    if (NULL == src) {
        SCON_ERROR_LOG(SCON_ERR_BAD_PARAM);
        return SCON_ERR_BAD_PARAM;
    }

   /* Lookup the copy function for this type and call it */

    if (NULL == (info = (scon_bfrop_type_info_t*)scon_pointer_array_get_item(&scon_bfrop_types, type))) {
        SCON_ERROR_LOG(SCON_ERR_UNKNOWN_DATA_TYPE);
        return SCON_ERR_UNKNOWN_DATA_TYPE;
    }

    return info->odti_copy_fn(dest, src, type);
}

scon_status_t scon_bfrop_copy_payload(scon_buffer_t *dest, scon_buffer_t *src)
{
    size_t to_copy = 0;
    char *ptr;
    /* deal with buffer type */
    if( NULL == dest->base_ptr ){
        /* destination buffer is empty - derive src buffer type */
        dest->type = src->type;
    } else if( dest->type != src->type ){
        /* buffer types mismatch */
        SCON_ERROR_LOG(SCON_ERR_BAD_PARAM);
        return SCON_ERR_BAD_PARAM;
    }

    to_copy = src->pack_ptr - src->unpack_ptr;
    if( NULL == (ptr = scon_bfrop_buffer_extend(dest, to_copy)) ){
        SCON_ERROR_LOG(SCON_ERR_OUT_OF_RESOURCE);
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    memcpy(ptr,src->unpack_ptr, to_copy);
    dest->bytes_used += to_copy;
    dest->pack_ptr += to_copy;
    return SCON_SUCCESS;
}


/*
 * STANDARD COPY FUNCTION - WORKS FOR EVERYTHING NON-STRUCTURED
 */
 scon_status_t scon_bfrop_std_copy(void **dest, void *src, scon_data_type_t type)
{
    size_t datasize;
    uint8_t *val = NULL;

    switch(type) {
    case SCON_BOOL:
        datasize = sizeof(bool);
        break;

    case SCON_INT:
    case SCON_UINT:
        datasize = sizeof(int);
        break;

    case SCON_SIZE:
        datasize = sizeof(size_t);
        break;

    case SCON_PID:
        datasize = sizeof(pid_t);
        break;

    case SCON_BYTE:
    case SCON_INT8:
    case SCON_UINT8:
        datasize = 1;
        break;

    case SCON_INT16:
    case SCON_UINT16:
        datasize = 2;
        break;

    case SCON_INT32:
    case SCON_UINT32:
        datasize = 4;
        break;

    case SCON_INT64:
    case SCON_UINT64:
        datasize = 8;
        break;

    case SCON_FLOAT:
        datasize = sizeof(float);
        break;

    case SCON_TIMEVAL:
        datasize = sizeof(struct timeval);
        break;

    case SCON_TIME:
        datasize = sizeof(time_t);
        break;

    case SCON_STATUS:
        datasize = sizeof(scon_status_t);
        break;

    case SCON_PROC_RANK:
        datasize = sizeof(scon_rank_t);
        break;

    case SCON_POINTER:
        datasize = sizeof(char*);
        break;

    case SCON_DATA_RANGE:
        datasize = sizeof(scon_member_range_t);
        break;

    case SCON_INFO_DIRECTIVES:
        datasize = sizeof(scon_info_directives_t);
        break;


    default:
        return SCON_ERR_UNKNOWN_DATA_TYPE;
    }

    val = (uint8_t*)malloc(datasize);
    if (NULL == val) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    memcpy(val, src, datasize);
    *dest = val;

    return SCON_SUCCESS;
}

/* COPY FUNCTIONS FOR NON-STANDARD SYSTEM TYPES */

/*
 * STRING
 */
 scon_status_t scon_bfrop_copy_string(char **dest, char *src, scon_data_type_t type)
{
    if (NULL == src) {  /* got zero-length string/NULL pointer - store NULL */
        *dest = NULL;
    } else {
        *dest = strdup(src);
    }

    return SCON_SUCCESS;
}
/* compare function for scon_value_t */
bool scon_value_cmp(scon_value_t *p, scon_value_t *p1)
{
    bool rc = false;

    if (p->type != p1->type) {
        return rc;
    }

    switch (p->type) {
        case SCON_UNDEF:
            rc = true;
            break;
        case SCON_BOOL:
            rc = (p->data.flag == p1->data.flag);
            break;
        case SCON_BYTE:
            rc = (p->data.byte == p1->data.byte);
            break;
        case SCON_SIZE:
            rc = (p->data.size == p1->data.size);
            break;
        case SCON_INT:
            rc = (p->data.integer == p1->data.integer);
            break;
        case SCON_INT8:
            rc = (p->data.int8 == p1->data.int8);
            break;
        case SCON_INT16:
            rc = (p->data.int16 == p1->data.int16);
            break;
        case SCON_INT32:
            rc = (p->data.int32 == p1->data.int32);
            break;
        case SCON_INT64:
            rc = (p->data.int64 == p1->data.int64);
            break;
        case SCON_UINT:
            rc = (p->data.uint == p1->data.uint);
            break;
        case SCON_UINT8:
            rc = (p->data.uint8 == p1->data.int8);
            break;
        case SCON_UINT16:
            rc = (p->data.uint16 == p1->data.uint16);
            break;
        case SCON_UINT32:
            rc = (p->data.uint32 == p1->data.uint32);
            break;
        case SCON_UINT64:
            rc = (p->data.uint64 == p1->data.uint64);
            break;
        case SCON_STRING:
            rc = strcmp(p->data.string, p1->data.string);
            break;
        case SCON_STATUS:
            rc = (p->data.status == p1->data.status);
            break;
        default:
            scon_output(0, "COMPARE-SCON-VALUE: UNSUPPORTED TYPE %d", (int)p->type);
    }
    return rc;
}

/* COPY FUNCTIONS FOR GENERIC SCON TYPES - we
 * are not allocating memory and so we cannot
 * use the regular copy functions */
SCON_EXPORT scon_status_t scon_value_xfer(scon_value_t *p, scon_value_t *src)
{
    size_t n;
    scon_status_t rc;
    char **prarray, **strarray;
    scon_value_t *pv, *sv;
    scon_info_t *p1, *s1;
    scon_buffer_t *pb, *sb;
    scon_byte_object_t *pbo, *sbo;
   // scon_query_t *pq, *sq;

    /* copy the right field */
    p->type = src->type;
    switch (src->type) {
    case SCON_UNDEF:
    break;
    case SCON_BOOL:
        p->data.flag = src->data.flag;
        break;
    case SCON_BYTE:
        p->data.byte = src->data.byte;
        break;
    case SCON_STRING:
        if (NULL != src->data.string) {
            p->data.string = strdup(src->data.string);
        } else {
            p->data.string = NULL;
        }
        break;
    case SCON_SIZE:
        p->data.size = src->data.size;
        break;
    case SCON_PID:
        p->data.pid = src->data.pid;
        break;
    case SCON_INT:
        /* to avoid alignment issues */
        memcpy(&p->data.integer, &src->data.integer, sizeof(int));
        break;
    case SCON_INT8:
        p->data.int8 = src->data.int8;
        break;
    case SCON_INT16:
        /* to avoid alignment issues */
        memcpy(&p->data.int16, &src->data.int16, 2);
        break;
    case SCON_INT32:
        /* to avoid alignment issues */
        memcpy(&p->data.int32, &src->data.int32, 4);
        break;
    case SCON_INT64:
        /* to avoid alignment issues */
        memcpy(&p->data.int64, &src->data.int64, 8);
        break;
    case SCON_UINT:
        /* to avoid alignment issues */
        memcpy(&p->data.uint, &src->data.uint, sizeof(unsigned int));
        break;
    case SCON_UINT8:
        p->data.uint8 = src->data.uint8;
        break;
    case SCON_UINT16:
        /* to avoid alignment issues */
        memcpy(&p->data.uint16, &src->data.uint16, 2);
        break;
    case SCON_UINT32:
        /* to avoid alignment issues */
        memcpy(&p->data.uint32, &src->data.uint32, 4);
        break;
    case SCON_UINT64:
        /* to avoid alignment issues */
        memcpy(&p->data.uint64, &src->data.uint64, 8);
        break;
    case SCON_FLOAT:
        p->data.fval = src->data.fval;
        break;
    case SCON_DOUBLE:
        p->data.dval = src->data.dval;
        break;
    case SCON_TIMEVAL:
        memcpy(&p->data.tv, &src->data.tv, sizeof(struct timeval));
        break;
    case SCON_TIME:
        memcpy(&p->data.time, &src->data.time, sizeof(time_t));
        break;
    case SCON_STATUS:
        memcpy(&p->data.status, &src->data.status, sizeof(scon_status_t));
        break;
    case SCON_PROC:
        memcpy(&p->data.proc, &src->data.proc, sizeof(scon_proc_t));
        break;
    /*case SCON_PROC_RANK:
        memcpy(&p->data.proc, &src->data.rank, sizeof(scon_rank_t));
        break;*/
    case SCON_BYTE_OBJECT:
        memset(&p->data.bo, 0, sizeof(scon_byte_object_t));
        if (NULL != src->data.bo.bytes && 0 < src->data.bo.size) {
            p->data.bo.bytes = malloc(src->data.bo.size);
            memcpy(p->data.bo.bytes, src->data.bo.bytes, src->data.bo.size);
            p->data.bo.size = src->data.bo.size;
        } else {
            p->data.bo.bytes = NULL;
            p->data.bo.size = 0;
        }
        break;
    case SCON_DATA_RANGE:
        memcpy(&p->data.range, &src->data.range, sizeof(scon_member_range_t));
        break;
    case SCON_DATA_ARRAY:
        p->data.darray = (scon_data_array_t*)calloc(1, sizeof(scon_data_array_t));
        p->data.darray->type = src->data.darray->type;
        p->data.darray->size = src->data.darray->size;
        if (0 == p->data.darray->size || NULL == src->data.darray->array) {
            p->data.darray->array = NULL;
            p->data.darray->size = 0;
            break;
        }
        /* allocate space and do the copy */
        switch (src->type) {
            case SCON_UINT8:
            case SCON_INT8:
            case SCON_BYTE:
                p->data.darray->array = (char*)malloc(src->data.darray->size);
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size);
                break;
            case SCON_UINT16:
            case SCON_INT16:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(uint16_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(uint16_t));
                break;
            case SCON_UINT32:
            case SCON_INT32:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(uint32_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(uint32_t));
                break;
            case SCON_UINT64:
            case SCON_INT64:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(uint64_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(uint64_t));
                break;
            case SCON_BOOL:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(bool));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(bool));
                break;
            case SCON_SIZE:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(size_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(size_t));
                break;
            case SCON_PID:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(pid_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(pid_t));
                break;
            case SCON_STRING:
                p->data.darray->array = (char**)malloc(src->data.darray->size * sizeof(char*));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                prarray = (char**)p->data.darray->array;
                strarray = (char**)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                    if (NULL != strarray[n]) {
                        prarray[n] = strdup(strarray[n]);
                    }
                }
                break;
            case SCON_INT:
            case SCON_UINT:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(int));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(int));
                break;
            case SCON_FLOAT:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(float));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(float));
                break;
            case SCON_DOUBLE:
                p->data.darray->array = (char*)malloc(src->data.darray->size * sizeof(double));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(double));
                break;
            case SCON_TIMEVAL:
                p->data.darray->array = (struct timeval*)malloc(src->data.darray->size * sizeof(struct timeval));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(struct timeval));
                break;
            case SCON_TIME:
                p->data.darray->array = (time_t*)malloc(src->data.darray->size * sizeof(time_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(time_t));
                break;
            case SCON_STATUS:
                p->data.darray->array = (scon_status_t*)malloc(src->data.darray->size * sizeof(scon_status_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(scon_status_t));
                break;
            case SCON_VALUE:
                SCON_VALUE_CREATE(p->data.darray->array, src->data.darray->size);
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                pv = (scon_value_t*)p->data.darray->array;
                sv = (scon_value_t*)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                    if (SCON_SUCCESS != (rc = scon_value_xfer(&pv[n], &sv[n]))) {
                        SCON_VALUE_FREE(pv, src->data.darray->size);
                        return rc;
                    }
                }
                break;
            case SCON_PROC:
                SCON_PROC_CREATE(p->data.darray->array, src->data.darray->size);
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(scon_proc_t));
                break;
            case SCON_INFO:
                SCON_INFO_CREATE(p->data.darray->array, src->data.darray->size);
                p1 = (scon_info_t*)p->data.darray->array;
                s1 = (scon_info_t*)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                    SCON_INFO_LOAD(&p1[n], s1[n].key, &s1[n].value.data.flag, s1[n].value.type);
                }
                break;
            case SCON_BUFFER:
                p->data.darray->array = (scon_buffer_t*)malloc(src->data.darray->size * sizeof(scon_buffer_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                pb = (scon_buffer_t*)p->data.darray->array;
                sb = (scon_buffer_t*)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                   // SCON_CONSTRUCT(&pb[n], scon_buffer_t);
                    scon_bfrop.copy_payload(&pb[n], &sb[n]);
                }
                break;
            case SCON_BYTE_OBJECT:
                p->data.darray->array = (scon_byte_object_t*)malloc(src->data.darray->size * sizeof(scon_byte_object_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                pbo = (scon_byte_object_t*)p->data.darray->array;
                sbo = (scon_byte_object_t*)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                    if (NULL != sbo[n].bytes && 0 < sbo[n].size) {
                        pbo[n].size = sbo[n].size;
                        pbo[n].bytes = (char*)malloc(pbo[n].size);
                        memcpy(pbo[n].bytes, sbo[n].bytes, pbo[n].size);
                    } else {
                        pbo[n].bytes = NULL;
                        pbo[n].size = 0;
                    }
                }
                break;
                // Lets revisit if key val data type applies to SCON
          /*  case SCON_KVAL:
                p->data.darray->array = (scon_kval_t*)calloc(src->data.darray->size , sizeof(scon_kval_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                pk = (scon_kval_t*)p->data.darray->array;
                sk = (scon_kval_t*)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                    if (NULL != sk[n].key) {
                        pk[n].key = strdup(sk[n].key);
                    }
                    if (NULL != sk[n].value) {
                        SCON_VALUE_CREATE(pk[n].value, 1);
                        if (NULL == pk[n].value) {
                            free(p->data.darray->array);
                            return SCON_ERR_NOMEM;
                        }
                        if (SCON_SUCCESS != (rc = scon_value_xfer(pk[n].value, sk[n].value))) {
                            return rc;
                        }
                    }
                }
                break;*/
            case SCON_POINTER:
                p->data.darray->array = (char**)malloc(src->data.darray->size * sizeof(char*));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                prarray = (char**)p->data.darray->array;
                strarray = (char**)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                    prarray[n] = strarray[n];
                }
                break;
            case SCON_DATA_RANGE:
                p->data.darray->array = (scon_member_range_t*)malloc(src->data.darray->size * sizeof(scon_member_range_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(scon_member_range_t));
                break;
            case SCON_INFO_DIRECTIVES:
                p->data.darray->array = (scon_info_directives_t*)malloc(src->data.darray->size * sizeof(scon_info_directives_t));
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                memcpy(p->data.darray->array, src->data.darray->array, src->data.darray->size * sizeof(scon_info_directives_t));
                break;
            /* revisit SCON proc_info  if it needed uncomment this */
           /* case SCON_PROC_INFO:
                SCON_PROC_INFO_CREATE(p->data.darray->array, src->data.darray->size);
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                pi = (scon_proc_info_t*)p->data.darray->array;
                si = (scon_proc_info_t*)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                    memcpy(&pi[n].proc, &si[n].proc, sizeof(scon_proc_t));
                    if (NULL != si[n].hostname) {
                        pi[n].hostname = strdup(si[n].hostname);
                    } else {
                        pi[n].hostname = NULL;
                    }
                    if (NULL != si[n].executable_name) {
                        pi[n].executable_name = strdup(si[n].executable_name);
                    } else {
                        pi[n].executable_name = NULL;
                    }
                    pi[n].pid = si[n].pid;
                    pi[n].exit_code = si[n].exit_code;
                    pi[n].state = si[n].state;
                }
                break;*/
            case SCON_DATA_ARRAY:
                return SCON_ERR_NOT_SUPPORTED;  // don't support iterative arrays
          /*  case SCON_QUERY:
               SCON_QUERY_CREATE(p->data.darray->array, src->data.darray->size);
                if (NULL == p->data.darray->array) {
                    return SCON_ERR_NOMEM;
                }
                pq = (scon_query_t*)p->data.darray->array;
                sq = (scon_query_t*)src->data.darray->array;
                for (n=0; n < src->data.darray->size; n++) {
                    if (NULL != sq[n].keys) {
                        pq[n].keys = scon_argv_copy(sq[n].keys);
                    }
                    if (NULL != sq[n].qualifiers && 0 < sq[n].nqual) {
                        SCON_INFO_CREATE(pq[n].qualifiers, sq[n].nqual);
                        if (NULL == pq[n].qualifiers) {
                            SCON_QUERY_FREE(pq, src->data.darray->size);
                            return SCON_ERR_NOMEM;
                        }
                        for (m=0; m < sq[n].nqual; m++) {
                            SCON_INFO_XFER(&pq[n].qualifiers[m], &sq[n].qualifiers[m]);
                        }
                        pq[n].nqual = sq[n].nqual;
                    } else {
                        pq[n].qualifiers = NULL;
                        pq[n].nqual = 0;
                    }
                }
                return SCON_ERR_NOT_SUPPORTED;
                break;*/
            default:
                return SCON_ERR_UNKNOWN_DATA_TYPE;
        }
        break;
    case SCON_POINTER:
        memcpy(&p->data.ptr, &src->data.ptr, sizeof(void*));
        break;
    default:
        scon_output(0, "COPY-SCON-VALUE: UNSUPPORTED TYPE %d", (int)src->type);
        return SCON_ERROR;
    }
    return SCON_SUCCESS;
}

/* SCON_VALUE */
scon_status_t scon_bfrop_copy_value(scon_value_t **dest, scon_value_t *src,
                                    scon_data_type_t type)
{
    scon_value_t *p;

    /* create the new object */
    *dest = (scon_value_t*)malloc(sizeof(scon_value_t));
    if (NULL == *dest) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    p = *dest;

    /* copy the type */
    p->type = src->type;
    /* copy the data */
    return scon_value_xfer(p, src);
}

scon_status_t scon_bfrop_copy_info(scon_info_t **dest, scon_info_t *src,
                                   scon_data_type_t type)
{
    *dest = (scon_info_t*)malloc(sizeof(scon_info_t));
    (void)strncpy((*dest)->key, src->key, SCON_MAX_KEYLEN);
    (*dest)->flags = src->flags;
    return scon_value_xfer(&(*dest)->value, &src->value);
}

scon_status_t scon_bfrop_copy_buf(scon_buffer_t **dest, scon_buffer_t *src,
                                  scon_data_type_t type)
{
    //*dest = SCON_NEW(scon_buffer_t);
    *dest = (scon_buffer_t*) malloc(sizeof(scon_buffer_t));
    scon_bfrop.copy_payload(*dest, src);
    return SCON_SUCCESS;
}

//revisit key val
#if 0
scon_status_t scon_bfrop_copy_kval(scon_kval_t **dest, scon_kval_t *src,
                                   scon_data_type_t type)
{
    scon_kval_t *p;

    /* create the new object */
   *dest = SCON_NEW(scon_kval_t);
    if (NULL == *dest) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    p = *dest;

    /* copy the type */
    p->value->type = src->value->type;
    /* copy the data */
    return scon_value_xfer(p->value, src->value);
}
#endif

scon_status_t scon_bfrop_copy_proc(scon_proc_t **dest, scon_proc_t *src,
                                   scon_data_type_t type)
{
    *dest = (scon_proc_t*)malloc(sizeof(scon_proc_t));
    if (NULL == *dest) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    (void)strncpy((*dest)->job_name, src->job_name, SCON_MAX_NSLEN);
    (*dest)->rank = src->rank;
    return SCON_SUCCESS;
}



scon_status_t scon_bfrop_copy_bo(scon_byte_object_t **dest, scon_byte_object_t *src,
                                 scon_data_type_t type)
{
    *dest = (scon_byte_object_t*)malloc(sizeof(scon_byte_object_t));
    if (NULL == *dest) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    (*dest)->bytes = (char*)malloc(src->size);
    memcpy((*dest)->bytes, src->bytes, src->size);
    (*dest)->size = src->size;
    return SCON_SUCCESS;
}
/* revisit later : comment out for now */
/*scon_status_t scon_bfrop_copy_pdata(scon_pdata_t **dest, scon_pdata_t *src,
                                    scon_data_type_t type)
{
    *dest = (scon_pdata_t*)malloc(sizeof(scon_pdata_t));
    (void)strncpy((*dest)->proc.nspace, src->proc.nspace, SCON_MAX_NSLEN);
    (*dest)->proc.rank = src->proc.rank;
    (void)strncpy((*dest)->key, src->key, SCON_MAX_KEYLEN);
    return scon_value_xfer(&(*dest)->value, &src->value);
}*/

/* the scon_data_array_t is a little different in that it
 * is an array of values, and so we cannot just copy one
 * value at a time. So handle all value types here */
scon_status_t scon_bfrop_copy_darray(scon_data_array_t **dest,
                                     scon_data_array_t *src,
                                     scon_data_type_t type)
{
    scon_data_array_t *p;
    size_t n;
    scon_status_t rc;
    char **prarray, **strarray;
    scon_value_t *pv, *sv;
    scon_info_t *p1, *s1;
    scon_buffer_t *pb, *sb;
    scon_byte_object_t *pbo, *sbo;

    p = (scon_data_array_t*)calloc(1, sizeof(scon_data_array_t));
    if (NULL == p) {
        return SCON_ERR_NOMEM;
    }
    p->type = src->type;
    p->size = src->size;
    /* process based on type of array element */
    switch (src->type) {
        p->type = src->type;
        p->size = src->size;
        if (0 == p->size || NULL == src->array) {
            p->array = NULL;
            p->size = 0;
            break;
        }
        case SCON_UINT8:
        case SCON_INT8:
        case SCON_BYTE:
            p->array = (char*)malloc(src->size);
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size);
            break;
        case SCON_UINT16:
        case SCON_INT16:
            p->array = (char*)malloc(src->size * sizeof(uint16_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(uint16_t));
            break;
        case SCON_UINT32:
        case SCON_INT32:
            p->array = (char*)malloc(src->size * sizeof(uint32_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(uint32_t));
            break;
        case SCON_UINT64:
        case SCON_INT64:
            p->array = (char*)malloc(src->size * sizeof(uint64_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(uint64_t));
            break;
        case SCON_BOOL:
            p->array = (char*)malloc(src->size * sizeof(bool));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(bool));
            break;
        case SCON_SIZE:
            p->array = (char*)malloc(src->size * sizeof(size_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(size_t));
            break;
        case SCON_PID:
            p->array = (char*)malloc(src->size * sizeof(pid_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(pid_t));
            break;
        case SCON_STRING:
            p->array = (char**)malloc(src->size * sizeof(char*));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            prarray = (char**)p->array;
            strarray = (char**)src->array;
            for (n=0; n < src->size; n++) {
                if (NULL != strarray[n]) {
                    prarray[n] = strdup(strarray[n]);
                }
            }
            break;
        case SCON_INT:
        case SCON_UINT:
            p->array = (char*)malloc(src->size * sizeof(int));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(int));
            break;
        case SCON_FLOAT:
            p->array = (char*)malloc(src->size * sizeof(float));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(float));
            break;
        case SCON_DOUBLE:
            p->array = (char*)malloc(src->size * sizeof(double));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(double));
            break;
        case SCON_TIMEVAL:
            p->array = (struct timeval*)malloc(src->size * sizeof(struct timeval));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(struct timeval));
            break;
        case SCON_TIME:
            p->array = (time_t*)malloc(src->size * sizeof(time_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(time_t));
            break;
        case SCON_STATUS:
            p->array = (scon_status_t*)malloc(src->size * sizeof(scon_status_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(scon_status_t));
            break;
        case SCON_VALUE:
            SCON_VALUE_CREATE(p->array, src->size);
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            pv = (scon_value_t*)p->array;
            sv = (scon_value_t*)src->array;
            for (n=0; n < src->size; n++) {
                if (SCON_SUCCESS != (rc = scon_value_xfer(&pv[n], &sv[n]))) {
                    SCON_VALUE_FREE(pv, src->size);
                    free(p);
                    return rc;
                }
            }
            break;
        case SCON_PROC:
            SCON_PROC_CREATE(p->array, src->size);
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(scon_proc_t));
            break;
        case SCON_PROC_RANK:
            p->array = (char*)malloc(src->size * sizeof(scon_rank_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(scon_proc_t));
            break;
        case SCON_INFO:
            SCON_INFO_CREATE(p->array, src->size);
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            p1 = (scon_info_t*)p->array;
            s1 = (scon_info_t*)src->array;
            for (n=0; n < src->size; n++) {
                SCON_INFO_LOAD(&p1[n], s1[n].key, &s1[n].value.data.flag, s1[n].value.type);
            }
            break;
        case SCON_BUFFER:
            p->array = (scon_buffer_t*)malloc(src->size * sizeof(scon_buffer_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            pb = (scon_buffer_t*)p->array;
            sb = (scon_buffer_t*)src->array;
            for (n=0; n < src->size; n++) {
                //SCON_CONSTRUCT(&pb[n], scon_buffer_t);
                scon_bfrop.copy_payload(&pb[n], &sb[n]);
            }
            break;
        case SCON_BYTE_OBJECT:
            p->array = (scon_byte_object_t*)malloc(src->size * sizeof(scon_byte_object_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            pbo = (scon_byte_object_t*)p->array;
            sbo = (scon_byte_object_t*)src->array;
            for (n=0; n < src->size; n++) {
                if (NULL != sbo[n].bytes && 0 < sbo[n].size) {
                    pbo[n].size = sbo[n].size;
                    pbo[n].bytes = (char*)malloc(pbo[n].size);
                    memcpy(pbo[n].bytes, sbo[n].bytes, pbo[n].size);
                } else {
                    pbo[n].bytes = NULL;
                    pbo[n].size = 0;
                }
            }
            break;
       /* case SCON_KVAL:
            p->array = (scon_kval_t*)calloc(src->size , sizeof(scon_kval_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            pk = (scon_kval_t*)p->array;
            sk = (scon_kval_t*)src->array;
            for (n=0; n < src->size; n++) {
                if (NULL != sk[n].key) {
                    pk[n].key = strdup(sk[n].key);
                }
                if (NULL != sk[n].value) {
                    SCON_VALUE_CREATE(pk[n].value, 1);
                    if (NULL == pk[n].value) {
                        SCON_VALUE_FREE(pk[n].value, 1);
                        free(p);
                        return SCON_ERR_NOMEM;
                    }
                    if (SCON_SUCCESS != (rc = scon_value_xfer(pk[n].value, sk[n].value))) {
                        SCON_VALUE_FREE(pk[n].value, 1);
                        free(p);
                        return rc;
                    }
                }
            }
            break;*/
        case SCON_POINTER:
            p->array = (char**)malloc(src->size * sizeof(char*));
            prarray = (char**)p->array;
            strarray = (char**)src->array;
            for (n=0; n < src->size; n++) {
                prarray[n] = strarray[n];
            }
            break;
        case SCON_DATA_RANGE:
            p->array = (scon_member_range_t*)malloc(src->size * sizeof(scon_member_range_t));
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            memcpy(p->array, src->array, src->size * sizeof(scon_member_range_t));
            break;
        case SCON_DATA_ARRAY:
            free(p);
            return SCON_ERR_NOT_SUPPORTED;  // don't support iterative arrays
      /*  case SCON_QUERY:
            SCON_QUERY_CREATE(p->array, src->size);
            if (NULL == p->array) {
                free(p);
                return SCON_ERR_NOMEM;
            }
            pq = (scon_query_t*)p->array;
            sq = (scon_query_t*)src->array;
            for (n=0; n < src->size; n++) {
                if (NULL != sq[n].keys) {
                    pq[n].keys = scon_argv_copy(sq[n].keys);
                }
                if (NULL != sq[n].qualifiers && 0 < sq[n].nqual) {
                    SCON_INFO_CREATE(pq[n].qualifiers, sq[n].nqual);
                    if (NULL == pq[n].qualifiers) {
                        SCON_INFO_FREE(pq[n].qualifiers, sq[n].nqual);
                        free(p);
                        return SCON_ERR_NOMEM;
                    }
                    for (m=0; m < sq[n].nqual; m++) {
                        SCON_INFO_XFER(&pq[n].qualifiers[m], &sq[n].qualifiers[m]);
                    }
                    pq[n].nqual = sq[n].nqual;
                } else {
                    pq[n].qualifiers = NULL;
                    pq[n].nqual = 0;
                }
            }
            break;*/
        default:
            free(p);
            return SCON_ERR_UNKNOWN_DATA_TYPE;
    }

    (*dest) = p;
    return SCON_SUCCESS;
}

int scon_bfrop_copy_coll_sig(scon_collectives_signature_t **dest,
                             scon_collectives_signature_t *src, scon_data_type_t type)
{
    *dest = SCON_NEW(scon_collectives_signature_t);
    if (NULL == *dest) {
        SCON_ERROR_LOG(SCON_ERR_OUT_OF_RESOURCE);
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    (*dest)->scon_handle = src->scon_handle;
    (*dest)->nprocs = src->nprocs;
    (*dest)->procs = (scon_proc_t*)malloc(src->nprocs * sizeof(scon_proc_t));
    (*dest)->seq_num = src->seq_num;
    if (NULL == (*dest)->procs) {
        SCON_ERROR_LOG(SCON_ERR_OUT_OF_RESOURCE);
        SCON_RELEASE(*dest);
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    memcpy((*dest)->procs, src->procs, src->nprocs * sizeof(scon_proc_t));
    return SCON_SUCCESS;
}

/*scon_status_t scon_bfrop_copy_query(scon_query_t **dest,
                                    scon_query_t *src,
                                    scon_data_type_t type)
{
    scon_status_t rc;

    *dest = (scon_query_t*)malloc(sizeof(scon_query_t));
    if (NULL != src->keys) {
        (*dest)->keys = scon_argv_copy(src->keys);
    }
    (*dest)->nqual = src->nqual;
    if (NULL != src->qualifiers) {
        if (SCON_SUCCESS != (rc = scon_bfrop_copy_info(&((*dest)->qualifiers), src->qualifiers, SCON_INFO))) {
            free(*dest);
            return rc;
        }
    }
    return SCON_SUCCESS;
}*/

/**** DEPRECATED ****/
scon_status_t scon_bfrop_copy_array(scon_info_array_t **dest,
                                    scon_info_array_t *src,
                                    scon_data_type_t type)
{
    scon_info_t *d1, *s1;

    *dest = (scon_info_array_t*)malloc(sizeof(scon_info_array_t));
    (*dest)->size = src->size;
    (*dest)->array = (struct scon_info_t*)malloc(src->size * sizeof(scon_info_t));
    d1 = (scon_info_t*)(*dest)->array;
    s1 = (scon_info_t*)src->array;
    memcpy(d1, s1, src->size * sizeof(scon_info_t));
    return SCON_SUCCESS;
}
/*******************/
