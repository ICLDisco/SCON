/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2016 Intel, Inc. All rights reserved
 * Copyright (c) 2016      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer listed
 *   in this license in the documentation and/or other materials
 *   provided with the distribution.
 *
 * - Neither the name of the copyright holders nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * The copyright holders provide no reassurances that the source code
 * provided does not infringe any patent, copyright, or any other
 * intellectual property rights of third parties.  The copyright holders
 * disclaim any liability to any recipient for claims brought against
 * recipient by any third party for infringement of that parties
 * intellectual property rights.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 *
 * $HEADER$
 */

#ifndef SCON_COMMON_H
#define SCON_COMMON_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "src/class/scon_object.h"
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* for struct timeval */
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* for uid_t and gid_t */
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> /* for uid_t and gid_t */
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/****  SCON CONSTANTS    ****/

/* define maximum value and key sizes */
#define SCON_MAX_JOBLEN  255
#define SCON_MAX_NSLEN  255
#define SCON_MAX_KEYLEN  511
#if defined(MAXHOSTNAMELEN)
#define SCON_MAXHOSTNAMELEN (MAXHOSTNAMELEN + 1)
#elif defined(HOST_NAME_MAX)
#define SCON_MAXHOSTNAMELEN (HOST_NAME_MAX + 1)
#else
    /* SUSv2 guarantees that "Host names are limited to 255 bytes". */
#define SCON_MAXHOSTNAMELEN (255 + 1)
#endif

/* define for scon handle */
typedef int32_t scon_handle_t;


/* define a value for requests for job-level data
 * where the info itself isn't associated with any
 * specific rank, or when a request involves
 * a rank that isn't known - e.g., when someone requests
 * info thru one of the legacy interfaces where the rank
 * is typically encoded into the key itself since there is
 * no rank parameter in the API itself */
#define SCON_RANK_UNDEF     INT32_MAX

/* define a *wildcard* value for requests involving any process */
#define SCON_RANK_WILDCARD  INT32_MAX-1

/****    SCON ERROR CONSTANTS    ****/
/* SCON errors are always negative, with 0 reserved for success */
#define SCON_ERR_BASE                   0

typedef int scon_status_t;

#define SCON_SUCCESS                            (SCON_ERR_BASE)
#define SCON_ERROR                              (SCON_ERR_BASE -  1)    // general error
#define SCON_ERR_INFO_KEY_INVALID               (SCON_ERR_BASE -  2)
#define SCON_ERR_TIMEOUT                        (SCON_ERR_BASE -  3)
#define SCON_ERR_INFO_KEY_NOT_SUPPORTED         (SCON_ERR_BASE -  4)
#define SCON_ERR_BAD_NAMESPACE                  (SCON_ERR_BASE -  5)
#define SCON_ERR_NOT_FOUND                      (SCON_ERR_BASE -  6)
/* communication failures */
#define SCON_ERR_UNREACH                        (SCON_ERR_BASE -  7)
#define SCON_ERR_LOST_PEER_CONNECTION           (SCON_ERR_BASE -  8)
/* operational */
#define SCON_ERR_NO_PERMISSIONS                 (SCON_ERR_BASE -  9)
#define SCON_ALREADY_INITED                     (SCON_ERR_BASE - 10)
#define SCON_EXISTS                             (SCON_ERR_BASE - 11)
#define SCON_ERR_NOT_SUPPORTED                  (SCON_ERR_BASE - 12)
#define SCON_ERR_INVALID_CFG                    (SCON_ERR_BASE - 13)
#define SCON_ERR_BAD_PARAM                      (SCON_ERR_BASE - 14)
#define SCON_ERR_DATA_VALUE_NOT_FOUND           (SCON_ERR_BASE - 15)
#define SCON_ERR_OUT_OF_RESOURCE                (SCON_ERR_BASE - 16)
#define SCON_ERR_TYPE_NOTSUPPORTED              (SCON_ERR_BASE - 17)
#define SCON_ERR_INVALID_SIZE                   (SCON_ERR_BASE - 18)
#define SCON_ERR_INIT                           (SCON_ERR_BASE - 19)
#define SCON_ERR_TOPO_UNSUPPORTED               (SCON_ERR_BASE - 20)
#define SCON_ERR_NOT_IMPLEMENTED                (SCON_ERR_BASE - 21)
/* system failures */
#define SCON_ERR_NODE_DOWN                      (SCON_ERR_BASE - 22)
#define SCON_ERR_NODE_OFFLINE                   (SCON_ERR_BASE - 23)

