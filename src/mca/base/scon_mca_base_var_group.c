/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2012 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/scon_config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <errno.h>

#include "src/include/scon_stdint.h"
#include "src/util/show_help.h"
#include "src/mca/mca.h"
#include "src/mca/base/scon_mca_base_vari.h"
#include "scon_common.h"
#include "src/util/output.h"
#include "src/util/scon_environ.h"

static scon_pointer_array_t scon_mca_base_var_groups;
static scon_hash_table_t scon_mca_base_var_group_index_hash;
static int scon_mca_base_var_group_count = 0;
static int scon_mca_base_var_groups_timestamp = 0;
static bool scon_mca_base_var_group_initialized = false;

static void scon_mca_base_var_group_constructor (scon_mca_base_var_group_t *group);
static void scon_mca_base_var_group_destructor (scon_mca_base_var_group_t *group);
SCON_CLASS_INSTANCE(scon_mca_base_var_group_t, scon_object_t,
                    scon_mca_base_var_group_constructor,
                    scon_mca_base_var_group_destructor);

int scon_mca_base_var_group_init (void)
{
    int ret;

    if (!scon_mca_base_var_group_initialized) {
        SCON_CONSTRUCT(&scon_mca_base_var_groups, scon_pointer_array_t);

        /* These values are arbitrary */
        ret = scon_pointer_array_init (&scon_mca_base_var_groups, 128, 16384, 128);
        if (SCON_SUCCESS != ret) {
            return ret;
        }

        SCON_CONSTRUCT(&scon_mca_base_var_group_index_hash, scon_hash_table_t);
        ret = scon_hash_table_init (&scon_mca_base_var_group_index_hash, 256);
        if (SCON_SUCCESS != ret) {
            return ret;
        }

        scon_mca_base_var_group_initialized = true;
        scon_mca_base_var_group_count = 0;
    }

    return SCON_SUCCESS;
}

int scon_mca_base_var_group_finalize (void)
{
    scon_object_t *object;
    int size, i;

    if (scon_mca_base_var_group_initialized) {
        size = scon_pointer_array_get_size(&scon_mca_base_var_groups);
        for (i = 0 ; i < size ; ++i) {
            object = scon_pointer_array_get_item (&scon_mca_base_var_groups, i);
            if (NULL != object) {
                SCON_RELEASE(object);
            }
        }
        SCON_DESTRUCT(&scon_mca_base_var_groups);
        SCON_DESTRUCT(&scon_mca_base_var_group_index_hash);
        scon_mca_base_var_group_count = 0;
        scon_mca_base_var_group_initialized = false;
    }

    return SCON_SUCCESS;
}

int scon_mca_base_var_group_get_internal (const int group_index, scon_mca_base_var_group_t **group, bool invalidok)
{
    if (group_index < 0) {
        return SCON_ERR_NOT_FOUND;
    }

    *group = (scon_mca_base_var_group_t *) scon_pointer_array_get_item (&scon_mca_base_var_groups,
                                                                   group_index);
    if (NULL == *group || (!invalidok && !(*group)->group_isvalid)) {
        *group = NULL;
        return SCON_ERR_NOT_FOUND;
    }

    return SCON_SUCCESS;
}

static int group_find_by_name (const char *full_name, int *index, bool invalidok)
{
    scon_mca_base_var_group_t *group;
    void *tmp;
    int rc;

    rc = scon_hash_table_get_value_ptr (&scon_mca_base_var_group_index_hash, full_name,
                                        strlen (full_name), &tmp);
    if (SCON_SUCCESS != rc) {
        return rc;
    }

    rc = scon_mca_base_var_group_get_internal ((int)(uintptr_t) tmp, &group, invalidok);
    if (SCON_SUCCESS != rc) {
        return rc;
    }

    if (invalidok || group->group_isvalid) {
        *index = (int)(uintptr_t) tmp;
        return SCON_SUCCESS;
    }

    return SCON_ERR_NOT_FOUND;
}

static bool compare_strings (const char *str1, const char *str2) {
    if ((NULL != str1 && 0 == strcmp (str1, "*")) ||
        (NULL == str1 && NULL == str2)) {
        return true;
    }

    if (NULL != str1 && NULL != str2) {
        return 0 == strcmp (str1, str2);
    }

    return false;
}

static int group_find_linear (const char *project_name, const char *framework_name,
                              const char *component_name, bool invalidok)
{
    for (int i = 0 ; i < scon_mca_base_var_group_count ; ++i) {
        scon_mca_base_var_group_t *group;

        int rc = scon_mca_base_var_group_get_internal (i, &group, invalidok);
        if (SCON_SUCCESS != rc) {
            continue;
        }

        if (compare_strings (project_name, group->group_project) &&
            compare_strings (framework_name, group->group_framework) &&
            compare_strings (component_name, group->group_component)) {
            return i;
        }
    }

    return SCON_ERR_NOT_FOUND;
}

