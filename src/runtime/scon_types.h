/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * scon_types.h -  SCON type definitions
 * $HEADER$
 */
#ifndef SCON_TYPES_H
#define SCON_TYPES_H

#include <stdint.h>
#include <string.h>
/* this opal include can be removed when scon defines its own buffer operations */
#include "opal/dss/dss_types.h"

BEGIN_C_DECLS
#define SCON_MAX_JOBLEN  255
#define SCON_MAX_KEYLEN  511

/****  SCON_HANDLE   ****/
typedef int32_t scon_handle_t;

/****  SCON BUFFER  *****/
/* define it as opal_buffer for now will put our own definition in future */
typedef opal_buffer_t scon_buffer_t;
/****   SCON MSG TAG   *****/
/*  (1 - 100 are reserved, values match that of RML) */
typedef uint32_t scon_msg_tag_t;

/****   SCON PROC OBJECT   ****/
typedef struct {
    char job_name[SCON_MAX_JOBLEN+1];
    unsigned int rank;
} scon_proc_t;

/* enumerate different data types and corresponding arrays to define
 * scon_value_t. This has to match opal data types as we are using the
 * opal buffer and dss under the covers.
 */
typedef uint8_t  scon_data_type_t;  /** data type indicators */
/****    SCON DATA TYPES    ****/
#define    SCON_UNDEF               (scon_data_type_t)    0 /**< type hasn't been defined yet */
#define    SCON_BYTE                (scon_data_type_t)    1 /**< a byte of data */
#define    SCON_BOOL                (scon_data_type_t)    2 /**< boolean */
#define    SCON_STRING              (scon_data_type_t)    3 /**< a NULL terminated string */
#define    SCON_SIZE                (scon_data_type_t)    4 /**< the generic size_t */
#define    SCON_PID                 (scon_data_type_t)    5 /**< process pid */
/* all the integer flavors */
#define    SCON_INT                 (scon_data_type_t)    6 /**< generic integer */
#define    SCON_INT8                (scon_data_type_t)    7 /**< an 8-bit integer */
#define    SCON_INT16               (scon_data_type_t)    8 /**< a 16-bit integer */
#define    SCON_INT32               (scon_data_type_t)    9 /**< a 32-bit integer */
#define    SCON_INT64               (scon_data_type_t)   10 /**< a 64-bit integer */
/* all the unsigned integer flavors */
#define    SCON_UINT                (scon_data_type_t)   11 /**< generic unsigned integer */
#define    SCON_UINT8               (scon_data_type_t)   12 /**< an 8-bit unsigned integer */
#define    SCON_UINT16              (scon_data_type_t)   13 /**< a 16-bit unsigned integer */
#define    SCON_UINT32              (scon_data_type_t)   14 /**< a 32-bit unsigned integer */
#define    SCON_UINT64              (scon_data_type_t)   15 /**< a 64-bit unsigned integer */
/* floating point types */
#define    SCON_FLOAT               (scon_data_type_t)   16
#define    SCON_DOUBLE              (scon_data_type_t)   17
/* system types */
#define    SCON_TIMEVAL             (scon_data_type_t)   18
#define    SCON_TIME                (scon_data_type_t)   19
/* SCON types */
#define    SCON_BYTE_OBJECT         (scon_data_type_t)   20 /**< byte object structure */
#define    SCON_DATA_TYPE           (scon_data_type_t)   21 /**< data type */
#define    SCON_NULL                (scon_data_type_t)   22 /**< don't interpret data type */
#define    SCON_PSTAT               (scon_data_type_t)   23 /**< process statistics */
#define    SCON_NODE_STAT           (scon_data_type_t)   24 /**< node statistics */
#define    SCON_HWLOC_TOPO          (scon_data_type_t)   25 /**< hwloc topology */
#define    SCON_VALUE               (scon_data_type_t)   26 /**< opal value structure */
#define    SCON_BUFFER              (scon_data_type_t)   27 /**< pack the remaining contents of a buffer as an object */
#define    SCON_PTR                 (scon_data_type_t)   28 /**< pointer to void* */
/* SCON Array types */
#define    SCON_FLOAT_ARRAY         (scon_data_type_t)   31
#define    SCON_DOUBLE_ARRAY        (scon_data_type_t)   32
#define    SCON_STRING_ARRAY        (scon_data_type_t)   33
#define    SCON_BOOL_ARRAY          (scon_data_type_t)   34
#define    SCON_SIZE_ARRAY          (scon_data_type_t)   35
#define    SCON_BYTE_ARRAY          (scon_data_type_t)   36
#define    SCON_INT_ARRAY           (scon_data_type_t)   37
#define    SCON_INT8_ARRAY          (scon_data_type_t)   38
#define    SCON_INT16_ARRAY         (scon_data_type_t)   39
#define    SCON_INT32_ARRAY         (scon_data_type_t)   40
#define    SCON_INT64_ARRAY         (scon_data_type_t)   41
#define    SCON_UINT_ARRAY          (scon_data_type_t)   42
#define    SCON_UINT8_ARRAY         (scon_data_type_t)   43
#define    SCON_UINT16_ARRAY        (scon_data_type_t)   44
#define    SCON_UINT32_ARRAY        (scon_data_type_t)   45
#define    SCON_UINT64_ARRAY        (scon_data_type_t)   46
#define    SCON_BYTE_OBJECT_ARRAY   (scon_data_type_t)   47
#define    SCON_PID_ARRAY           (scon_data_type_t)   48
#define    SCON_TIMEVAL_ARRAY       (scon_data_type_t)   49
#define    SCON_NAME                (scon_data_type_t)   50
#define    SCON_JOBID               (scon_data_type_t)   51
#define    SCON_VPID                (scon_data_type_t)   52
/* SCON Dynamic */
#define    SCON_DSS_ID_DYNAMIC      (scon_data_type_t)  100