#define SCON_ERR_CONFIG_MISMATCH                (SCON_ERR_BASE - 24)
#define SCON_ERR_MULTI_JOB_NOT_SUPPORTED        (SCON_ERR_BASE - 25)

/* used by the query system */
#define SCON_QUERY_PARTIAL_SUCCESS              (SCON_ERR_BASE - 26)


/* define a starting point for SCON internal error codes
 * that are never exposed outside the library */
#define SCON_INTERNAL_ERR_BASE          -1000



typedef enum {
    SCON_TOPO_UNDEFINED,
    SCON_TOPO_BINOM_TREE,
    SCON_TOPO_RADIX_TREE,
    SCON_TOPO_DEBRUIJN_MESH,
} scon_topo_type_t;

typedef enum {
    SCON_TYPE_MY_JOB_ALL,
    SCON_TYPE_MY_JOB_PARTIAL,
    SCON_TYPE_MULTI_JOB,
} scon_type_t;

/****    SCON PROC OBJECT    ****/
typedef struct {
    char job_name[SCON_MAX_JOBLEN+1];
    unsigned int rank;
} scon_proc_t;
#define SCON_PROC_CREATE(m, n)                                  \
    do {                                                        \
    (m) = (scon_proc_t*)malloc((n) * sizeof(scon_proc_t));  \
    memset((m), 0, (n) * sizeof(scon_proc_t));              \
} while (0)

#define SCON_PROC_RELEASE(m)                    \
        do {                                        \
        SCON_PROC_FREE((m), 1);                 \
    } while (0)

#define SCON_PROC_CONSTRUCT(m)                  \
            do {                                        \
            memset((m), 0, sizeof(scon_proc_t));    \
        } while (0)

#define SCON_PROC_DESTRUCT(m)

#define SCON_PROC_FREE(m, n)                    \
                do {                                        \
                    if (NULL != (m)) {                      \
                    free((m));                          \
                    (m) = NULL;                         \
                }                                       \
            } while (0)


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
#define    SCON_VALUE               (scon_data_type_t)   26 /**< scon value structure */
#define    SCON_BUFFER              (scon_data_type_t)   27 /**< pack the remaining contents of a buffer as an object */
#define    SCON_POINTER             (scon_data_type_t)   28 /**< pointer to void* */
#define    SCON_INFO                (scon_data_type_t)   29
#define    SCON_INFO_ARRAY          (scon_data_type_t)   30
#define    SCON_PROC                (scon_data_type_t)   31
#define    SCON_PROC_RANK           (scon_data_type_t)   53
#define    SCON_DATA_RANGE          (scon_data_type_t)   54
#define    SCON_INFO_DIRECTIVES     (scon_data_type_t)   55
#define    SCON_PROC_STATUS         (scon_data_type_t)   56
#define    SCON_DATA_ARRAY          (scon_data_type_t)   56
/* SCON Array types */
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
#define    SCON_FLOAT_ARRAY         (scon_data_type_t)   50
#define    SCON_NAME_SPACE          (scon_data_type_t)   51
#define    SCON_STATUS              (scon_data_type_t)   52
/* SCON Dynamic */
#define    SCON_DSS_ID_DYNAMIC      (scon_data_type_t)  100

typedef uint32_t scon_rank_t;

/* define a range for data "published" by SCON
 */
typedef uint8_t scon_member_range_t;
#define SCON_RANGE_UNDEF        0
#define SCON_RANGE_LOCAL        1   // all local processes
#define SCON_RANGE_NAMESPACE    2   // all members belonging to the namespace only
#define SCON_RANGE_SESSION      3   // all procs in the given session
#define SCON_RANGE_USER         4   // range is specified in a scon_info_t

