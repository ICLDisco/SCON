/* -*- C -*-
 *
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
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */
#ifndef SCON_BFROP_INTERNAL_H_
#define SCON_BFROP_INTERNAL_H_

#include <src/include/scon_config.h>


#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* for struct timeval */
#endif

#include "src/class/scon_pointer_array.h"

#include "buffer_ops.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif
BEGIN_C_DECLS

/*
 * The default starting chunk size
 */
#define SCON_BFROP_DEFAULT_INITIAL_SIZE  128
/*
 * The default threshold size when we switch from doubling the
 * buffer size to addatively increasing it
 */
#define SCON_BFROP_DEFAULT_THRESHOLD_SIZE 1024

/*
 * Internal type corresponding to size_t.  Do not use this in
 * interface calls - use SCON_SIZE instead.
 */
#if SIZEOF_SIZE_T == 1
#define BFROP_TYPE_SIZE_T SCON_UINT8
#elif SIZEOF_SIZE_T == 2
#define BFROP_TYPE_SIZE_T SCON_UINT16
#elif SIZEOF_SIZE_T == 4
#define BFROP_TYPE_SIZE_T SCON_UINT32
#elif SIZEOF_SIZE_T == 8
#define BFROP_TYPE_SIZE_T SCON_UINT64
#else
#error Unsupported size_t size!
#endif

/*
 * Internal type corresponding to bool.  Do not use this in interface
 * calls - use SCON_BOOL instead.
 */
#if SIZEOF__BOOL == 1
#define BFROP_TYPE_BOOL SCON_UINT8
#elif SIZEOF__BOOL == 2
#define BFROP_TYPE_BOOL SCON_UINT16
#elif SIZEOF__BOOL == 4
#define BFROP_TYPE_BOOL SCON_UINT32
#elif SIZEOF__BOOL == 8
#define BFROP_TYPE_BOOL SCON_UINT64
#else
#error Unsupported bool size!
#endif

/*
 * Internal type corresponding to int and unsigned int.  Do not use
 * this in interface calls - use SCON_INT / SCON_UINT instead.
 */
#if SIZEOF_INT == 1
#define BFROP_TYPE_INT SCON_INT8
#define BFROP_TYPE_UINT SCON_UINT8
#elif SIZEOF_INT == 2
#define BFROP_TYPE_INT SCON_INT16
#define BFROP_TYPE_UINT SCON_UINT16
#elif SIZEOF_INT == 4
#define BFROP_TYPE_INT SCON_INT32
#define BFROP_TYPE_UINT SCON_UINT32
#elif SIZEOF_INT == 8
#define BFROP_TYPE_INT SCON_INT64
#define BFROP_TYPE_UINT SCON_UINT64
#else
#error Unsupported int size!
#endif

/*
 * Internal type corresponding to pid_t.  Do not use this in interface
 * calls - use SCON_PID instead.
 */
#if SIZEOF_PID_T == 1
#define BFROP_TYPE_PID_T SCON_UINT8
#elif SIZEOF_PID_T == 2
#define BFROP_TYPE_PID_T SCON_UINT16
#elif SIZEOF_PID_T == 4
#define BFROP_TYPE_PID_T SCON_UINT32
#elif SIZEOF_PID_T == 8
#define BFROP_TYPE_PID_T SCON_UINT64
#else
#error Unsupported pid_t size!
#endif

