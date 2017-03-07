/**
 * copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 * This header defines  Scalable Overlay Network Interface for Runtime messaging
 */

/**
 * @file
 *
 * Point to Point messaging pt2pt Interface
 *
 * This layer provides interface for send/recv messages between two members of
 * the scalable overlay network
 */
#ifndef SCON_MCA_PT2PT_PT2PT_H
#define SCON_MCA_PT2PT_PT2PT_H

#include "scon.h"
#include "scon_common.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/class/scon_list.h"

BEGIN_C_DECLS

/* scon send req */
typedef struct {
    scon_list_item_t super;
    /*status */
    int status;
    /*  scon object */
    scon_handle_t scon_handle;
    /* sender or originator of msg */
    scon_proc_t origin;
    /* destination */
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
SCON_EXPORT SCON_CLASS_DECLARATION(scon_send_t);

/* Request object for transfering requests to the event lib */
typedef struct {
    scon_object_t super;
    scon_event_t ev;
    union {
        scon_send_t send;
    }post;
} scon_send_req_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_send_req_t);

/* structure to recv scon messages - used internally */
typedef struct {
    scon_list_item_t super;
    scon_event_t ev;
    scon_handle_t scon_handle;
    scon_proc_t sender;          // sender
    scon_msg_tag_t tag;          // targeted tag
    struct iovec iov;            // the recvd data
} scon_recv_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_recv_t);

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
SCON_EXPORT SCON_CLASS_DECLARATION(scon_posted_recv_t);

/* define an object for transferring recv requests to the list of posted recvs */
typedef struct {
    scon_object_t super;
    scon_event_t ev;
    bool cancel;
    scon_posted_recv_t *post;
} scon_recv_req_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_recv_req_t);

/** pt2pt module specific functions */
/**
 * scon send  - non blocking send a msg to a peer scon process
 */
typedef int (*scon_mca_pt2pt_module_send_nb_fn_t)(scon_send_t *msg);

/**
 * pt2pt module definition
 */
struct scon_pt2pt_base_module_1_0_0_t {
    scon_mca_base_module_t            super;
    scon_mca_pt2pt_module_send_nb_fn_t     send;
} ;

typedef struct scon_pt2pt_base_module_1_0_0_t scon_pt2pt_module_1_0_0_t;
typedef struct scon_pt2pt_base_module_1_0_0_t scon_pt2pt_module_t;

/** pt2pt component specific functions */
typedef int (*scon_mca_pt2pt_base_component_start_fn_t)(void);

typedef void (*scon_mca_pt2pt_base_component_shutdown_fn_t)(void);

/* Get the contact info for this component */
typedef char* (*scon_mca_pt2pt_base_component_get_addr_fn_t)(void);

/* Set contact info */
typedef int (*scon_mca_pt2pt_base_component_set_addr_fn_t)(scon_proc_t *peer,
                                                             char **uris);
/**
 * topology component specific functions
 */
typedef scon_pt2pt_module_t* (*scon_mca_pt2pt_base_component_get_module_fn_t) (void);

/**
 * pt2pt Component definition
 */
struct scon_pt2pt_base_component_1_0_0_t {
    /** MCA base component */
    scon_mca_base_component_t                             base_version;
    /** Default priority */
    int                                                   priority;
    int                                                   idx;
    scon_mca_pt2pt_base_component_start_fn_t              start;
    scon_mca_pt2pt_base_component_shutdown_fn_t           shutdown;
    scon_mca_pt2pt_base_component_get_addr_fn_t           get_addr;
    scon_mca_pt2pt_base_component_set_addr_fn_t           set_addr;
    scon_mca_pt2pt_base_component_get_module_fn_t         get_module;
};
typedef struct scon_pt2pt_base_component_1_0_0_t scon_pt2pt_base_component_1_0_0_t;
typedef struct scon_pt2pt_base_component_1_0_0_t scon_pt2pt_base_component_t;
/**
 * Macro for use in components that are of type Pt2pt
 */
#define SCON_PT2PT_BASE_VERSION_1_0_0              \
    SCON_MCA_BASE_VERSION_1_0_0("pt2pt", 1, 0, 0)

/** PREDEFINED MESSAGE TAGS **/
#define SCON_MSG_TAG_CFG_INFO              1
#define SCON_MSG_TAG_XCAST                 5
#define SCON_MSG_TAG_ALLGATHER_DIRECT      6
#define SCON_MSG_TAG_BARRIER_DIRECT        7
#define SCON_MSG_TAG_COLL_RELEASE          8
#define SCON_MSG_TAG_ALLGATHER_BRUCKS      9
#define SCON_MSG_TAG_BARRIER_BRUCKS        10
#define SCON_MSG_TAG_ALLGATHER_RCD         11
#define SCON_MSG_TAG_BARRIER_RCD           12
/** RECV MSG FLAGS */
#define SCON_MSG_PERSISTENT                1


END_C_DECLS

#endif /* SCON_MCA_PT2PT_PT2PT_H */
