/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2010      Oracle and/or its affiliates.  All rights reserved.
 * Copyright (c) 2014-2016 Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "scon_config.h"
#include "scon_types.h"
#include "scon_common.h"

#include <stdio.h>
#include <string.h>

#include "src/util/printf.h"

#include "src/buffer_ops/internal.h"
#include "src/util/error.h"
#include "src/util/name_fns.h"

#define SCON_PRINT_NAME_ARGS_MAX_SIZE   50
#define SCON_PRINT_NAME_ARG_NUM_BUFS    16

#define SCON_SCHEMA_DELIMITER_CHAR      '.'
#define SCON_SCHEMA_WILDCARD_CHAR       '*'
#define SCON_SCHEMA_WILDCARD_STRING     "*"
#define SCON_SCHEMA_INVALID_CHAR        '$'
#define SCON_SCHEMA_INVALID_STRING      "$"




SCON_EXPORT char* scon_util_print_name_args(const scon_proc_t *name)
{
    char *output = NULL;
   // scon_value_t src;
    int ret = SCON_SUCCESS;
   // SCON_PROC_CREATE(src.data.proc, 1);
   // memcpy(src.data.proc, name, sizeof(scon_proc_t));
    //scon_value_load(&src, (void *)name,
      //              SCON_PROC);

    ret= scon_bfrop.print(&output, NULL, name, SCON_PROC);
    scon_output(0, "testing scon_util_print_name_args input namespace = %s, rank =%d", name->job_name, name->rank);
    scon_output(0, "testing scon_util_print_name_args output =%s ret =%d", output, ret);
    return output;
}



/****    COMPARE NAME FIELDS     ****/
int scon_util_compare_name_fields(scon_ns_cmp_bitmask_t fields,
                                  const scon_proc_t* name1,
                                  const scon_proc_t* name2)
{
    /* handle the NULL pointer case */
    if (NULL == name1 && NULL == name2) {
        return SCON_EQUAL;
    } else if (NULL == name1) {
        return SCON_VALUE2_GREATER;
    } else if (NULL == name2) {
        return SCON_VALUE1_GREATER;
    }

    /* in this comparison function, we check for exact equalities.
     * In the case of wildcards, we check to ensure that the fields
     * actually match those values - thus, a "wildcard" in this
     * function does not actually stand for a wildcard value, but
     * rather a specific value - UNLESS the CMP_WILD bitmask value
     * is set
     */

    /* check job id */
    if (SCON_NS_CMP_JOB & fields) {
        if (SCON_NS_CMP_WILD & fields &&
            ((0 == strcmp(name1->job_name, SCON_JOBNAME_WILDCARD))||
             (0 == strcmp(name1->job_name, SCON_JOBNAME_WILDCARD)))) {
            goto check_rank;
        }
        if (0 != strcmp(name1->job_name, name2->job_name))
            return SCON_NOT_EQUAL;

    }
    /* get here if jobid's are equal, or not being checked
     * now check vpid
     */
 check_rank:
    if (SCON_NS_CMP_RANK & fields) {
        if (SCON_NS_CMP_WILD & fields &&
            (SCON_RANK_WILDCARD == name1->rank ||
             SCON_RANK_WILDCARD == name2->rank)) {
            return SCON_EQUAL;
        }
        if (name1->rank < name2->rank) {
            return SCON_VALUE2_GREATER;
        } else if (name1->rank > name2->rank) {
            return SCON_VALUE1_GREATER;
        }
    }

    /* only way to get here is if all fields are being checked and are equal,
    * or jobid not checked, but vpid equal,
    * only vpid being checked, and equal
    * return that fact
    */
    return SCON_EQUAL;
}

int scon_util_convert_string_to_process_name(scon_proc_t *name,
                                             const char* name_string)
{
    char *temp, *token;
    unsigned int rank;
    /* check for NULL string - error */
    if (NULL == name_string) {
        SCON_ERROR_LOG(SCON_ERR_BAD_PARAM);
        return SCON_ERR_BAD_PARAM;
    }

    temp = strdup(name_string);  /** copy input string as the strtok process is destructive */
    token = strtok(temp, SCON_SCHEMA_DELIMITER_CHAR); /** get first field -> namespace */

    /* check for error */
    if (NULL == token) {
        SCON_ERROR_LOG(SCON_ERR_BAD_PARAM);
        free(temp);
        return SCON_ERR_BAD_PARAM;
    }
    /* copy namespace */
    strncpy(name->job_name, token, SCON_MAX_JOBLEN);
    /* copy rank */
    token = strtok(NULL, SCON_SCHEMA_DELIMITER_CHAR);
    if (NULL == token) {
        SCON_ERROR_LOG(SCON_ERR_BAD_PARAM);
        free(temp);
        return SCON_ERR_BAD_PARAM;
    }
    name->rank = strtoul(token, NULL, 10);
    free(temp);
    return SCON_SUCCESS;
}

int scon_util_convert_process_name_to_string(char **name_string,
                                        const scon_proc_t* name)
{
    char *tmp;

    if (NULL == name) { /* got an error */
        SCON_ERROR_LOG(SCON_ERR_BAD_PARAM);
        return SCON_ERR_BAD_PARAM;
    }
    asprintf(&tmp, "%s%c%lu", name->job_name, SCON_SCHEMA_DELIMITER_CHAR, (unsigned long)name->rank);
    asprintf(name_string, "%s", tmp);

    free(tmp);
    return SCON_SUCCESS;
}
