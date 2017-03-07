/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2013 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2010-2011 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015-2016     Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _SCON_PT2PT_TCP_PEER_H_
#define _SCON_PT2PT_TCP_PEER_H_

#include "scon_config.h"

#include "pt2pt_tcp.h"
#include "pt2pt_tcp_sendrecv.h"

typedef struct {
    scon_list_item_t super;
    struct sockaddr_storage addr; // an address where a peer can be found
    int retries;                  // number of times we have tried to connect to this address
    scon_pt2pt_tcp_conn_state_t state;    // state of this address
} scon_pt2pt_tcp_addr_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_addr_t);

/* object for tracking peers in the module */
typedef struct {
    scon_list_item_t super;
    /* although not required, there is enough debug
     * value that retaining the name makes sense
     */
    scon_proc_t name;
    char *auth_method;  // method they used to authenticate
    int sd;
    scon_list_t addrs;
    scon_pt2pt_tcp_addr_t *active_addr;
    scon_pt2pt_tcp_conn_state_t state;
    int num_retries;
    scon_event_t send_event;    /**< registration with event thread for send events */
    bool send_ev_active;
    scon_event_t recv_event;    /**< registration with event thread for recv events */
    bool recv_ev_active;
    scon_event_t timer_event;   /**< timer for retrying connection failures */
    bool timer_ev_active;
    scon_list_t send_queue;      /**< list of messages to send */
    scon_pt2pt_tcp_send_t *send_msg; /**< current send in progress */
    scon_pt2pt_tcp_recv_t *recv_msg; /**< current recv in progress */
} scon_pt2pt_tcp_peer_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_peer_t);

/* state machine for processing peer data */
typedef struct {
    scon_comm_scon_t *scon;
    scon_object_t super;
    scon_event_t ev;
    scon_proc_t peer;
    uint16_t af_family;
    char *net;
    char *port;
} scon_pt2pt_tcp_peer_op_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_peer_op_t);

#define SCON_ACTIVATE_TCP_PEER_OP(p, a, n, pts, cbfunc)                 \
    do {                                                                \
        scon_pt2pt_tcp_peer_op_t *pop;                                   \
        pop = SCON_NEW(scon_pt2pt_tcp_peer_op_t);                        \
        strncpy(pop->peer.job_name,(p)->job_name,                     \
                  SCON_MAX_JOBLEN);                              \
        pop->peer.rank = (p)->rank;                                     \
        pop->af_family = (a);                                           \
        if (NULL != (n)) {                                              \
            pop->net = strdup((n));                                     \
        }                                                               \
        if (NULL != (pts)) {                                            \
            pop->port = strdup((pts));                                  \
        }                                                               \
        scon_event_set(scon_pt2pt_tcp_module.ev_base, &pop->ev, -1,      \
                       SCON_EV_WRITE, (cbfunc), pop);                   \
        scon_event_set_priority(&pop->ev, SCON_MSG_PRI);                \
        scon_event_active(&pop->ev, SCON_EV_WRITE, 1);                  \
    } while(0);

#define SCON_ACTIVATE_TCP_CMP_OP(p, cbfunc)                             \
    do {                                                                \
        scon_pt2pt_tcp_peer_op_t *pop;                                   \
        pop = SCON_NEW(scon_pt2pt_tcp_peer_op_t);                        \
        strncpy(pop->peer.job_name,(p)->job_name,                     \
               SCON_MAX_JOBLEN);                                 \
        pop->peer.rank = (p)->rank;                                     \
        scon_event_set(scon_pt2pt_tcp_module.ev_base, &pop->ev, -1,      \
                       SCON_EV_WRITE, (cbfunc), pop);                   \
        scon_event_set_priority(&pop->ev, SCON_MSG_PRI);                \
        scon_event_active(&pop->ev, SCON_EV_WRITE, 1);                  \
    } while(0);

#endif /* _SCON_PT2PT_TCP_PEER_H_ */
