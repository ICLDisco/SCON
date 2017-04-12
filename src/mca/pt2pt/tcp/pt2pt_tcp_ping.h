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
 * Copyright (c) 2014-2017     Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _SCON_PT2PT_TCP_PING_H_
#define _SCON_PT2PT_TCP_PING_H_

#include "scon_config.h"
#include "scon_types.h"

#include "pt2pt_tcp.h"
#include "pt2pt_tcp_sendrecv.h"

typedef struct {
    scon_object_t super;
    scon_event_t ev;
    scon_proc_t peer;
} scon_pt2pt_tcp_ping_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_ping_t);

#define ACTIVATE_TCP_PING(p, cbfunc)                               \
    do {                                                                \
        scon_pt2pt_tcp_ping_t *pop;                                        \
        pop = SCON_NEW(scon_pt2pt_tcp_ping_t);                              \
        pop->peer.jobid = (p)->jobid;                                   \
        pop->peer.vpid = (p)->vpid;                                     \
        scon_event_set(scon_pt2pt_tcp_module.ev_base, &pop->ev, -1,        \
                       SCON_EV_WRITE, (cbfunc), pop);                   \
        scon_event_set_priority(&pop->ev, SCON_MSG_PRI);                \
        scon_event_active(&pop->ev, SCON_EV_WRITE, 1);                  \
    } while(0);

#endif /* _SCON_PT2PT_TCP_PING_H_ */