/* Unpack generic size macros */
#define UNPACK_SIZE_MISMATCH(unpack_type, remote_type, ret)                 \
 do {                                                                    \
    switch(remote_type) {                                               \
        case SCON_UINT8:                                                    \
        UNPACK_SIZE_MISMATCH_FOUND(unpack_type, uint8_t, remote_type);  \
        break;                                                          \
        case SCON_INT8:                                                     \
        UNPACK_SIZE_MISMATCH_FOUND(unpack_type, int8_t, remote_type);   \
        break;                                                          \
        case SCON_UINT16:                                                   \
        UNPACK_SIZE_MISMATCH_FOUND(unpack_type, uint16_t, remote_type); \
        break;                                                          \
        case SCON_INT16:                                                    \
        UNPACK_SIZE_MISMATCH_FOUND(unpack_type, int16_t, remote_type);  \
        break;                                                          \
        case SCON_UINT32:                                                   \
        UNPACK_SIZE_MISMATCH_FOUND(unpack_type, uint32_t, remote_type); \
        break;                                                          \
        case SCON_INT32:                                                    \
        UNPACK_SIZE_MISMATCH_FOUND(unpack_type, int32_t, remote_type);  \
        break;                                                          \
        case SCON_UINT64:                                                   \
        UNPACK_SIZE_MISMATCH_FOUND(unpack_type, uint64_t, remote_type); \
        break;                                                          \
        case SCON_INT64:                                                    \
        UNPACK_SIZE_MISMATCH_FOUND(unpack_type, int64_t, remote_type);  \
        break;                                                          \
        default:                                                            \
        ret = SCON_ERR_NOT_FOUND;                                       \
    }                                                                   \
} while (0)

/* NOTE: do not need to deal with endianness here, as the unpacking of
   the underling sender-side type will do that for us.  Repeat: the
   data in tmpbuf[] is already in host byte order. */
#define UNPACK_SIZE_MISMATCH_FOUND(unpack_type, tmptype, tmpbfroptype)        \
   do {                                                                    \
    int32_t i;                                                          \
    tmptype *tmpbuf = (tmptype*)malloc(sizeof(tmptype) * (*num_vals));  \
    ret = scon_bfrop_unpack_buffer(buffer, tmpbuf, num_vals, tmpbfroptype); \
    for (i = 0 ; i < *num_vals ; ++i) {                                 \
        ((unpack_type*) dest)[i] = (unpack_type)(tmpbuf[i]);            \
    }                                                                   \
    free(tmpbuf);                                                       \
} while (0)


/**
 * Internal struct used for holding registered bfrop functions
 */
 typedef struct {
    scon_object_t super;
    /* type identifier */
    scon_data_type_t odti_type;
    /** Debugging string name */
    char *odti_name;
    /** Pack function */
    scon_bfrop_pack_fn_t odti_pack_fn;
    /** Unpack function */
    scon_bfrop_unpack_fn_t odti_unpack_fn;
    /** copy function */
    scon_bfrop_copy_fn_t odti_copy_fn;
    /** print function */
    scon_bfrop_print_fn_t odti_print_fn;
} scon_bfrop_type_info_t;
SCON_CLASS_DECLARATION(scon_bfrop_type_info_t);

/*
 * globals needed within bfrop
 */
 extern bool scon_bfrop_initialized;
 extern size_t scon_bfrop_initial_size;
 extern size_t scon_bfrop_threshold_size;
 extern scon_pointer_array_t scon_bfrop_types;
 extern scon_data_type_t scon_bfrop_num_reg_types;

/* macro for registering data types */
#define SCON_REGISTER_TYPE(n, t, p, u, c, pr)                           \
 do {                                                                \
    scon_bfrop_type_info_t *_info;                                  \
    _info = SCON_NEW(scon_bfrop_type_info_t);                        \
    _info->odti_name = strdup((n));                                 \
    _info->odti_type = (t);                                         \
    _info->odti_pack_fn = (scon_bfrop_pack_fn_t)(p);                \
    _info->odti_unpack_fn = (scon_bfrop_unpack_fn_t)(u);            \
    _info->odti_copy_fn = (scon_bfrop_copy_fn_t)(c) ;               \
    _info->odti_print_fn = (scon_bfrop_print_fn_t)(pr) ;            \
    scon_pointer_array_set_item(&scon_bfrop_types, (t), _info);     \
    ++scon_bfrop_num_reg_types;                                     \
} while (0)

/*
 * Implementations of API functions
 */