/****    SCON VALUE STRUCT    ****/
struct scon_info_t;

typedef struct {
    size_t size;
    struct scon_info_t *array;
} scon_info_array_t;

/****    SCON BYTE OBJECT    ****/
typedef struct {
    char *bytes;
    size_t size;
} scon_byte_object_t;

/* List types for scon_value_t, needs number of elements and a pointer */
/* float array object */
typedef struct {
    int32_t size;
    float *data;
} scon_float_array_t;
/* double array object */
typedef struct {
    int32_t size;
    double *data;
} scon_double_array_t;
/* string array object */
typedef struct {
    int32_t size;
    char **data;
} scon_string_array_t;
/* bool array object */
typedef struct {
    int32_t size;
    bool *data;
} scon_bool_array_t;
/* size array object */
typedef struct {
    int32_t size;
    size_t *data;
} scon_size_array_t;
/* opal byte object array object */
typedef struct {
    int32_t size;
    scon_byte_object_t *data;
} scon_byte_object_array_t;
/* int array object */
typedef struct {
    int32_t size;
    int *data;
} scon_int_array_t;
/* int8 array object */
typedef struct {
    int32_t size;
    int8_t *data;
} scon_int8_array_t;
/* int16 array object */
typedef struct {
    int32_t size;
    int16_t *data;
} scon_int16_array_t;
/* int32 array object */
typedef struct {
    int32_t size;
    int32_t *data;
} scon_int32_array_t;
/* int64 array object */
typedef struct {
    int32_t size;
    int64_t *data;
} scon_int64_array_t;
/* uint array object */
typedef struct {
    int32_t size;
    unsigned int *data;
} scon_uint_array_t;
/* uint8 array object */
typedef struct {
    int32_t size;
    uint8_t *data;
} scon_uint8_array_t;
/* uint16 array object */
typedef struct {
    int32_t size;
    uint16_t *data;
} scon_uint16_array_t;
/* uint32 array object */
typedef struct {
    int32_t size;
    uint32_t *data;
} scon_uint32_array_t;
/* uint64 array object */
typedef struct {
    int32_t size;
    uint64_t *data;
} scon_uint64_array_t;
/* pid array object */
typedef struct {
    int32_t size;
    pid_t *data;
} scon_pid_array_t;
/* timeval array object */
typedef struct {
    int32_t size;
    struct timeval *data;
} scon_timeval_array_t;

typedef struct scon_data_array {
    scon_data_type_t type;
    size_t size;
    void *array;
} scon_data_array_t;

/**** TO DO: this matches opal value but we do need to add
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
        time_t time ;
        struct timeval tv;
        scon_proc_t *proc;
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
        scon_data_array_t *darray;
        scon_member_range_t range;
        scon_status_t status;
        void *ptr;
    } data;
} scon_value_t;

/* allocate and initialize a specified number of value structs */
#define SCON_VALUE_CREATE(m, n)                                 \
    do {                                                        \
        int _ii;                                                \
        scon_value_t *_v;                                       \
        (m) = (scon_value_t*)calloc((n), sizeof(scon_value_t)); \
        _v = (scon_value_t*)(m);                                \
        if (NULL != (m)) {                                      \
            for (_ii=0; _ii < (int)(n); _ii++) {                \
                _v[_ii].type = SCON_UNDEF;                     \
            }                                                   \
        }                                                       \
    } while (0)

/* release a single scon_value_t struct, including its data */
#define SCON_VALUE_RELEASE(m)       \
    do {                            \
        SCON_VALUE_DESTRUCT((m));   \
        free((m));                  \
    } while (0)

/* initialize a single value struct */
#define SCON_VALUE_CONSTRUCT(m)                 \
    do {                                        \
        memset((m), 0, sizeof(scon_value_t));   \
        (m)->type = SCON_UNDEF;                 \
    } while (0)

