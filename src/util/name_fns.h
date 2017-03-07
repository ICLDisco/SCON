/*
 * Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
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
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file:
 *
 * Populates global structure with system-specific information.
 *
 * Notes: add limits.h, compute size of integer and other types via sizeof(type)*CHAR_BIT
 *
 */

#ifndef _SCON_NAME_FNS_H_
#define _SCON_NAME_FNS_H_

#include "scon_config.h"

#ifdef HAVE_STDINT_h
#include <stdint.h>
#endif

#include "scon_types.h"
#include "scon_globals.h"
#include "src/class/scon_list.h"

BEGIN_C_DECLS

typedef uint8_t  scon_ns_cmp_bitmask_t;  /**< Bit mask for comparing process names */
#define SCON_NS_CMP_NONE       0x00
#define SCON_NS_CMP_JOB        0x02
#define SCON_NS_CMP_RANK       0x04
#define SCON_NS_CMP_ALL        0x0f
#define SCON_NS_CMP_WILD       0x10

/* useful define to print name args in output messages */
extern char* scon_util_print_name_args(const scon_proc_t *name);
#define SCON_PRINT_PROC(n) \
    scon_util_print_name_args(n)

int scon_util_compare_name_fields(scon_ns_cmp_bitmask_t fields,
                                  const scon_proc_t* name1,
                                  const scon_proc_t* name2);
/** This funtion returns a guaranteed unique hash value for the passed process name */
uint32_t scon_util_hash_rank(int32_t rank);

int scon_util_convert_process_name_to_string(char **name_string,
        const scon_proc_t* name);
int scon_util_convert_string_to_process_name(scon_proc_t *name,
        const char* name_string);
END_C_DECLS
#endif
