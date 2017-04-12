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
 * Copyright (c) 2014-2017 Intel, Inc. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _SCON_PT2PT_TCP_HDR_H_
#define _SCON_PT2PT_TCP_HDR_H_

#include "scon_config.h"

/* define several internal-only message
 * types this component uses for its own
 * handshake operations, plus one indicating
 * the message came from an external (to
 * this component) source
 */
typedef enum {
    SCON_PT2PT_TCP_IDENT,
    SCON_PT2PT_TCP_PROBE,
    SCON_PT2PT_TCP_PING,
    SCON_PT2PT_TCP_USER
} scon_pt2pt_tcp_msg_type_t;

/* header for tcp msgs */
typedef struct {
    scon_handle_t scon_handle;
    /* the originator of the message - if we are routing,
     * it could be someone other than me
     */
    scon_proc_t     origin;
    /* the intended final recipient - if we don't have
     * a path directly to that process, then we will
     * attempt to route. If we have no route to that
     * process, then we should have rejected the message
     * and let some other module try to send it
     */
    scon_proc_t     dst;
    /* type of message */
    scon_pt2pt_tcp_msg_type_t type;
    /* the msg tag where this message is headed */
    scon_msg_tag_t tag;
    /* the seq number of this message */
    uint32_t seq_num;
    /* number of bytes in message */
    uint32_t nbytes;
} scon_pt2pt_tcp_hdr_t;
/**
 * Convert the message header to host byte order
 */
#define  SCON_PROCESS_NAME_NTOH(proc_name)      \
    (proc_name).rank = ntohl(proc_name.rank);

#define  SCON_PROCESS_NAME_HTON(proc_name)      \
(proc_name).rank = htonl(proc_name.rank);

#define SCON_PT2PT_TCP_HDR_NTOH(h)              \
    (h)->scon_handle = ntohl((h)->scon_handle);   \
    SCON_PROCESS_NAME_NTOH((h)->origin);        \
    SCON_PROCESS_NAME_NTOH((h)->dst);           \
    (h)->type = ntohl((h)->type);               \
    (h)->tag = ntohl((h)->tag);                 \
    (h)->nbytes = ntohl((h)->nbytes);

/**
 * Convert the message header to network byte order
 */
#define SCON_PT2PT_TCP_HDR_HTON(h)              \
    (h)->scon_handle = htonl ((h)->scon_handle);   \
    SCON_PROCESS_NAME_HTON((h)->origin);        \
    SCON_PROCESS_NAME_HTON((h)->dst);           \
    (h)->type = htonl((h)->type);               \
    (h)->tag = htonl((h)->tag);                 \
    (h)->nbytes = htonl((h)->nbytes);

#endif /* _SCON_PT2PT_TCP_HDR_H_ */
