/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef SCON_PDL_BASE_H
#define SCON_PDL_BASE_H

#include <src/include/scon_config.h>
#include "src/mca/pdl/pdl.h"
#include "src/util/scon_environ.h"

#include "src/mca/base/base.h"


BEGIN_C_DECLS

/**
 * Globals
 */
extern scon_mca_base_framework_t scon_pdl_base_framework;
extern scon_pdl_base_component_t
*scon_pdl_base_selected_component;
extern scon_pdl_base_module_t *scon_pdl;


/**
 * Initialize the PDL MCA framework
 *
 * @retval SCON_SUCCESS Upon success
 * @retval SCON_ERROR   Upon failures
 *
 * This function is invoked during scon_init();
 */
int scon_pdl_base_open(scon_mca_base_open_flag_t flags);

/**
 * Select an available component.
 *
 * @retval SCON_SUCCESS Upon Success
 * @retval SCON_NOT_FOUND If no component can be selected
 * @retval SCON_ERROR Upon other failure
 *
 */
int scon_pdl_base_select(void);

/**
 * Finalize the PDL MCA framework
 *
 * @retval SCON_SUCCESS Upon success
 * @retval SCON_ERROR   Upon failures
 *
 * This function is invoked during scon_finalize();
 */
int scon_pdl_base_close(void);

/**
 * Open a DSO
 *
 * (see scon_pdl_base_module_open_ft_t in scon/mca/pdl/pdl.h for
 * documentation of this function)
 */
int scon_pdl_open(const char *fname,
                 bool use_ext, bool private_namespace,
                 scon_pdl_handle_t **handle, char **err_msg);

/**
 * Lookup a symbol in a DSO
 *
 * (see scon_pdl_base_module_lookup_ft_t in scon/mca/pdl/pdl.h for
 * documentation of this function)
 */
int scon_pdl_lookup(scon_pdl_handle_t *handle,
                   const char *symbol,
                   void **ptr, char **err_msg);

/**
 * Close a DSO
 *
 * (see scon_pdl_base_module_close_ft_t in scon/mca/pdl/pdl.h for
 * documentation of this function)
 */
int scon_pdl_close(scon_pdl_handle_t *handle);

/**
 * Iterate over files in a path
 *
 * (see scon_pdl_base_module_foreachfile_ft_t in scon/mca/pdl/pdl.h for
 * documentation of this function)
 */
int scon_pdl_foreachfile(const char *search_path,
                        int (*cb_func)(const char *filename, void *context),
                        void *context);

END_C_DECLS

#endif /* SCON_PDL_BASE_H */
