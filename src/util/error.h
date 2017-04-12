/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015-2017 Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_UTIL_ERROR_H
#define SCON_UTIL_ERROR_H

#include <src/include/scon_config.h>


#include <scon_common.h>
#include "src/util/output.h"

 BEGIN_C_DECLS

/* internal error codes - never exposed outside of the library */
#define SCON_ERR_INVALID_CRED                           (SCON_INTERNAL_ERR_BASE -  1)
#define SCON_ERR_HANDSHAKE_FAILED                       (SCON_INTERNAL_ERR_BASE -  2)
#define SCON_ERR_READY_FOR_HANDSHAKE                    (SCON_INTERNAL_ERR_BASE -  3)
#define SCON_ERR_UNKNOWN_DATA_TYPE                      (SCON_INTERNAL_ERR_BASE -  4)
#define SCON_ERR_TYPE_MISMATCH                          (SCON_INTERNAL_ERR_BASE -  5)
#define SCON_ERR_UNPACK_INADEQUATE_SPACE                (SCON_INTERNAL_ERR_BASE -  6)
#define SCON_ERR_UNPACK_FAILURE                         (SCON_INTERNAL_ERR_BASE -  7)
#define SCON_ERR_PACK_FAILURE                           (SCON_INTERNAL_ERR_BASE -  8)
#define SCON_ERR_PACK_MISMATCH                          (SCON_INTERNAL_ERR_BASE -  9)
#define SCON_ERR_PROC_ENTRY_NOT_FOUND                   (SCON_INTERNAL_ERR_BASE - 10)
#define SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER         (SCON_INTERNAL_ERR_BASE - 11)
#define SCON_ERR_INVALID_KEYVAL                        (SCON_INTERNAL_ERR_BASE - 13)
#define SCON_ERR_INVALID_NUM_PARSED                     (SCON_INTERNAL_ERR_BASE - 14)
#define SCON_ERR_INVALID_ARGS                           (SCON_INTERNAL_ERR_BASE - 15)
#define SCON_ERR_INVALID_NUM_ARGS                       (SCON_INTERNAL_ERR_BASE - 16)
#define SCON_ERR_INVALID_LENGTH                         (SCON_INTERNAL_ERR_BASE - 17)
#define SCON_ERR_INVALID_VAL_LENGTH                     (SCON_INTERNAL_ERR_BASE - 18)
#define SCON_ERR_INVALID_VAL                            (SCON_INTERNAL_ERR_BASE - 19)
#define SCON_ERR_INVALID_KEY_LENGTH                     (SCON_INTERNAL_ERR_BASE - 20)
#define SCON_ERR_INVALID_KEY                            (SCON_INTERNAL_ERR_BASE - 21)
#define SCON_ERR_INVALID_ARG                            (SCON_INTERNAL_ERR_BASE - 22)
#define SCON_ERR_NOMEM                                  (SCON_INTERNAL_ERR_BASE - 23)
#define SCON_ERR_IN_ERRNO                               (SCON_INTERNAL_ERR_BASE - 24)
#define SCON_ERR_SILENT                                 (SCON_INTERNAL_ERR_BASE - 25)
#define SCON_ERR_UNKNOWN_DATATYPE                       (SCON_INTERNAL_ERR_BASE - 26)
#define SCON_ERR_RESOURCE_BUSY                          (SCON_INTERNAL_ERR_BASE - 27)
#define SCON_ERR_NOT_AVAILABLE                          (SCON_INTERNAL_ERR_BASE - 28)
#define SCON_ERR_FATAL                                  (SCON_INTERNAL_ERR_BASE - 29)
#define SCON_ERR_VALUE_OUT_OF_BOUNDS                    (SCON_INTERNAL_ERR_BASE - 30)
#define SCON_ERR_PERM                                   (SCON_INTERNAL_ERR_BASE - 31)
#define SCON_ERR_OPERATION_IN_PROGRESS                  (SCON_INTERNAL_ERR_BASE - 32)
#define SCON_ERR_INVALID_HANDLE                         (SCON_INTERNAL_ERR_BASE - 33)
#define SCON_ERR_NETWORK_NOT_PARSEABLE                  (SCON_INTERNAL_ERR_BASE - 34)
#define SCON_ERR_TAKE_NEXT_OPTION                       (SCON_INTERNAL_ERR_BASE - 35)
#define SCON_ERR_SOCKET_NOT_AVAILABLE                   (SCON_INTERNAL_ERR_BASE - 36)
#define SCON_ERR_SYS_LIMITS_SOCKETS                     (SCON_INTERNAL_ERR_BASE - 37)
#define SCON_ERR_CONNECTION_REFUSED                     (SCON_INTERNAL_ERR_BASE - 38)
#define SCON_ERR_COMM_FAILURE                           (SCON_INTERNAL_ERR_BASE - 39)
#define SCON_ERR_CONFIG_NOT_SUPPORTED                   (SCON_INTERNAL_ERR_BASE - 40)
#define SCON_ERR_WOULD_BLOCK                            (SCON_INTERNAL_ERR_BASE - 41)
#define SCON_ERR_PMIXINIT_FAILED                        (SCON_INTERNAL_ERR_BASE - 42)
#define SCON_ERR_PMIXGET_FAILED                         (SCON_INTERNAL_ERR_BASE - 43)
#define SCON_ERR_CREATE_XCAST_SEND_FAIL                 (SCON_INTERNAL_ERR_BASE - 44)
#define SCON_ERR_ADDRESSEE_UNKNOWN                      (SCON_INTERNAL_ERR_BASE - 45)


#define SCON_ERROR_LOG(r)                                           \
 do {                                                               \
    if (SCON_ERR_SILENT != (r)) {                                   \
        scon_output(0, "SCON ERROR: %s in file %s at line %d",      \
                    SCON_Error_string((r)), __FILE__, __LINE__);    \
    }                                                               \
} while (0)

const char* SCON_Error_string(scon_status_t errnum);
 END_C_DECLS

#endif /* SCON_UTIL_ERROR_H */