/* List types for scon_value_t, needs number of elements and a pointer */

/* scon_info_array_t */
typedef struct {
    size_t size;
    struct scon_info_t *array;
} scon_info_array_t;
/* scon_byte_object_t */
typedef struct {
    char *bytes;
    size_t size;
} scon_byte_object_t;
/* scon_float array object */
typedef struct {
    int32_t size;
    float *data;
} scon_float_array_t;
/* scon_double array object */
typedef struct {
    int32_t size;
    double *data;
} scon_double_array_t;
/* scon_string array object */
typedef struct {
    int32_t size;
    char **data;
} scon_string_array_t;
/* scon bool array object */
typedef struct {
    int32_t size;
    bool *data;
} scon_bool_array_t;
/* scon size array object */
typedef struct {
    int32_t size;
    size_t *data;
} scon_size_array_t;
/* scon byte object array object */
typedef struct {
    int32_t size;
    scon_byte_object_t *data;
} scon_byte_object_array_t;
/* scon int array object */
typedef struct {
    int32_t size;
    int *data;
} scon_int_array_t;
/* scon int8 array object */
typedef struct {
    int32_t size;
    int8_t *data;
} scon_int8_array_t;
/* scon int16 array object */
typedef struct {
    int32_t size;
    int16_t *data;
} scon_int16_array_t;
/* scon int32 array object */
typedef struct {
    int32_t size;
    int32_t *data;
} scon_int32_array_t;
/* scon int64 array object */
typedef struct {
    int32_t size;
    int64_t *data;
} scon_int64_array_t;
/* scon uint array object */
typedef struct {
    int32_t size;
    unsigned int *data;
} scon_uint_array_t;
/* scon uint8 array object */
typedef struct {
    int32_t size;
    uint8_t *data;
} scon_uint8_array_t;
/* scon uint16 array object */
typedef struct {
    int32_t size;
    uint16_t *data;
} scon_uint16_array_t;
/* scon uint32 array object */
typedef struct {
    int32_t size;
    uint32_t *data;
} scon_uint32_array_t;
/* scon uint64 array object */
typedef struct {
    int32_t size;
    uint64_t *data;
} scon_uint64_array_t;
/* scon pid array object */
typedef struct {
    int32_t size;
    pid_t *data;
} scon_pid_array_t;
/* scon timeval array object */
typedef struct {
    int32_t size;
    struct timeval *data;
} scon_timeval_array_t;

/**** TO DO: this must match opal value but we do need to add
  * INFO array support */

/* NOTE: operations can supply a collection of values under
 * a single key by passing a scon_value_t containing an
 * array of type SCON_INFO_ARRAY, with each array element
 * containing its own scon_info_t object */