/* release the memory in the value struct data field */
#define SCON_VALUE_DESTRUCT(m)                                                  \
    do {                                                                        \
        size_t _n;                                                              \
        if (SCON_STRING == (m)->type) {                                         \
            if (NULL != (m)->data.string) {                                     \
                free((m)->data.string);                                         \
            }                                                                   \
        } else if (SCON_BYTE_OBJECT == (m)->type) {                             \
            if (NULL != (m)->data.bo.bytes) {                                   \
                free((m)->data.bo.bytes);                                       \
            }                                                                   \
        } else if (SCON_DATA_ARRAY == (m)->type) {                              \
            if (SCON_STRING == (m)->data.darray->type) {                        \
                char **_str = (char**)(m)->data.darray->array;                  \
                for (_n=0; _n < (m)->data.darray->size; _n++) {                 \
                    if (NULL != _str[_n]) {                                     \
                        free(_str[_n]);                                         \
                    }                                                           \
                }                                                               \
            } else if (SCON_INFO == (m)->data.darray->type) {                   \
                scon_info_t *_info =                                            \
                            (scon_info_t*)(m)->data.darray->array;              \
                for (_n=0; _n < (m)->data.darray->size; _n++) {                 \
                    /* cannot use info destruct as that loops back */           \
                    if (SCON_STRING == _info[_n].value.type) {                  \
                        if (NULL != _info[_n].value.data.string) {              \
                            free(_info[_n].value.data.string);                  \
                        }                                                       \
                    } else if (SCON_BYTE_OBJECT == _info[_n].value.type) {      \
                        if (NULL != _info[_n].value.data.bo.bytes) {            \
                            free(_info[_n].value.data.bo.bytes);                \
                        }                                                       \
                    }                                                           \
                }                                                               \
            } else if (SCON_BYTE_OBJECT == (m)->data.darray->type) {            \
                scon_byte_object_t *_obj =                                      \
                            (scon_byte_object_t*)(m)->data.darray->array;       \
                for (_n=0; _n < (m)->data.darray->size; _n++) {                 \
                    if (NULL != _obj[_n].bytes) {                               \
                        free(_obj[_n].bytes);                                   \
                    }                                                           \
                }                                                               \
            }                                                                   \
            if (NULL != (m)->data.darray->array) {                              \
                free((m)->data.darray->array);                                  \
            }                                                                   \
        }                                                                       \
    } while (0)

#define SCON_VALUE_FREE(m, n)                           \
    do {                                                \
        size_t _s;                                      \
        if (NULL != (m)) {                              \
            for (_s=0; _s < (n); _s++) {                \
                SCON_VALUE_DESTRUCT(&((m)[_s]));        \
            }                                           \
            free((m));                                  \
        }                                               \
    } while (0)

/* define the results values for comparisons so we can change them in only one place */
#define SCON_VALUE1_GREATER  +1
#define SCON_VALUE2_GREATER  -1
#define SCON_EQUAL            0

/**
 * buffer type
 */
enum scon_bfrop_buffer_type_t {
    SCON_BFROP_BUFFER_NON_DESC   = 0x00,
    SCON_BFROP_BUFFER_FULLY_DESC = 0x01
};

typedef enum scon_bfrop_buffer_type_t scon_bfrop_buffer_type_t;

#define SCON_BFROP_BUFFER_TYPE_HTON(h);
#define SCON_BFROP_BUFFER_TYPE_NTOH(h);

/**
 * Structure for holding a buffer */
typedef struct {
    /** First member must be the object's parent */
    scon_object_t parent;
    /** type of buffer */
    scon_bfrop_buffer_type_t type;
    /** Start of my memory */
    char *base_ptr;
    /** Where the next data will be packed to (within the allocated
        memory starting at base_ptr) */
    char *pack_ptr;
    /** Where the next data will be unpacked from (within the
        allocated memory starting as base_ptr) */
    char *unpack_ptr;

    /** Number of bytes allocated (starting at base_ptr) */
    size_t bytes_allocated;
    /** Number of bytes used by the buffer (i.e., amount of data --
        including overhead -- packed in the buffer) */
    size_t bytes_used;
} scon_buffer_t;



