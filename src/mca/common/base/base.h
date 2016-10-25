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

#ifndef SCON_COMMON_BASE_H
#define SCON_COMMON_BASE_H

#include <scon_config.h>
#include "src/mca/common/common.h"
#include "src/util/error.h"

#include "src/class/scon_list.h"
#include "src/class/scon_object.h"
#include "src/include/types.h"
#include "src/class/scon_pointer_array.h"
#include "src/mca/base/base.h"

#define SCON_COMMON_MAX_COMPONENTS 5

/*
 * MCA Framework
 */
extern scon_mca_base_framework_t scon_common_base_framework;
extern scon_common_base_component_t* scon_common_base_selected_component;
extern scon_common_module_t scon_common;
/* select a component */
int scon_common_base_select(void);

/* a global struct tracking framework level objects */
typedef struct {
    scon_pointer_array_t scons;
    scon_list_t actives;
} scon_common_base_t;
extern scon_common_base_t scon_common_base;

/* max no of scons for a process */
#define MAX_SCONS 10
/* lets make 0 to indicate non existent scon for legacy reasons*/
#define SCON_HANDLE_INVALID 0

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
    uint32_t num_procs;
    /* scon type */
    scon_type_t type;
    /* scon attributes copied from the info array*/
    scon_list_t attributes;
    /* the topology of the scon, this will be
       used to retrieve the routed module associated
       with this scon */
    scon_topo_type_t topo;
    scon_state_t state;
    /* ptr to the outstanding send request only
     * the create and teardown requests are tracked here*/
    void  *req;
    /* sends in queue while scon is getting created */
    scon_list_t queued_msgs;
    /* recvs posted on this scon */
    scon_list_t posted_recvs;
    /* unmmatched messages received on this scon */
    scon_list_t unmatched_msgs;
} scon_common_scon_t;
SCON_CLASS_DECLARATION(scon_common_scon_t);

/* scon member */
typedef struct {
    scon_list_item_t super;
    scon_proc_t name;
    scon_handle_t local_handle;
} scon_member_t;
SCON_CLASS_DECLARATION(scon_member_t);

/* scon create request*/
typedef struct scon_create {
    scon_list_item_t super;
    /*create status */
    int status;
    /* the scon object */
    scon_common_scon_t *scon;
    /* user's callback function */
    scon_create_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
} scon_create_t;
SCON_CLASS_DECLARATION(scon_create_t);

/* scon delete request */
typedef struct {
    scon_list_item_t super;
    /*teardown status */
    int status;
    /* the scon object */
    scon_common_scon_t *scon;
    /* user's callback function */
    scon_op_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
} scon_teardown_t;
SCON_CLASS_DECLARATION(scon_teardown_t);

/* scon send req */
typedef struct {
    scon_list_item_t super;
    /*status */
    int status;
    /*  scon object */
    scon_common_scon_t *scon;
    /* receiver */
    scon_proc_t dst;
    /*  msg buffer */
    scon_buffer_t *buf;
    /* msg tag */
    scon_msg_tag_t tag;
    /* user's callback function */
    scon_send_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
    /* info struct */
    scon_info_t *info;
    /* number of info */
    size_t ninfo;
} scon_send_t;
SCON_CLASS_DECLARATION(scon_send_t);

/* scon xcast req */
typedef struct {
    scon_list_item_t super;
    /*status */
    int status;
    /*  scon object */
    scon_common_scon_t *scon;
    /* recipients */
    scon_proc_t *procs;
    /* num recipients */
    size_t nprocs;
    /*  msg buffer */
    scon_buffer_t *buf;
    /* msg tag */
    scon_msg_tag_t tag;
    /* user's callback function */
    scon_xcast_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
    /* info struct */
    scon_info_t *info;
    /* number of info */
    size_t ninfo;
} scon_xcast_t;
SCON_CLASS_DECLARATION(scon_xcast_t);

/* scon barrier req */
typedef struct {
    scon_list_item_t super;
    /*status */
    int status;
    /*  scon object */
    scon_common_scon_t *scon;
    /* partcipants */
    scon_proc_t *procs;
    /* num participants */
    size_t nprocs;
    /* user's callback function */
    scon_barrier_cbfunc_t cbfunc;
    /* user's cbdata */
    void *cbdata;
    /* info struct */
    scon_info_t *info;
    /* number of info */
    size_t ninfo;
} scon_barrier_t;
SCON_CLASS_DECLARATION(scon_barrier_t);


