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
 * Copyright (c) 2013-2017 Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _SCON_PT2PT_TCP_LISTENER_H_
#define _SCON_PT2PT_TCP_LISTENER_H_

#include "scon_config.h"
#include "scon_types.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "src/class/scon_list.h"

/*
 * Data structure for accepting connections.
 */
struct scon_pt2pt_tcp_listener_t {
    scon_list_item_t item;
    bool ev_active;
    scon_event_t event;
    bool tcp6;
    int sd;
    uint16_t port;
};
typedef struct scon_pt2pt_tcp_listener_t scon_pt2pt_tcp_listener_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_listener_t);

typedef struct {
    scon_object_t super;
    scon_event_t ev;
    int fd;
    struct sockaddr_storage addr;
} scon_pt2pt_tcp_pending_connection_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_pending_connection_t);

int scon_pt2pt_tcp_start_listening(void);

#endif /* _SCON_PT2PT_TCP_LISTENER_H_ */