/* SCON INFO KEYS */
/* define strings for key - comments contain hints of value type */
#define SCON_EVENT_BASE            "scon.evbase"           /* (struct event_base *) pointer to libevent event_base to use in place
                                                             of the internal progress thread */
/*  JOB and PROC keys*/
#define SCON_JOB_NAME              "scon.job.name"   /* job name key, value type char * - name of participating job. */
#define SCON_JOB_ALL               "scon.job.all"  /* value type bool, set to true if all ranks of job participate in scon
                                                           users can set this key instead of listing all ranks in job*/
#define SCON_JOB_RANKS             "scon.job.ranks" /* array of job ranks participating in scon. value - scon_job_rank_info_t */
#define SCON_NUM_JOBS              "scon.numjobs"  /* number of jobs participating in scon */

/* SCON Topology keys */
#define SCON_TOPO_TYPE             "scon.topo.type"     /* topology type  value is enum  scon_topo_type */
#define SCON_TOPO_TREE_ROOT        "scon.topo.tree.root"     /* value - scon_proc_t name of the root process
                                                                (default is rank 0 of lowest job)*/
#define SCON_TOPO_TREE_DEPTH       "scon_topo.tree.depth"  /* maximum depth of the tree  value - uint*/
#define SCON_TOPO_FILENAME         "scon.topo.filename"  /* topology file name, value is char * (path to file name*/

/* SCON attribute keys */

#define SCON_PRIORITY              "scon.priority"      /* int relative priority of this scon wrt to other scons */
#define SCON_TRANSPORT             "scon.transport"     /*  enum - SCON_SUPPORTED_TRANSPORTS (TCP/IP, UD,  */
#define SCON_SELECTED_FABRICS      "scon.fabrics.sel"   /* value - string - a selection string specifying the physical fabrics
                                                                 that are allowed to exchange msgs */
#define SCON_TOPO_MASTER_PROC      "scon.topo.master"   /* master process for the scon - topology root
                                                               value is scon_proc_t */



/* define a set of bit-mask flags for specifying behavior of
 * command directives via scon_info_t arrays */
typedef uint32_t scon_info_directives_t;
#define SCON_INFO_REQD          0x0001



/* expose two functions that are resolved in the
 * SCON library, but part of a header that
 * includes internal functions - we don't
 * want to expose the entire header here
 */
void scon_value_load(scon_value_t *v, void *data, scon_data_type_t type);
scon_status_t scon_value_xfer(scon_value_t *kv, scon_value_t *src);




/****    SCON INFO STRUCT    ****/
typedef struct {
    char key[SCON_MAX_KEYLEN+1];    // ensure room for the NULL terminator
    scon_info_directives_t flags;   // bit-mask of flags
    scon_value_t value;
}scon_info_t;

/* utility macros for working with scon_info_t structs */
#define SCON_INFO_CREATE(m, n)                                  \
    do {                                                        \
        (m) = (scon_info_t*)malloc((n) * sizeof(scon_info_t));  \
        memset((m), 0, (n) * sizeof(scon_info_t));              \
    } while (0)

#define SCON_INFO_CONSTRUCT(m)                  \
    do {                                        \
        memset((m), 0, sizeof(scon_info_t));    \
        (m)->value.type = SCON_UNDEF;           \
    } while (0)

#define SCON_INFO_DESTRUCT(m) \
    do {                                        \
        SCON_VALUE_DESTRUCT(&(m)->value);       \
    } while (0)

#define SCON_INFO_FREE(m, n)                    \
    do {                                        \
        size_t _s;                              \
        if (NULL != (m)) {                      \
            for (_s=0; _s < (n); _s++) {        \
                SCON_INFO_DESTRUCT(&((m)[_s])); \
            }                                   \
            free((m));                          \
            (m) = NULL;                         \
        }                                       \
    } while (0)

#define SCON_INFO_LOAD(m, k, v, t)                      \
    do {                                                \
        (void)strncpy((m)->key, (k), SCON_MAX_KEYLEN);  \
        scon_value_load(&((m)->value), (v), (t));       \
    } while (0)