static int group_find (const char *project_name, const char *framework_name,
                       const char *component_name, bool invalidok)
{
    char *full_name;
    int ret, index=0;

    if (!scon_mca_base_var_initialized) {
        return SCON_ERR_NOT_FOUND;
    }

    /* check for wildcards */
    if ((project_name && '*' == project_name[0]) || (framework_name && '*' == framework_name[0]) ||
        (component_name && '*' == component_name[0])) {
        return group_find_linear (project_name, framework_name, component_name, invalidok);
    }

    ret = scon_mca_base_var_generate_full_name4(project_name, framework_name, component_name,
                                           NULL, &full_name);
    if (SCON_SUCCESS != ret) {
        return SCON_ERROR;
    }

    ret = group_find_by_name(full_name, &index, invalidok);
    free (full_name);

    return (0 > ret) ? ret : index;
}

static int group_register (const char *project_name, const char *framework_name,
                           const char *component_name, const char *description)
{
    scon_mca_base_var_group_t *group;
    int group_id, parent_id = -1;
    int ret;

    if (NULL == project_name && NULL == framework_name && NULL == component_name) {
        /* don't create a group with no name (maybe we should create a generic group?) */
        return -1;
    }

    /* avoid groups of the form scon_scon, etc */
    if (NULL != project_name && NULL != framework_name &&
        (0 == strcmp (project_name, framework_name))) {
        project_name = NULL;
    }

    group_id = group_find (project_name, framework_name, component_name, true);
    if (0 <= group_id) {
        ret = scon_mca_base_var_group_get_internal (group_id, &group, true);
        if (SCON_SUCCESS != ret) {
            /* something went horribly wrong */
            assert (NULL != group);
            return ret;
        }
        group->group_isvalid = true;
        scon_mca_base_var_groups_timestamp++;

        /* group already exists. return it's index */
        return group_id;
    }

    group = SCON_NEW(scon_mca_base_var_group_t);

    group->group_isvalid = true;

    if (NULL != project_name) {
        group->group_project = strdup (project_name);
        if (NULL == group->group_project) {
            SCON_RELEASE(group);
            return SCON_ERR_OUT_OF_RESOURCE;
        }
    }
    if (NULL != framework_name) {
        group->group_framework = strdup (framework_name);
        if (NULL == group->group_framework) {
            SCON_RELEASE(group);
            return SCON_ERR_OUT_OF_RESOURCE;
        }
    }
    if (NULL != component_name) {
        group->group_component = strdup (component_name);
        if (NULL == group->group_component) {
            SCON_RELEASE(group);
            return SCON_ERR_OUT_OF_RESOURCE;
        }
    }
    if (NULL != description) {
        group->group_description = strdup (description);
        if (NULL == group->group_description) {
            SCON_RELEASE(group);
            return SCON_ERR_OUT_OF_RESOURCE;
        }
    }

    if (NULL != framework_name && NULL != component_name) {
        if (component_name) {
            parent_id = group_register (project_name, framework_name, NULL, NULL);
        } else if (framework_name && project_name) {
            parent_id = group_register (project_name, NULL, NULL, NULL);
        }
    }

    /* build the group name */
    ret = scon_mca_base_var_generate_full_name4 (NULL, project_name, framework_name, component_name,
                                            &group->group_full_name);
    if (SCON_SUCCESS != ret) {
        SCON_RELEASE(group);
        return ret;
    }

    group_id = scon_pointer_array_add (&scon_mca_base_var_groups, group);
    if (0 > group_id) {
        SCON_RELEASE(group);
        return SCON_ERROR;
    }

    scon_hash_table_set_value_ptr (&scon_mca_base_var_group_index_hash, group->group_full_name,
                                   strlen (group->group_full_name), (void *)(uintptr_t) group_id);

    scon_mca_base_var_group_count++;
    scon_mca_base_var_groups_timestamp++;

    if (0 <= parent_id) {
        scon_mca_base_var_group_t *parent_group;

        (void) scon_mca_base_var_group_get_internal(parent_id, &parent_group, false);
        scon_value_array_append_item (&parent_group->group_subgroups, &group_id);
    }

    return group_id;
}

int scon_mca_base_var_group_register (const char *project_name, const char *framework_name,
                                      const char *component_name, const char *description)
{
    return group_register (project_name, framework_name, component_name, description);
}

int scon_mca_base_var_group_component_register (const scon_mca_base_component_t *component,
                                                const char *description)
{
    /* 1.7 components do not store the project */
    return group_register (NULL, component->scon_mca_type_name,
                           component->scon_mca_component_name, description);
}