scon_status_t scon_bfrop_pack(scon_buffer_t *buffer, const void *src,
                              int32_t num_vals,
                              scon_data_type_t type);
scon_status_t scon_bfrop_unpack(scon_buffer_t *buffer, void *dest,
                                int32_t *max_num_vals,
                                scon_data_type_t type);

scon_status_t scon_bfrop_copy(void **dest, void *src, scon_data_type_t type);

scon_status_t scon_bfrop_print(char **output, char *prefix, void *src, scon_data_type_t type);

scon_status_t scon_bfrop_copy_payload(scon_buffer_t *dest, scon_buffer_t *src);

/*
 * Specialized functions
 */
scon_status_t scon_bfrop_pack_buffer(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type);

scon_status_t scon_bfrop_unpack_buffer(scon_buffer_t *buffer, void *dst,
                                       int32_t *num_vals, scon_data_type_t type);

/*
 * Internal pack functions
 */

scon_status_t scon_bfrop_pack_bool(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_byte(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_string(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_sizet(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_pid(scon_buffer_t *buffer, const void *src,
                                  int32_t num_vals, scon_data_type_t type);

scon_status_t scon_bfrop_pack_int(scon_buffer_t *buffer, const void *src,
                                  int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_int16(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_int32(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_datatype(scon_buffer_t *buffer, const void *src,
                                       int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_int64(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type);

scon_status_t scon_bfrop_pack_float(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_double(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_time(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_timeval(scon_buffer_t *buffer, const void *src,
                                      int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_time(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_status(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_value(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_proc(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_info(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_buf(scon_buffer_t *buffer, const void *src,
                                  int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_range(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_infodirs(scon_buffer_t *buffer, const void *src,
                                       int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_bo(scon_buffer_t *buffer, const void *src,
                                 int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_ptr(scon_buffer_t *buffer, const void *src,
                                  int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_darray(scon_buffer_t *buffer, const void *src,
                                     int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_query(scon_buffer_t *buffer, const void *src,
                                    int32_t num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_pack_rank(scon_buffer_t *buffer, const void *src,
                                   int32_t num_vals, scon_data_type_t type);
/*
 * Internal unpack functions
 */
 scon_status_t scon_bfrop_unpack_bool(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_byte(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_string(scon_buffer_t *buffer, void *dest,
                                        int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_sizet(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_pid(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type);

 scon_status_t scon_bfrop_unpack_int(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_int16(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_int32(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_datatype(scon_buffer_t *buffer, void *dest,
                                          int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_int64(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type);

 scon_status_t scon_bfrop_unpack_float(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_double(scon_buffer_t *buffer, void *dest,
                                        int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_timeval(scon_buffer_t *buffer, void *dest,
                                         int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_time(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_status(scon_buffer_t *buffer, void *dest,
                                        int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_value(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_proc(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_info(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_buf(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_range(scon_buffer_t *buffer, void *dest,
                                       int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_infodirs(scon_buffer_t *buffer, void *dest,
                                          int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_bo(scon_buffer_t *buffer, void *dest,
                                    int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_ptr(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type);
 scon_status_t scon_bfrop_unpack_darray(scon_buffer_t *buffer, void *dest,
                                        int32_t *num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_unpack_query(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type);
scon_status_t scon_bfrop_unpack_rank(scon_buffer_t *buffer, void *dest,
                                     int32_t *num_vals, scon_data_type_t type);
/**** DEPRECATED ****/
scon_status_t scon_bfrop_unpack_array(scon_buffer_t *buffer, void *dest,
                                      int32_t *num_vals, scon_data_type_t type);
/********************/

/*
 * Internal copy functions
 */

 scon_status_t scon_bfrop_std_copy(void **dest, void *src, scon_data_type_t type);

 scon_status_t scon_bfrop_copy_string(char **dest, char *src, scon_data_type_t type);

#if SCON_HAVE_HWLOC
 scon_status_t scon_bfrop_copy_topo(hwloc_topology_t *dest,
                          hwloc_topology_t src,
                          scon_data_type_t type);
#endif
scon_status_t scon_bfrop_copy_value(scon_value_t **dest, scon_value_t *src,
                                    scon_data_type_t type);
scon_status_t scon_bfrop_copy_proc(scon_proc_t **dest, scon_proc_t *src,
                                   scon_data_type_t type);
scon_status_t scon_bfrop_copy_info(scon_info_t **dest, scon_info_t *src,
                                   scon_data_type_t type);
scon_status_t scon_bfrop_copy_buf(scon_buffer_t **dest, scon_buffer_t *src,
                                  scon_data_type_t type);
/*scon_status_t scon_bfrop_copy_kval(scon_kval_t **dest, scon_kval_t *src,
                                   scon_data_type_t type);*/
scon_status_t scon_bfrop_copy_bo(scon_byte_object_t **dest, scon_byte_object_t *src,
                                 scon_data_type_t type);
scon_status_t scon_bfrop_copy_darray(scon_data_array_t **dest, scon_data_array_t *src,
                                     scon_data_type_t type);

/**** DEPRECATED ****/
scon_status_t scon_bfrop_copy_array(scon_info_array_t **dest,
                                    scon_info_array_t *src,
                                    scon_data_type_t type);

/********************/

/*
 * Internal print functions
 */
scon_status_t scon_bfrop_print_bool(char **output, char *prefix, bool *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_byte(char **output, char *prefix, uint8_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_string(char **output, char *prefix, char *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_size(char **output, char *prefix, size_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_pid(char **output, char *prefix, pid_t *src, scon_data_type_t type);

scon_status_t scon_bfrop_print_int(char **output, char *prefix, int *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_int8(char **output, char *prefix, int8_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_int16(char **output, char *prefix, int16_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_int32(char **output, char *prefix, int32_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_int64(char **output, char *prefix, int64_t *src, scon_data_type_t type);

scon_status_t scon_bfrop_print_uint(char **output, char *prefix, uint *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_uint8(char **output, char *prefix, uint8_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_uint16(char **output, char *prefix, uint16_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_uint32(char **output, char *prefix, uint32_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_uint64(char **output, char *prefix, uint64_t *src, scon_data_type_t type);

scon_status_t scon_bfrop_print_float(char **output, char *prefix, float *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_double(char **output, char *prefix, double *src, scon_data_type_t type);

scon_status_t scon_bfrop_print_timeval(char **output, char *prefix, struct timeval *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_time(char **output, char *prefix, time_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_status(char **output, char *prefix, scon_status_t *src, scon_data_type_t type);


scon_status_t scon_bfrop_print_value(char **output, char *prefix, scon_value_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_proc(char **output, char *prefix,
                                    scon_proc_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_info(char **output, char *prefix,
                                    scon_info_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_buf(char **output, char *prefix,
                                   scon_buffer_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_range(char **output, char *prefix,
                                     scon_member_range_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_infodirs(char **output, char *prefix,
                                        scon_info_directives_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_bo(char **output, char *prefix,
                                  scon_byte_object_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_ptr(char **output, char *prefix,
                                   void *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_darray(char **output, char *prefix,
                                      scon_data_array_t *src, scon_data_type_t type);
scon_status_t scon_bfrop_print_rank(char **output, char *prefix,
                                    scon_rank_t *src, scon_data_type_t type);

/*
 * Internal helper functions
 */

 char* scon_bfrop_buffer_extend(scon_buffer_t *bptr, size_t bytes_to_add);

 bool scon_bfrop_too_small(scon_buffer_t *buffer, size_t bytes_reqd);

 scon_bfrop_type_info_t* scon_bfrop_find_type(scon_data_type_t type);

 scon_status_t scon_bfrop_store_data_type(scon_buffer_t *buffer, scon_data_type_t type);

 scon_status_t scon_bfrop_get_data_type(scon_buffer_t *buffer, scon_data_type_t *type);

 END_C_DECLS

#endif