/* Data value object */
typedef struct {
    scon_data_type_t type;              /* the type of value stored */
    union {
        bool flag;
        uint8_t byte;
        char *string;
        size_t size;
        pid_t pid;
        int integer;
        int8_t int8;
        int16_t int16;
        int32_t int32;
        int64_t int64;
        unsigned int uint;
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        uint64_t uint64;
        scon_byte_object_t bo;
        float fval;
        double dval;
        struct timeval tv;
        opal_process_name_t name;
        scon_bool_array_t flag_array;
        scon_uint8_array_t byte_array;
        scon_string_array_t string_array;
        scon_size_array_t size_array;
        scon_int_array_t integer_array;
        scon_int8_array_t int8_array;
        scon_int16_array_t int16_array;
        scon_int32_array_t int32_array;
        scon_int64_array_t int64_array;
        scon_uint_array_t uint_array;
        scon_uint8_array_t uint8_array;
        scon_uint16_array_t uint16_array;
        scon_uint32_array_t uint32_array;
        scon_uint64_array_t uint64_array;
        scon_byte_object_array_t bo_array;
        scon_float_array_t fval_array;
        scon_double_array_t dval_array;
        scon_pid_array_t pid_array;
        scon_timeval_array_t tv_array;
        void *ptr;  // never packed or passed anywhere
    } data;
} scon_value_t;

/****    SCON INFO STRUCT    ****/
typedef struct {
    char key[SCON_MAX_KEYLEN+1];  // ensure room for the NULL terminator
    scon_value_t value;
} scon_info_t;

/****   SCON ERROR CONSTANTS   ****/

#define SCON_ERROR_MAX  17  // set equal to number of non-zero entries in enum

typedef enum {
    SCON_SUCCESS,
    SCON_ERROR,
    SCON_ERR_INFO_KEY_INVALID,
    SCON_ERR_INFO_KEY_UNSUPPORTED,
    SCON_ERR_BAD_NAMESPACE,
    SCON_ERR_NOT_FOUND,
    SCON_ERR_INVALID_HANDLE,
    SCON_ERR_TIMEOUT,
    SCON_ALREADY_INITED,
    SCON_ERR_SILENT,
    SCON_ERR_TYPE_UNSUPPORTED,
    SCON_ERR_TOPO_UNSUPPORTED,
    SCON_ERR_CONFIG_MISMATCH,
    SCON_ERR_LOCAL,
    SCON_ERR_MULTI_JOB_NOT_SUPPORTED,
    SCON_ERR_WILDCARD_NOT_SUPPORTED,
    SCON_ERR_OP_NOT_SUPPORTED,
} scon_status_t;

/****   SCON TOPOLOGY ENUM   ****/
/* To do: need to replace enum with #defined strings for topology to be compatible
   with topology info keys*/
typedef enum {
    SCON_TOPO_UNDEFINED,
    SCON_TOPO_BINOM_TREE,
    SCON_TOPO_RADIX_TREE,
    SCON_TOPO_DEBRUIJN_MESH,
} scon_topo_type_t;

/****    SCON_TYPE_ENUM    ****/
typedef enum {
    /* a SCON of all ranks in a single job */
    SCON_TYPE_MY_JOB_ALL,
    /* a SCON consisting of a subset of ranks of a single job */
    SCON_TYPE_MY_JOB_PARTIAL,
    /* a SCON consisting of ranks from multiple jobs */
    SCON_TYPE_MULTI_JOB,
} scon_type_t;

/*** HELPER MACROS FOR ALLOC/RELEASE of SCON objects ***/

/*** scon_value helper macros ***/
/* allocate and initialize a specified number of value structs */
#define SCON_VALUE_CREATE(m, n)                                         \
    do {                                                                \
        int _ii;                                                        \
        (m) = (scon_value_t*)malloc((n) * sizeof(scon_value_t));        \
        memset((m), 0, (n) * sizeof(scon_value_t));                     \
        for (_ii=0; _ii < (int)(n); _ii++) {                            \
            (m)[_ii].type = SCON_UNDEF;                                 \
        }                                                               \
    } while(0);