int scon_mca_base_var_group_deregister (int group_index)
{
    scon_mca_base_var_group_t *group;
    int size, ret;
    int *params, *subgroups;

    ret = scon_mca_base_var_group_get_internal (group_index, &group, false);
    if (SCON_SUCCESS != ret) {
        return ret;
    }

    group->group_isvalid = false;

    /* deregister all associated mca parameters */
    size = scon_value_array_get_size(&group->group_vars);
    params = SCON_VALUE_ARRAY_GET_BASE(&group->group_vars, int);

    for (int i = 0 ; i < size ; ++i) {
        const scon_mca_base_var_t *var;

        ret = scon_mca_base_var_get (params[i], &var);
        if (SCON_SUCCESS != ret || !(var->mbv_flags & SCON_MCA_BASE_VAR_FLAG_DWG)) {
            continue;
        }

        (void) scon_mca_base_var_deregister (params[i]);
    }

    size = scon_value_array_get_size(&group->group_subgroups);
    subgroups = SCON_VALUE_ARRAY_GET_BASE(&group->group_subgroups, int);
    for (int i = 0 ; i < size ; ++i) {
        (void) scon_mca_base_var_group_deregister (subgroups[i]);
    }
    /* ordering of variables and subgroups must be the same if the
     * group is re-registered */

    scon_mca_base_var_groups_timestamp++;

    return SCON_SUCCESS;
}

int scon_mca_base_var_group_find (const char *project_name,
                             const char *framework_name,
                             const char *component_name)
{
    return group_find (project_name, framework_name, component_name, false);
}

int scon_mca_base_var_group_find_by_name (const char *full_name, int *index)
{
    return group_find_by_name (full_name, index, false);
}

int scon_mca_base_var_group_add_var (const int group_index, const int param_index)
{
    scon_mca_base_var_group_t *group;
    int size, i, ret;
    int *params;

    ret = scon_mca_base_var_group_get_internal (group_index, &group, false);
    if (SCON_SUCCESS != ret) {
        return ret;
    }

    size = scon_value_array_get_size(&group->group_vars);
    params = SCON_VALUE_ARRAY_GET_BASE(&group->group_vars, int);
    for (i = 0 ; i < size ; ++i) {
        if (params[i] == param_index) {
            return i;
        }
    }

    if (SCON_SUCCESS !=
        (ret = scon_value_array_append_item (&group->group_vars, &param_index))) {
        return ret;
    }

    scon_mca_base_var_groups_timestamp++;

    /* return the group index */
    return (int) scon_value_array_get_size (&group->group_vars) - 1;
}

int scon_mca_base_var_group_get (const int group_index, const scon_mca_base_var_group_t **group)
{
    return scon_mca_base_var_group_get_internal (group_index, (scon_mca_base_var_group_t **) group, false);
}

int scon_mca_base_var_group_set_var_flag (const int group_index, int flags, bool set)
{
    scon_mca_base_var_group_t *group;
    int size, i, ret;
    int *vars;

    ret = scon_mca_base_var_group_get_internal (group_index, &group, false);
    if (SCON_SUCCESS != ret) {
        return ret;
    }

    /* set the flag on each valid variable */
    size = scon_value_array_get_size(&group->group_vars);
    vars = SCON_VALUE_ARRAY_GET_BASE(&group->group_vars, int);

    for (i = 0 ; i < size ; ++i) {
        if (0 <= vars[i]) {
            (void) scon_mca_base_var_set_flag (vars[i], flags, set);
        }
    }

    return SCON_SUCCESS;
}


static void scon_mca_base_var_group_constructor (scon_mca_base_var_group_t *group)
{
    memset ((char *) group + sizeof (group->super), 0, sizeof (*group) - sizeof (group->super));

    SCON_CONSTRUCT(&group->group_subgroups, scon_value_array_t);
    scon_value_array_init (&group->group_subgroups, sizeof (int));

    SCON_CONSTRUCT(&group->group_vars, scon_value_array_t);
    scon_value_array_init (&group->group_vars, sizeof (int));

}

static void scon_mca_base_var_group_destructor (scon_mca_base_var_group_t *group)
{
    free (group->group_full_name);
    group->group_full_name = NULL;

    free (group->group_description);
    group->group_description = NULL;

    free (group->group_project);
    group->group_project = NULL;

    free (group->group_framework);
    group->group_framework = NULL;

    free (group->group_component);
    group->group_component = NULL;

    SCON_DESTRUCT(&group->group_subgroups);
    SCON_DESTRUCT(&group->group_vars);
}

int scon_mca_base_var_group_get_count (void)
{
    return scon_mca_base_var_group_count;
}

int scon_mca_base_var_group_get_stamp (void)
{
    return scon_mca_base_var_groups_timestamp;
}