#define SCON_INFO_XFER(d, s)                                \
    do {                                                    \
        (void)strncpy((d)->key, (s)->key, SCON_MAX_KEYLEN); \
        (d)->flags = (s)->flags;                            \
        scon_value_xfer(&(d)->value, &(s)->value);          \
    } while(0)

#define SCON_INFO_REQUIRED(m)       \
    (m)->flags |= SCON_INFO_REQD;
#define SCON_INFO_OPTIONAL(m)       \
    (m)->flags &= ~SCON_INFO_REQD;



/* Key-Value pair management macros */
// TODO: add all possible types/fields here.

#define SCON_VAL_FIELD_int(x)       ((x)->data.integer)
#define SCON_VAL_FIELD_uint32_t(x)  ((x)->data.uint32)
#define SCON_VAL_FIELD_uint16_t(x)  ((x)->data.uint16)
#define SCON_VAL_FIELD_string(x)    ((x)->data.string)
#define SCON_VAL_FIELD_float(x)     ((x)->data.fval)
#define SCON_VAL_FIELD_byte(x)      ((x)->data.byte)
#define SCON_VAL_FIELD_flag(x)      ((x)->data.flag)

#define SCON_VAL_TYPE_int      SCON_INT
#define SCON_VAL_TYPE_uint32_t SCON_UINT32
#define SCON_VAL_TYPE_uint16_t SCON_UINT16
#define SCON_VAL_TYPE_string   SCON_STRING
#define SCON_VAL_TYPE_float    SCON_FLOAT
#define SCON_VAL_TYPE_byte     SCON_BYTE
#define SCON_VAL_TYPE_flag     SCON_BOOL

#define SCON_VAL_set_assign(_v, _field, _val )   \
    do {                                                            \
        (_v)->type = SCON_VAL_TYPE_ ## _field;                      \
        SCON_VAL_FIELD_ ## _field((_v)) = _val;                     \
    } while (0)

#define SCON_VAL_set_strdup(_v, _field, _val )       \
    do {                                                                \
        (_v)->type = SCON_VAL_TYPE_ ## _field;                          \
        SCON_VAL_FIELD_ ## _field((_v)) = strdup(_val);                 \
    } while (0)

#define SCON_VAL_SET_int        SCON_VAL_set_assign
#define SCON_VAL_SET_uint32_t   SCON_VAL_set_assign
#define SCON_VAL_SET_uint16_t   SCON_VAL_set_assign
#define SCON_VAL_SET_string     SCON_VAL_set_strdup
#define SCON_VAL_SET_float      SCON_VAL_set_assign
#define SCON_VAL_SET_byte       SCON_VAL_set_assign
#define SCON_VAL_SET_flag       SCON_VAL_set_assign

#define SCON_VAL_SET(_v, _field, _val )   \
    SCON_VAL_SET_ ## _field(_v, _field, _val)

#define SCON_VAL_cmp_val(_val1, _val2)      ((_val1) != (_val2))
#define SCON_VAL_cmp_float(_val1, _val2)    (((_val1)>(_val2))?(((_val1)-(_val2))>0.000001):(((_val2)-(_val1))>0.000001))
#define SCON_VAL_cmp_ptr(_val1, _val2)      strncmp(_val1, _val2, strlen(_val1)+1)

#define SCON_VAL_CMP_int        SCON_VAL_cmp_val
#define SCON_VAL_CMP_uint32_t   SCON_VAL_cmp_val
#define SCON_VAL_CMP_uint16_t   SCON_VAL_cmp_val
#define SCON_VAL_CMP_float      SCON_VAL_cmp_float
#define SCON_VAL_CMP_string     SCON_VAL_cmp_ptr
#define SCON_VAL_CMP_byte       SCON_VAL_cmp_val
#define SCON_VAL_CMP_flag       SCON_VAL_cmp_val

#define SCON_VAL_CMP(_field, _val1, _val2) \
    SCON_VAL_CMP_ ## _field(_val1, _val2)

#define SCON_VAL_FREE(_v) \
     SCON_free_value_data(_v)

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
