/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2012 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>


#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <errno.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <scon_common.h>

#include "src/util/error.h"

SCON_EXPORT const char* SCON_Error_string(scon_status_t errnum)
{
    switch(errnum) {
    case SCON_ERR_UNPACK_READ_PAST_END_OF_BUFFER:
        return "UNPACK-PAST-END";
    case SCON_ERR_LOST_PEER_CONNECTION:
        return "LOST_CONNECTION_TO_PEER";
    case SCON_ERR_NOT_SUPPORTED:
        return "NOT-SUPPORTED";
    case SCON_ERR_NOT_FOUND:
        return "NOT-FOUND";
    case SCON_ERR_INVALID_SIZE:
        return "INVALID-SIZE";
    case SCON_ERR_INVALID_NUM_PARSED:
        return "INVALID-NUM-PARSED";

    case SCON_ERR_INVALID_ARGS:
        return "INVALID-ARGS";
    case SCON_ERR_INVALID_NUM_ARGS:
        return "INVALID-NUM-ARGS";
    case SCON_ERR_INVALID_LENGTH:
        return "INVALID-LENGTH";
    case SCON_ERR_INVALID_VAL_LENGTH:
        return "INVALID-VAL-LENGTH";
    case SCON_ERR_INVALID_VAL:
        return "INVALID-VAL";
    case SCON_ERR_INVALID_KEY_LENGTH:
        return "INVALID-KEY-LENGTH";
    case SCON_ERR_INVALID_KEY:
        return "INVALID-KEY";
    case SCON_ERR_INVALID_ARG:
        return "INVALID-ARG";
    case SCON_ERR_NOMEM:
        return "NO-MEM";
    case SCON_ERR_INIT:
        return "INIT";

    case SCON_ERR_DATA_VALUE_NOT_FOUND:
        return "DATA-VALUE-NOT-FOUND";
    case SCON_ERR_OUT_OF_RESOURCE:
        return "OUT-OF-RESOURCE";
    case SCON_ERR_BAD_PARAM:
        return "BAD-PARAM";
    case SCON_ERR_IN_ERRNO:
        return "ERR-IN-ERRNO";
    case SCON_ERR_UNREACH:
        return "UNREACHABLE";
    case SCON_ERR_TIMEOUT:
        return "TIMEOUT";
    case SCON_ERR_NO_PERMISSIONS:
        return "NO-PERMISSIONS";
    case SCON_ERR_PACK_MISMATCH:
        return "PACK-MISMATCH";
    case SCON_ERR_PACK_FAILURE:
        return "PACK-FAILURE";

    case SCON_ERR_UNPACK_FAILURE:
        return "UNPACK-FAILURE";
    case SCON_ERR_UNPACK_INADEQUATE_SPACE:
        return "UNPACK-INADEQUATE-SPACE";
    case SCON_ERR_TYPE_MISMATCH:
        return "TYPE-MISMATCH";
    case SCON_ERR_UNKNOWN_DATA_TYPE:
        return "UNKNOWN-DATA-TYPE";
    case SCON_ERR_READY_FOR_HANDSHAKE:
        return "READY-FOR-HANDSHAKE";
    case SCON_ERR_HANDSHAKE_FAILED:
        return "HANDSHAKE-FAILED";
    case SCON_ERR_INVALID_CRED:
        return "INVALID-CREDENTIAL";
    case SCON_EXISTS:
        return "EXISTS";
    case SCON_ERR_SILENT:
        return "SILENT_ERROR";
    case SCON_ERROR:
        return "ERROR";
    case SCON_ERR_NOT_AVAILABLE:
        return "SCON_ERR_NOT_AVAILABLE";
    case SCON_ERR_FATAL:
        return "SCON_ERR_FATAL";
    case SCON_ERR_VALUE_OUT_OF_BOUNDS:
        return "SCON_ERR_VALUE_OUT_OF_BOUNDS";
    case SCON_ERR_INFO_KEY_INVALID:
        return "SCON_ERR_INVALID_INFO_KEY";
    case SCON_ERR_INFO_KEY_NOT_SUPPORTED:
        return "SCON_ERR_INVALID_INFO_KEY";
    case SCON_ERR_PERM:
        return "SCON_ERR_PERM";
    case SCON_SUCCESS:
        return "SUCCESS";
    case SCON_ERR_NETWORK_NOT_PARSEABLE:
        return "NETWORK_NOT_PARSEABLE";
    case SCON_ERR_WOULD_BLOCK:
        return "OPERATION_WOULD_BLOCK";
    default:
        return "ERROR STRING NOT FOUND";
    }
}
