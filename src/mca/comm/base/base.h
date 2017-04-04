/*
 * Copyright (c) 2015-2016     Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**
 * @file
 *
 * Common Framework maintenence interface
 *
 *
 *
 */

#ifndef SCON_COMM_BASE_H
#define SCON_COMM_BASE_H

#include <scon_common.h>
#include "src/include/scon_config.h"
#include "src/mca/comm/comm.h"
#include "src/util/error.h"
#include "src/util/name_fns.h"
#include "src/class/scon_list.h"
#include "src/class/scon_object.h"
#include "src/class/scon_pointer_array.h"
#include "src/mca/base/base.h"
#include "src/mca/collectives/collectives.h"
#include "src/mca/topology/topology.h"
#include "src/mca/pt2pt/pt2pt.h"


#define SCON_COMMON_MAX_COMPONENTS 5

#define SCON_PT2PT_DEFAULT_COMPONENT          "tcp"
#define SCON_TOPOLOGY_DEFAULT_COMPONENT       "radixtree"
#define SCON_COLLECTIVES_DEFAULT_COMPONENT    "default"

/* select a component */
int scon_comm_base_select(void);

/* a global struct tracking framework level objects */
typedef struct {
    scon_pointer_array_t scons;
    scon_list_t actives;
} comm_base_t;

/* SCON internal states*/
typedef enum {
    SCON_STATE_CREATING,  /* local scon create complete */
    SCON_STATE_VERIFYING, /* config verified */
    SCON_STATE_WIRING_UP,  /* wiring up SCON */
    SCON_STATE_OPERATIONAL,
    SCON_STATE_ERROR,
    SCON_STATE_DELETING,
}scon_state_t;

/* scon object definition */
typedef struct  {
    scon_list_item_t super;
    /* scon id */
    uint32_t handle;
    /* master proc */
    scon_proc_t master;
    /* num members replied or active */
    uint32_t num_replied;
    /* scon members */
    scon_list_t members;
    /* num members */
    uint32_t nmembers;
    /* scon type */
    scon_type_t type;
    /* scon attributes copied from the info array*/
    scon_info_t *info;
    size_t ninfo;
    scon_state_t state;
    /* ptr to the outstanding send request only
     * the create and teardown requests are tracked here*/
    void  *req;
    /* sends in queue while scon is getting created */
    scon_list_t queued_msgs;
    /* recvs posted on this scon */
    scon_list_t posted_recvs;
    /* recv buffer msg queue size */
    uint16_t recv_queue_len;
    /* unmmatched messages received on this scon */
    scon_list_t unmatched_msgs;
    /* reference to the active pt2pt module for this scon */
    scon_pt2pt_module_t *pt2pt_module;
    /* reference to the active collective module this scon */
    scon_collectives_module_t *collective_module;
    /* reference to the pt2pt module this scon is using */
    scon_topology_module_t *topology_module;
} scon_comm_scon_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_comm_scon_t);

/* scon member */
typedef struct {
    scon_list_item_t super;
    scon_proc_t name;
    scon_handle_t local_handle;
    char *mem_uri;
} scon_member_t;
SCON_CLASS_DECLARATION(scon_member_t);

/* scon create request*/
typedef struct scon_create {
    scon_list_item_t super;
    /*create status */
    int status;
    /* the local scon handle */
    scon_handle_t scon_handle;
    /* user's callback function */
    scon_create_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
    /* the req's infos */
    scon_info_t *info;
    /* number of req infos */
    size_t ninfo;
    /* create request timeout */
    unsigned int timeout;
    /* min members required to be up before
     SCON can become operational */
    unsigned int quorum;
} scon_create_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_create_t);

/* scon delete request */
typedef struct {
    scon_list_item_t super;
    /*teardown status */
    int status;
    /* the local scon handle */
    scon_handle_t scon_handle;
    /* user's callback function */
    scon_op_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
    /* the req's infos */
    scon_info_t *info;
    /* number of req infos */
    size_t ninfo;
} scon_teardown_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_teardown_t);


/* Request object for transfering requests to the event lib */
typedef struct {
    scon_object_t super;
    scon_event_t ev;
    union {
        scon_create_t create;
        scon_teardown_t teardown;
    }post;
} scon_req_t;
SCON_CLASS_DECLARATION(scon_req_t);

/* comm implementations */

extern scon_comm_scon_t* comm_base_create (scon_proc_t procs[],
                                       size_t nprocs,
                                       scon_info_t info[],
                                       size_t ninfo,
                                       scon_req_t *req);

extern scon_handle_t scon_base_get_handle(scon_comm_scon_t *scon,
                                    scon_proc_t *member);

static inline uint32_t get_index (scon_handle_t handle)
{
    if (handle > SCON_HANDLE_INVALID)
        return handle - 1 ;
    else {
        SCON_ERROR_LOG (SCON_ERR_INVALID_HANDLE);
        return SCON_INDEX_UNDEFINED;
    }

}

#define SCON_GET_MASTER(scon)  (&scon->master)
static inline bool is_master(scon_comm_scon_t * scon)
{
    return ((0 == strncmp(scon->master.job_name, SCON_PROC_MY_NAME->job_name, SCON_MAX_JOBLEN)) &&
            (scon->master.rank == SCON_PROC_MY_NAME->rank));
}

scon_status_t comm_base_pack_scon_config(scon_comm_scon_t *scon,
                                           scon_buffer_t *buffer);

scon_status_t comm_base_check_config (scon_comm_scon_t *scon,
                                        scon_buffer_t *buffer);

scon_status_t comm_base_set_scon_config(scon_comm_scon_t *scon,
                                          scon_buffer_t *buffer);

scon_comm_scon_t * scon_comm_base_get_scon (scon_handle_t handle);
void scon_comm_base_add_scon(scon_comm_scon_t *scon);
void scon_comm_base_remove_scon(scon_comm_scon_t *scon);

END_C_DECLS
#endif /* SCON_COMM_BASE_H */
