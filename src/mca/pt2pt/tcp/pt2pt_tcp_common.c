/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2013 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * Copyright (c) 2014      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * In windows, many of the socket functions return an EWOULDBLOCK
 * instead of things like EAGAIN, EINPROGRESS, etc. It has been
 * verified that this will not conflict with other error codes that
 * are returned by these functions under UNIX/Linux environments
 */

#include "scon_config.h"
#include "scon_common.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <fcntl.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <ctype.h>

#include "src/util/error.h"
#include "src/util/output.h"
#include "src/include/scon_socket_errno.h"
/*#include "scon/util/if.h"
#include "scon/util/net.h"*/
#include "src/class/scon_hash_table.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_component.h"
#include "pt2pt_tcp_peer.h"
#include "pt2pt_tcp_common.h"

/**
 * Set socket buffering
 */
static void set_keepalive(int sd)
{
#if defined(SO_KEEPALIVE)
    int option;
    socklen_t optlen;

    /* see if the keepalive option is available */
    optlen = sizeof(option);
    if (getsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &option, &optlen) < 0) {
        /* not available, so just return */
        return;
    }

    /* Set the option active */
    option = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &option, optlen) < 0) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "[%s:%d] setsockopt(SO_KEEPALIVE) failed: %s (%d)",
                            __FILE__, __LINE__,
                            strerror(scon_socket_errno),
                            scon_socket_errno);
        return;
    }
#if defined(TCP_KEEPALIVE)
    /* set the idle time */
    if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPALIVE,
                   &mca_pt2pt_tcp_component.keepalive_time,
                   sizeof(mca_pt2pt_tcp_component.keepalive_time)) < 0) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "[%s:%d] setsockopt(TCP_KEEPALIVE) failed: %s (%d)",
                            __FILE__, __LINE__,
                            strerror(scon_socket_errno),
                            scon_socket_errno);
        return;
    }
#elif defined(TCP_KEEPIDLE)
    /* set the idle time */
    if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPIDLE,
                   &mca_pt2pt_tcp_component.keepalive_time,
                   sizeof(mca_pt2pt_tcp_component.keepalive_time)) < 0) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "[%s:%d] setsockopt(TCP_KEEPIDLE) failed: %s (%d)",
                            __FILE__, __LINE__,
                            strerror(scon_socket_errno),
                            scon_socket_errno);
        return;
    }
#endif  // TCP_KEEPIDLE
#if defined(TCP_KEEPINTVL)
    /* set the keepalive interval */
    if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPINTVL,
                   &mca_pt2pt_tcp_component.keepalive_intvl,
                   sizeof(mca_pt2pt_tcp_component.keepalive_intvl)) < 0) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "[%s:%d] setsockopt(TCP_KEEPINTVL) failed: %s (%d)",
                            __FILE__, __LINE__,
                            strerror(scon_socket_errno),
                            scon_socket_errno);
        return;
    }
#endif  // TCP_KEEPINTVL
#if defined(TCP_KEEPCNT)
    /* set the miss rate */
    if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPCNT,
                   &mca_pt2pt_tcp_component.keepalive_probes,
                   sizeof(mca_pt2pt_tcp_component.keepalive_probes)) < 0) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "[%s:%d] setsockopt(TCP_KEEPCNT) failed: %s (%d)",
                            __FILE__, __LINE__,
                            strerror(scon_socket_errno),
                            scon_socket_errno);
    }
#endif  // TCP_KEEPCNT
#endif //SO_KEEPALIVE
}

void scon_pt2pt_tcp_set_socket_options(int sd)
{

#if defined(TCP_NODELAY)
    int optval;
    optval = 1;
    if (setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval)) < 0) {
       // scon_backtrace_print(stderr, NULL, 1);
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "[%s:%d] setsockopt(TCP_NODELAY) failed: %s (%d)",
                            __FILE__, __LINE__,
                            strerror(scon_socket_errno),
                            scon_socket_errno);
    }
#endif

#if defined(SO_SNDBUF)
    if (mca_pt2pt_tcp_component.tcp_sndbuf > 0 &&
        setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&mca_pt2pt_tcp_component.tcp_sndbuf, sizeof(int)) < 0) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "[%s:%d] setsockopt(SO_SNDBUF) failed: %s (%d)",
                            __FILE__, __LINE__,
                            strerror(scon_socket_errno),
                            scon_socket_errno);
    }
#endif
#if defined(SO_RCVBUF)
    if (mca_pt2pt_tcp_component.tcp_rcvbuf > 0 &&
        setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&mca_pt2pt_tcp_component.tcp_rcvbuf, sizeof(int)) < 0) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "[%s:%d] setsockopt(SO_RCVBUF) failed: %s (%d)",
                            __FILE__, __LINE__,
                            strerror(scon_socket_errno),
                            scon_socket_errno);
    }
#endif

    if (0 < mca_pt2pt_tcp_component.keepalive_time) {
        set_keepalive(sd);
    }
}

scon_pt2pt_tcp_peer_t* scon_pt2pt_tcp_peer_lookup(const scon_proc_t *name)
{
    scon_pt2pt_tcp_peer_t *peer;
    uint64_t ui64;

    //memcpy(&ui64, (char*)name, sizeof(uint64_t));
    scon_util_convert_process_name_to_uint64(&ui64, name);
    if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_tcp_module.peers, ui64, (void**)&peer)) {
        return NULL;
    }
    return peer;
}

char* scon_pt2pt_tcp_state_print(scon_pt2pt_tcp_conn_state_t state)
{
    switch (state) {
    case SCON_PT2PT_TCP_UNCONNECTED:
        return "UNCONNECTED";
    case SCON_PT2PT_TCP_CLOSED:
        return "CLOSED";
    case SCON_PT2PT_TCP_RESOLVE:
        return "RESOLVE";
    case SCON_PT2PT_TCP_CONNECTING:
        return "CONNECTING";
    case SCON_PT2PT_TCP_CONNECT_ACK:
        return "ACK";
    case SCON_PT2PT_TCP_CONNECTED:
        return "CONNECTED";
    case SCON_PT2PT_TCP_FAILED:
        return "FAILED";
    default:
        return "UNKNOWN";
    }
}

