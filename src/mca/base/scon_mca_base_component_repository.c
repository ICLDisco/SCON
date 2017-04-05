/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
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
 * Copyright (c) 2008-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */


#include <src/include/scon_config.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/class/scon_list.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/base/scon_mca_base_component_repository.h"
#include "src/mca/sdl/base/base.h"
#include "scon_common.h"
#include "src/class/scon_hash_table.h"
#include "src/util/basename.h"

#if SCON_HAVE_SDL_SUPPORT

/*
 * Private types
 */
static void ri_constructor(scon_mca_base_component_repository_item_t *ri);
static void ri_destructor(scon_mca_base_component_repository_item_t *ri);
SCON_CLASS_INSTANCE(scon_mca_base_component_repository_item_t, scon_list_item_t,
                    ri_constructor, ri_destructor);

#endif /* SCON_HAVE_SDL_SUPPORT */


/*
 * Private variables
 */
static bool initialized = false;


#if SCON_HAVE_SDL_SUPPORT

static scon_hash_table_t scon_mca_base_component_repository;

/* two-level macro for stringifying a number */
#define STRINGIFYX(x) #x
#define STRINGIFY(x) STRINGIFYX(x)

static int process_repository_item (const char *filename, void *data)
{
    char name[SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN + 1];
    char type[SCON_MCA_BASE_MAX_TYPE_NAME_LEN + 1];
    scon_mca_base_component_repository_item_t *ri;
    scon_list_t *component_list;
    char *base;
    int ret;

    base = scon_basename (filename);
    if (NULL == base) {
        return SCON_ERROR;
    }

    /* check if the plugin has the appropriate prefix */
    if (0 != strncmp (base, "mca_", 4)) {
        free (base);
        return SCON_SUCCESS;
    }

    /* read framework and component names. framework names may not include an _
     * but component names may */
    ret = sscanf(base, "mca_%" STRINGIFY(SCON_MCA_BASE_MAX_TYPE_NAME_LEN) "[^_]_%"
                 STRINGIFY(SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN) "s", type, name);
    if (0 > ret) {
        /* does not patch the expected template. skip */
        free(base);
        return SCON_SUCCESS;
    }

    /* lookup the associated framework list and create if it doesn't already exist */
    ret = scon_hash_table_get_value_ptr(&scon_mca_base_component_repository, type,
                                        strlen (type), (void **) &component_list);
    if (SCON_SUCCESS != ret) {
        component_list = SCON_NEW(scon_list_t);
        if (NULL == component_list) {
            free (base);
            /* OOM. nothing to do but fail */
            return SCON_ERR_OUT_OF_RESOURCE;
        }

        ret = scon_hash_table_set_value_ptr(&scon_mca_base_component_repository, type,
                                            strlen (type), (void *) component_list);
        if (SCON_SUCCESS != ret) {
            free (base);
            SCON_RELEASE(component_list);
            return ret;
        }
    }

    /* check for duplicate components */
    SCON_LIST_FOREACH(ri, component_list, scon_mca_base_component_repository_item_t) {
        if (0 == strcmp (ri->ri_name, name)) {
            /* already scanned this component */
            free (base);
            return SCON_SUCCESS;
        }
    }

    ri = SCON_NEW(scon_mca_base_component_repository_item_t);
    if (NULL == ri) {
        free (base);
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    ri->ri_base = base;

    ri->ri_path = strdup (filename);
    if (NULL == ri->ri_path) {
        SCON_RELEASE(ri);
        return SCON_ERR_OUT_OF_RESOURCE;
    }

    /* strncpy does not guarantee a \0 */
    ri->ri_type[SCON_MCA_BASE_MAX_TYPE_NAME_LEN] = '\0';
    strncpy (ri->ri_type, type, SCON_MCA_BASE_MAX_TYPE_NAME_LEN);

    ri->ri_name[SCON_MCA_BASE_MAX_TYPE_NAME_LEN] = '\0';
    strncpy (ri->ri_name, name, SCON_MCA_BASE_MAX_COMPONENT_NAME_LEN);

    scon_list_append (component_list, &ri->super);

    return SCON_SUCCESS;
}

static int file_exists(const char *filename, const char *ext)
{
    char *final;
    int ret;

    if (NULL == ext) {
        return access (filename, F_OK) == 0;
    }

    ret = asprintf(&final, "%s.%s", filename, ext);
    if (0 > ret || NULL == final) {
        return 0;
    }

    ret = access (final, F_OK);
    free(final);
    return (0 == ret);
}

#endif /* SCON_HAVE_SDL_SUPPORT */

int scon_mca_base_component_repository_add (const char *path)
{
#if SCON_HAVE_SDL_SUPPORT
    char *path_to_use = NULL, *dir, *ctx;
    const char sep[] = {SCON_ENV_SEP, '\0'};

    if (NULL == path) {
        /* nothing to do */
        return SCON_SUCCESS;
    }

    path_to_use = strdup (path);

    dir = strtok_r (path_to_use, sep, &ctx);
    do {
        if ((0 == strcmp(dir, "USER_DEFAULT") || 0 == strcmp(dir, "USR_DEFAULT"))
            && NULL != scon_mca_base_user_default_path) {
            dir = scon_mca_base_user_default_path;
        } else if (0 == strcmp(dir, "SYS_DEFAULT") ||
                   0 == strcmp(dir, "SYSTEM_DEFAULT")) {
            dir = scon_mca_base_system_default_path;
        }

        if (0 != scon_sdl_foreachfile(dir, process_repository_item, NULL)) {
            break;
        }
    } while (NULL != (dir = strtok_r (NULL, sep, &ctx)));

    free (path_to_use);

#endif /* SCON_HAVE_SDL_SUPPORT */

    return SCON_SUCCESS;
}


/*
 * Initialize the repository
 */
int scon_mca_base_component_repository_init(void)
{
  /* Setup internal structures */

  if (!initialized) {
#if SCON_HAVE_SDL_SUPPORT

    /* Initialize the dl framework */
    int ret = scon_mca_base_framework_open(&scon_sdl_base_framework, 0);
    if (SCON_SUCCESS != ret) {
        scon_output(0, "%s %d:%s failed -- process will likely abort (open the dl framework returned %d instead of SCON_SUCCESS)\n",
                    __FILE__, __LINE__, __func__, ret);
        return ret;
    }
    scon_sdl_base_select();

    SCON_CONSTRUCT(&scon_mca_base_component_repository, scon_hash_table_t);
    ret = scon_hash_table_init (&scon_mca_base_component_repository, 128);
    if (SCON_SUCCESS != ret) {
        (void) scon_mca_base_framework_close(&scon_sdl_base_framework);
        return ret;
    }

    ret = scon_mca_base_component_repository_add(scon_mca_base_component_path);
    if (SCON_SUCCESS != ret) {
        SCON_DESTRUCT(&scon_mca_base_component_repository);
        (void) scon_mca_base_framework_close(&scon_sdl_base_framework);
        return ret;
    }
#endif

    initialized = true;
  }

  /* All done */

  return SCON_SUCCESS;
}

int scon_mca_base_component_repository_get_components (scon_mca_base_framework_t *framework,
                                                       scon_list_t **framework_components)
{
    *framework_components = NULL;
#if SCON_HAVE_SDL_SUPPORT
    return scon_hash_table_get_value_ptr (&scon_mca_base_component_repository, framework->framework_name,
                                          strlen (framework->framework_name), (void **) framework_components);
#endif
    return SCON_ERR_NOT_FOUND;
}

#if SCON_HAVE_SDL_SUPPORT
static void scon_mca_base_component_repository_release_internal(scon_mca_base_component_repository_item_t *ri) {
    int group_id;

    group_id = scon_mca_base_var_group_find (NULL, ri->ri_type, ri->ri_name);
    if (0 <= group_id) {
        /* ensure all variables are deregistered before we dlclose the component */
        scon_mca_base_var_group_deregister (group_id);
    }

    /* Close the component (and potentially unload it from memory */
    if (ri->ri_dlhandle) {
        scon_sdl_close(ri->ri_dlhandle);
        ri->ri_dlhandle = NULL;
    }
}
#endif

#if SCON_HAVE_SDL_SUPPORT
static scon_mca_base_component_repository_item_t *find_component(const char *type, const char *name)
{
    scon_mca_base_component_repository_item_t *ri;
    scon_list_t *component_list;
    int ret;

    ret = scon_hash_table_get_value_ptr (&scon_mca_base_component_repository, type,
                                         strlen (type), (void **) &component_list);
    if (SCON_SUCCESS != ret) {
        /* component does not exist in the repository */
        return NULL;
    }

    SCON_LIST_FOREACH(ri, component_list, scon_mca_base_component_repository_item_t) {
        if (0 == strcmp (ri->ri_name, name)) {
            return ri;
        }
    }

    return NULL;
}
#endif

void scon_mca_base_component_repository_release(const scon_mca_base_component_t *component)
{
#if SCON_HAVE_SDL_SUPPORT
    scon_mca_base_component_repository_item_t *ri;

    ri = find_component (component->scon_mca_type_name, component->scon_mca_component_name);
    if (NULL != ri && !(--ri->ri_refcnt)) {
        scon_mca_base_component_repository_release_internal (ri);
    }
#endif
}

int scon_mca_base_component_repository_retain_component(const char *type, const char *name)
{
#if SCON_HAVE_SDL_SUPPORT
    scon_mca_base_component_repository_item_t *ri = find_component(type, name);

    if (NULL != ri) {
        ++ri->ri_refcnt;
        return SCON_SUCCESS;
    }

    return SCON_ERR_NOT_FOUND;
#else
    return SCON_ERR_NOT_SUPPORTED;
#endif
}

int scon_mca_base_component_repository_open(scon_mca_base_framework_t *framework,
                                            scon_mca_base_component_repository_item_t *ri)
{
#if SCON_HAVE_SDL_SUPPORT
    scon_mca_base_component_t *component_struct;
    scon_mca_base_component_list_item_t *mitem = NULL;
    char *struct_name = NULL;
    int vl, ret;

    scon_output_verbose(SCON_MCA_BASE_VERBOSE_INFO, 0, "scon_mca_base_component_repository_open: examining dynamic "
                        "%s MCA component \"%s\" at path %s", ri->ri_type, ri->ri_name, ri->ri_path);

    vl = scon_mca_base_component_show_load_errors ? SCON_MCA_BASE_VERBOSE_ERROR : SCON_MCA_BASE_VERBOSE_INFO;

    /* Ensure that this component is not already loaded (should only happen
       if it was statically loaded).  It's an error if it's already
       loaded because we're evaluating this file -- not this component.
       Hence, returning SCON_ERR_PARAM indicates that the *file* failed
       to load, not the component. */

    SCON_LIST_FOREACH(mitem, &framework->framework_components, scon_mca_base_component_list_item_t) {
        if (0 == strcmp(mitem->cli_component->scon_mca_component_name, ri->ri_name)) {
            scon_output_verbose (SCON_MCA_BASE_VERBOSE_INFO, 0, "scon_mca_base_component_repository_open: already loaded (ignored)");
            return SCON_ERR_BAD_PARAM;
        }
    }

    /* silence coverity issue (invalid free) */
    mitem = NULL;

    if (NULL != ri->ri_dlhandle) {
        scon_output_verbose (SCON_MCA_BASE_VERBOSE_INFO, 0, "scon_mca_base_component_repository_open: already loaded. returning cached component");
        mitem = SCON_NEW(scon_mca_base_component_list_item_t);
        if (NULL == mitem) {
            return SCON_ERR_OUT_OF_RESOURCE;
        }

        mitem->cli_component = ri->ri_component_struct;
        scon_list_append (&framework->framework_components, &mitem->super);

        return SCON_SUCCESS;
    }

    if (0 != strcmp (ri->ri_type, framework->framework_name)) {
        /* shouldn't happen. attempting to open a component belonging to
         * another framework. if this happens it is likely a MCA base
         * bug so assert */
        assert (0);
        return SCON_ERR_NOT_SUPPORTED;
    }

    /* Now try to load the component */

    char *err_msg = NULL;
    if (SCON_SUCCESS != scon_sdl_open(ri->ri_path, true, false, &ri->ri_dlhandle, &err_msg)) {
        if (NULL == err_msg) {
            err_msg = "scon_dl_open() error message was NULL!";
        }
        /* Because libltdl erroneously says "file not found" for any
           type of error -- which is especially misleading when the file
           is actually there but cannot be opened for some other reason
           (e.g., missing symbol) -- do some simple huersitics and if
           the file [probably] does exist, print a slightly better error
           message. */
        if (0 == strcasecmp("file not found", err_msg) &&
            (file_exists(ri->ri_path, "lo") ||
             file_exists(ri->ri_path, "so") ||
             file_exists(ri->ri_path, "dylib") ||
             file_exists(ri->ri_path, "dll"))) {
            err_msg = "perhaps a missing symbol, or compiled for a different version of Open MPI?";
        }
        scon_output_verbose(vl, 0, "scon_mca_base_component_repository_open: unable to open %s: %s (ignored)",
                            ri->ri_base, err_msg);
        return SCON_ERR_BAD_PARAM;
    }

    /* Successfully opened the component; now find the public struct.
       Malloc out enough space for it. */

    do {
        ret = asprintf (&struct_name, "mca_%s_%s_component", ri->ri_type, ri->ri_name);
        if (0 > ret) {
            ret = SCON_ERR_OUT_OF_RESOURCE;
            break;
        }

        mitem = SCON_NEW(scon_mca_base_component_list_item_t);
        if (NULL == mitem) {
            ret = SCON_ERR_OUT_OF_RESOURCE;
            break;
        }

        err_msg = NULL;
        ret = scon_sdl_lookup(ri->ri_dlhandle, struct_name, (void**) &component_struct, &err_msg);
        if (SCON_SUCCESS != ret || NULL == component_struct) {
            if (NULL == err_msg) {
                err_msg = "scon_dl_loookup() error message was NULL!";
            }
            scon_output_verbose(vl, 0, "scon_mca_base_component_repository_open: \"%s\" does not appear to be a valid "
                                "%s MCA dynamic component (ignored): %s. ret %d", ri->ri_base, ri->ri_type, err_msg, ret);

            ret = SCON_ERR_BAD_PARAM;
            break;
        }

        /* done with the structure name */
        free (struct_name);
        struct_name = NULL;

        /* We found the public struct.  Make sure its MCA major.minor
           version is the same as ours. TODO -- add checks for project version (from framework) */
        if (!(SCON_MCA_BASE_VERSION_MAJOR == component_struct->scon_mca_major_version &&
              SCON_MCA_BASE_VERSION_MINOR == component_struct->scon_mca_minor_version)) {
            scon_output_verbose(vl, 0, "scon_mca_base_component_repository_open: %s \"%s\" uses an MCA interface that is "
                                "not recognized (component MCA v%d.%d.%d != supported MCA v%d.%d.%d) -- ignored",
                                ri->ri_type, ri->ri_path, component_struct->scon_mca_major_version,
                                component_struct->scon_mca_minor_version, component_struct->scon_mca_release_version,
                                SCON_MCA_BASE_VERSION_MAJOR, SCON_MCA_BASE_VERSION_MINOR, SCON_MCA_BASE_VERSION_RELEASE);
            ret = SCON_ERR_BAD_PARAM;
            break;
        }

        /* Also check that the component struct framework and component
           names match the expected names from the filename */
        if (0 != strcmp(component_struct->scon_mca_type_name, ri->ri_type) ||
            0 != strcmp(component_struct->scon_mca_component_name, ri->ri_name)) {
            scon_output_verbose(vl, 0, "Component file data does not match filename: %s (%s / %s) != %s %s -- ignored",
                                ri->ri_path, ri->ri_type, ri->ri_name,
                                component_struct->scon_mca_type_name,
                                component_struct->scon_mca_component_name);
            ret = SCON_ERR_BAD_PARAM;
            break;
        }

        /* Alles gut.  Save the component struct, and register this
           component to be closed later. */

        ri->ri_component_struct = mitem->cli_component = component_struct;
        ri->ri_refcnt = 1;
        scon_list_append(&framework->framework_components, &mitem->super);

        scon_output_verbose (SCON_MCA_BASE_VERBOSE_INFO, 0, "scon_mca_base_component_repository_open: opened dynamic %s MCA "
                             "component \"%s\"", ri->ri_type, ri->ri_name);

        return SCON_SUCCESS;
    } while (0);

    if (mitem) {
        SCON_RELEASE(mitem);
    }

    if (struct_name) {
        free (struct_name);
    }

    scon_sdl_close (ri->ri_dlhandle);
    ri->ri_dlhandle = NULL;

    return ret;
#else

    /* no dlopen support */
    return SCON_ERR_NOT_SUPPORTED;
#endif
}

/*
 * Finalize the repository -- close everything that's still open.
 */
void scon_mca_base_component_repository_finalize(void)
{
    if (!initialized) {
        return;
    }

    initialized = false;

#if SCON_HAVE_SDL_SUPPORT
    scon_list_t *component_list;
    void *node, *key;
    size_t key_size;
    int ret;

    ret = scon_hash_table_get_first_key_ptr (&scon_mca_base_component_repository, &key, &key_size,
                                             (void **) &component_list, &node);
    while (SCON_SUCCESS == ret) {
        SCON_LIST_RELEASE(component_list);
        ret = scon_hash_table_get_next_key_ptr (&scon_mca_base_component_repository, &key,
                                                &key_size, (void **) &component_list,
                                                node, &node);
    }

    (void) scon_mca_base_framework_close(&scon_sdl_base_framework);
    SCON_DESTRUCT(&scon_mca_base_component_repository);
#endif
}

#if SCON_HAVE_SDL_SUPPORT

/*
 * Basic sentinel values, and construct the inner list
 */
static void ri_constructor (scon_mca_base_component_repository_item_t *ri)
{
    memset(ri->ri_type, 0, sizeof(ri->ri_type));
    ri->ri_dlhandle = NULL;
    ri->ri_component_struct = NULL;
    ri->ri_path = NULL;
}


/*
 * Close a component
 */
static void ri_destructor (scon_mca_base_component_repository_item_t *ri)
{
    /* dlclose the component if it is still open */
    scon_mca_base_component_repository_release_internal (ri);

    /* It should be obvious, but I'll state it anyway because it bit me
       during debugging: after the dlclose(), the scon_mca_base_component_t
       pointer is no longer valid because it has [potentially] been
       unloaded from memory.  So don't try to use it.  :-) */

    if (ri->ri_path) {
        free (ri->ri_path);
    }

    if (ri->ri_base) {
        free (ri->ri_base);
    }
}

#endif /* SCON_HAVE_SDL_SUPPORT */