/* Request object for transfering requests to the event lib */
typedef struct {
    scon_object_t super;
    scon_event_t ev;
    union {
        scon_send_t send;
        scon_create_t create;
        scon_teardown_t teardown;
        scon_xcast_t xcast;
        scon_barrier_t barrier;
    }post;
} scon_send_req_t;
SCON_CLASS_DECLARATION(scon_send_req_t);

/* structure to recv scon messages - used internally */
typedef struct {
    scon_list_item_t super;
    scon_event_t ev;
    uint32_t scon_handle;
    scon_proc_t sender;  // sender
    scon_msg_tag_t tag;          // targeted tag
    struct iovec iov;            // the recvd data
} scon_recv_t;
SCON_CLASS_DECLARATION(scon_recv_t);

/* scon posted recvs */
typedef struct {
    scon_list_item_t super;
    uint32_t scon_handle;
    scon_proc_t peer;
    scon_msg_tag_t tag;
    bool persistent;
    scon_recv_cbfunc_t cbfunc;
    void *cbdata;
} scon_posted_recv_t;
SCON_CLASS_DECLARATION(scon_posted_recv_t);

/* define an object for transferring recv requests to the list of posted recvs */
typedef struct {
    scon_object_t super;
    scon_event_t ev;
    bool cancel;
    scon_posted_recv_t *post;
} scon_recv_req_t;
SCON_CLASS_DECLARATION(scon_recv_req_t);

/* common implementations */

scon_common_scon_t* scon_base_create (scon_proc_t procs[],
                                       size_t nprocs,
                                       scon_info_t info[],
                                       size_t ninfo,
                                       scon_create_cbfunc_t cbfunc,
                                       void *cbdata);
#if 0
/* Helper functions and  macros */
ORTE_DECLSPEC static inline bool orte_scon_is_master(orte_scon_scon_t * scon)
{
    return ((scon->master.jobid == ORTE_PROC_MY_NAME->jobid) &&
            (scon->master.vpid == ORTE_PROC_MY_NAME->vpid));
}
scon_status_t orte_scon_base_pack_scon_config (orte_scon_scon_t *scon,
                                               opal_buffer_t *buffer);
scon_status_t orte_scon_base_check_config (orte_scon_scon_t *scon,
                                           opal_buffer_t *buffer);
scon_handle_t orte_scon_base_get_handle(orte_scon_scon_t *scon,
                                        orte_process_name_t *member);
void orte_scon_base_post_recv(int sd, short args, void *cbdata);
void orte_scon_base_process_recv_msg(int fd, short flags, void *cbdata);

static inline uint32_t get_index (scon_handle_t handle)
{
    if (handle <= SCON_HANDLE_INVALID)
        return (handle - 1 ) % ORTE_SCON_MAX_SCONS;
    else
        ORTE_ERROR_LOG (SCON_ERR_INVALID_HANDLE);
    return 0;
}

#define SCON_GET_MASTER(scon)  &scon->master

#define ORTE_SCON_POST_MESSAGE(p, t, h, b, l )                             \
    do {                                                                   \
    orte_scon_recv_t *msg;                                                 \
    opal_output_verbose(5, orte_scon_base_framework.framework_output,      \
                       "%s Message from %s posted at %s:%d",               \
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),                \
                        ORTE_NAME_PRINT(p),                                \
                        __FILE__, __LINE__);                               \
    msg = OBJ_NEW(orte_scon_recv_t);                                       \
    msg->sender.jobid = (p)->jobid;                                        \
    msg->sender.vpid = (p)->vpid;                                          \
    msg->tag = (t);                                                        \
    msg->scon_handle = (h);                                                \
    msg->iov.iov_base = (IOVBASE_TYPE*)(b);                                \
    msg->iov.iov_len = (l);                                                \
    /* setup the event */                                                  \
    opal_event_set(orte_event_base, &msg->ev, -1,                          \
                   OPAL_EV_WRITE,                                          \
                   orte_scon_base_process_recv_msg, msg);                  \
    opal_event_set_priority(&msg->ev, ORTE_MSG_PRI);                       \
    opal_event_active(&msg->ev, OPAL_EV_WRITE, 1);                         \
} while(0);
#endif
END_C_DECLS

#endif /* SCON_COMMON_BASE_H */