/* release a single scon_value_t struct, including its data */
#define SCON_VALUE_RELEASE(m)                                           \
    do {                                                                \
        SCON_VALUE_DESTRUCT((m));                                       \
        free((m));                                                      \
    } while(0);

/* initialize a single value struct */
#define SCON_VALUE_CONSTRUCT(m)                 \
    do {                                        \
        memset((m), 0, sizeof(scon_value_t));   \
        (m)->type = SCON_UNDEF;                 \
    } while(0);

/* release the memory in the value struct data field */
#define SCON_VALUE_DESTRUCT(m)                                          \
    do {                                                                \
        if (SCON_STRING == (m)->type) {                                 \
            if (NULL != (m)->data.string) {                             \
                free((m)->data.string);                                 \
            }                                                           \
        } else if (SCON_BYTE_OBJECT == (m)->type) {                     \
            if (NULL != (m)->data.bo.bytes) {                           \
                free((m)->data.bo.bytes);                               \
            }                                                           \
        } else if (SCON_INFO_ARRAY == (m)->type) {                      \
            size_t _n;                                                  \
            scon_info_t *_p = (scon_info_t*)((m)->data.array.array);    \
            for (_n=0; _n < (m)->data.array.size; _n++) {               \
                if (SCON_STRING == _p[_n].value.type) {                 \
                    if (NULL != _p[_n].value.data.string) {             \
                        free(_p[_n].value.data.string);                 \
                    }                                                   \
                } else if (SCON_BYTE_OBJECT == _p[_n].value.type) {     \
                    if (NULL != _p[_n].value.data.bo.bytes) {           \
                        free(_p[_n].value.data.bo.bytes);               \
                    }                                                   \
                }                                                       \
            }                                                           \
            free(_p);                                                   \
        }                                                               \
    } while(0);

#define SCON_VALUE_FREE(m, n)                           \
    do {                                                \
        size_t _s;                                      \
        if (NULL != (m)) {                              \
            for (_s=0; _s < (n); _s++) {                \
                SCON_VALUE_DESTRUCT(&((m)[_s]));        \
            }                                           \
            free((m));                                  \
        }                                               \
    } while(0);


/*** scon_info helper macros ***/

/* utility macros for working with scon_info_t structs */
#define SCON_INFO_CREATE(m, n)                                  \
    do {                                                        \
        (m) = (scon_info_t*)malloc((n) * sizeof(scon_info_t));  \
        memset((m), 0, (n) * sizeof(scon_info_t));              \
    } while(0);

#define SCON_INFO_CONSTRUCT(m)                  \
    do {                                        \
        memset((m), 0, sizeof(scon_info_t));    \
        (m)->value.type = SCON_UNDEF;           \
    } while(0);

#define SCON_INFO_DESTRUCT(m) \
    do {                                        \
        SCON_VALUE_DESTRUCT(&(m)->value);       \
    } while(0);

#define SCON_INFO_FREE(m, n)                    \
    do {                                        \
        size_t _s;                              \
        if (NULL != (m)) {                      \
            for (_s=0; _s < (n); _s++) {        \
                SCON_INFO_DESTRUCT(&((m)[_s])); \
            }                                   \
            free((m));                          \
        }                                       \
    } while(0);

#define SCON_INFO_LOAD(m, k, v, t)                      \
    do {                                                \
        (void)strncpy((m)->key, (k), SCON_MAX_KEYLEN);  \
        scon_value_load(&((m)->value), (v), (t));       \
    } while(0);

/*** scon_proc helper macros ***/
#define SCON_PROC_CREATE(m, n)                              \
    do {                                                    \
    (m) = (scon_proc_t*)malloc((n) * sizeof(scon_proc_t));  \
    memset((m), 0, (n) * sizeof(scon_proc_t));              \
} while(0);

#define SCON_PROC_RELEASE(m)                    \
    do {                                        \
    SCON_PROC_FREE((m));                        \
} while(0);

#define SCON_PROC_CONSTRUCT(m)                  \
    do {                                        \
    memset((m), 0, sizeof(scon_proc_t));        \
} while(0);

#define SCON_PROC_DESTRUCT(m)

#define SCON_PROC_FREE(m, n)                    \
    do {                                        \
        if (NULL != (m)) {                      \
        free((m));                          \
    }                                       \
} while(0);

END_C_DECLS
#endif
