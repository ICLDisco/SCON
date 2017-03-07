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
 * Copyright (c) 2014-2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _SCON_PT2PT_TCP_CONNECTION_H_
#define _SCON_PT2PT_TCP_CONNECTION_H_

#include "scon_config.h"
#include "scon_types.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "pt2pt_tcp.h"
#include "pt2pt_tcp_peer.h"

/* State machine for connection operations */
typedef struct {
    scon_object_t super;
    scon_pt2pt_tcp_peer_t *peer;
    scon_event_t ev;
} scon_pt2pt_tcp_conn_op_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_conn_op_t);

#define CLOSE_THE_SOCKET(socket)    \
    do {                            \
        shutdown(socket, 2);        \
        close(socket);              \
    } while(0)

#define SCON_ACTIVATE_TCP_CONN_STATE(p, cbfunc)                         \
    do {                                                                \
        scon_pt2pt_tcp_conn_op_t *cop;                                     \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] connect to %s",                 \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__,                         \
                            SCON_PRINT_PROC((&(p)->name)));             \
        cop = SCON_NEW(scon_pt2pt_tcp_conn_op_t);                           \
        cop->peer = (p);                                                \
        scon_event_set(scon_pt2pt_tcp_module.ev_base, &cop->ev, -1,        \
                       SCON_EV_WRITE, (cbfunc), cop);                   \
        scon_event_set_priority(&cop->ev, SCON_MSG_PRI);                \
        scon_event_active(&cop->ev, SCON_EV_WRITE, 1);                  \
    } while(0);

#define SCON_ACTIVATE_TCP_ACCEPT_STATE(s, a, cbfunc)        \
    do {                                                        \
        scon_pt2pt_tcp_conn_op_t *cop;                            \
        cop = SCON_NEW(scon_pt2pt_tcp_conn_op_t);                  \
        scon_event_set(scon_pt2pt_tcp_module.ev_base, &cop->ev, s, \
                       SCON_EV_READ, (cbfunc), cop);            \
        scon_event_set_priority(&cop->ev, SCON_MSG_PRI);        \
        scon_event_add(&cop->ev, 0);                            \
    } while(0);

#define SCON_RETRY_TCP_CONN_STATE(p, cbfunc, tv)                     \
    do {                                                                \
        scon_pt2pt_tcp_conn_op_t *cop;                                     \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] retry connect to %s",           \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__,                         \
                            SCON_PRINT_PROC((&(p)->name)));             \
        cop = SCON_NEW(scon_pt2pt_tcp_conn_op_t);                           \
        cop->peer = (p);                                                \
        scon_event_evtimer_set(scon_pt2pt_tcp_module.ev_base,                   \
                               &cop->ev,                                \
                               (cbfunc), cop);                          \
        scon_event_evtimer_add(&cop->ev, (tv));                         \
    } while(0);

void scon_pt2pt_tcp_peer_try_connect(int fd, short args, void *cbdata);
void scon_pt2pt_tcp_peer_dump(scon_pt2pt_tcp_peer_t* peer, const char* msg);
bool scon_pt2pt_tcp_peer_accept(scon_pt2pt_tcp_peer_t* peer);
void scon_pt2pt_tcp_peer_complete_connect(scon_pt2pt_tcp_peer_t* peer);
int scon_pt2pt_tcp_peer_recv_connect_ack(scon_pt2pt_tcp_peer_t* peer,
                                                           int sd, scon_pt2pt_tcp_hdr_t *dhdr);
 void scon_pt2pt_tcp_peer_close(scon_pt2pt_tcp_peer_t *peer);

#endif /* _SCON_PT2PT_TCP_CONNECTION_H_ */
