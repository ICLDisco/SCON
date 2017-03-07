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
 * Copyright (c) 2014-2016  Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _MCA_PT2PT_TCP_COMMON_H_
#define _MCA_PT2PT_TCP_COMMON_H_

#include "scon_config.h"

#include "pt2pt_tcp.h"
#include "pt2pt_tcp_peer.h"

void scon_pt2pt_tcp_set_socket_options(int sd);
char* scon_pt2pt_tcp_state_print(scon_pt2pt_tcp_conn_state_t state);
scon_pt2pt_tcp_peer_t* scon_pt2pt_tcp_peer_lookup(const scon_proc_t *name);
#endif /* _MCA_PT2PT_TCP_COMMON_H_ */
