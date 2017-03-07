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
 * Copyright (c) 2009-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2013-2015 Intel, Inc.  All rights reserved.
 * Copyright (c) 2014-2015 Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "scon_config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_NET_UIO_H
#include <net/uio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include "scon_socket_errno.h"
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include "scon_common.h"
#include "src/util/output.h"
#include "src/util/net.h"
#include "src/util/error.h"
#include "src/class/scon_hash_table.h"
#include "src/runtime/scon_rte.h"

#include "src/util/name_fns.h"
#include "src/util/show_help.h"
#include "src/include/scon_globals.h"
#include "src/mca/topology/topology.h"

#include "pt2pt_tcp.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_component.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_peer.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_common.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_connection.h"

static void tcp_peer_event_init(scon_pt2pt_tcp_peer_t* peer);
static int  tcp_peer_send_connect_ack(scon_pt2pt_tcp_peer_t* peer);
static int tcp_peer_send_blocking(int sd, void* data, size_t size);
static bool tcp_peer_recv_blocking(scon_pt2pt_tcp_peer_t* peer, int sd,
                                   void* data, size_t size);
static void tcp_peer_connected(scon_pt2pt_tcp_peer_t* peer);

static int tcp_peer_create_socket(scon_pt2pt_tcp_peer_t* peer)
{
    int flags;

    if (peer->sd >= 0) {
        return SCON_SUCCESS;
    }

    scon_output_verbose(1, scon_pt2pt_base_framework.framework_output,
                         "%s pt2pt:tcp:peer creating socket to %s",
                         SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                         SCON_PRINT_PROC(&(peer->name)));

    peer->sd = socket(AF_INET, SOCK_STREAM, 0);
    if (peer->sd < 0) {
        scon_output(0, "%s-%s tcp_peer_create_socket: socket() failed: %s (%d)\n",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&(peer->name)),
                    strerror(scon_socket_errno),
                    scon_socket_errno);
        return SCON_ERR_UNREACH;
    }
   /* setup socket options */
    scon_pt2pt_tcp_set_socket_options(peer->sd);

    /* setup event callbacks */
    tcp_peer_event_init(peer);

    /* setup the socket as non-blocking */
    if (peer->sd >= 0) {
        if((flags = fcntl(peer->sd, F_GETFL, 0)) < 0) {
            scon_output(0, "%s-%s tcp_peer_connect: fcntl(F_GETFL) failed: %s (%d)\n",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)),
                        strerror(scon_socket_errno),
                        scon_socket_errno);
        } else {
            flags |= O_NONBLOCK;
            if(fcntl(peer->sd, F_SETFL, flags) < 0)
                scon_output(0, "%s-%s tcp_peer_connect: fcntl(F_SETFL) failed: %s (%d)\n",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&(peer->name)),
                            strerror(scon_socket_errno),
                            scon_socket_errno);
        }
    }

    return SCON_SUCCESS;
}


/*
 * Try connecting to a peer - cycle across all known addresses
 * until one succeeds.
 */
void scon_pt2pt_tcp_peer_try_connect(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_conn_op_t *op = (scon_pt2pt_tcp_conn_op_t*)cbdata;
    scon_pt2pt_tcp_peer_t *peer = op->peer;
    int rc;
    scon_socklen_t addrlen = 0;
    scon_pt2pt_tcp_addr_t *addr;
    char *host;
    scon_pt2pt_tcp_send_t *snd;
    bool connected = false;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s pt2pt_tcp_peer_try_connect: "
                        "attempting to connect to proc %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)));

    rc = tcp_peer_create_socket(peer);
    if (SCON_SUCCESS != rc) {
        /* FIXME: we cannot create a TCP socket - this spans
         * all interfaces, so all we can do is report
         * back to the component that this peer is
         * unreachable so it can remove the peer
         * from its list and report back to the base
         * NOTE: this could be a reconnect attempt,
         * so we also need to mark any queued messages
         * and return them as "unreachable"
         */
        scon_output(0, "%s CANNOT CREATE SOCKET", SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        SCON_RELEASE(op);
        return;
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s scon_tcp_peer_try_connect: "
                        "attempting to connect to proc %s on socket %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)), peer->sd);

    addrlen = sizeof(struct sockaddr_in);
    SCON_LIST_FOREACH(addr, &peer->addrs, scon_pt2pt_tcp_addr_t) {
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s pt2pt_tcp_peer_try_connect: "
                            "attempting to connect to proc %s on %s:%d - %d retries",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&(peer->name)),
                            scon_net_get_hostname((struct sockaddr*)&addr->addr),
                            scon_net_get_port((struct sockaddr*)&addr->addr),
                            addr->retries);
        if (SCON_PT2PT_TCP_FAILED == addr->state) {
            scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                "%s pt2pt_tcp_peer_try_connect: %s:%d is down",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                scon_net_get_hostname((struct sockaddr*)&addr->addr),
                                scon_net_get_port((struct sockaddr*)&addr->addr));
            continue;
        }
        if (scon_pt2pt_tcp_component.max_retries < addr->retries) {
            scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                "%s pt2pt_tcp_peer_try_connect: %s:%d retries exceeded",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                scon_net_get_hostname((struct sockaddr*)&addr->addr),
                                scon_net_get_port((struct sockaddr*)&addr->addr));
            continue;
        }
        peer->active_addr = addr;  // record the one we are using
    retry_connect:
        addr->retries++;
        if (connect(peer->sd, (struct sockaddr*)&addr->addr, addrlen) < 0) {
            /* non-blocking so wait for completion */
            if (scon_socket_errno == EINPROGRESS || scon_socket_errno == EWOULDBLOCK) {
                scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                    "%s waiting for connect completion to %s - activating send event",
                                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                    SCON_PRINT_PROC(&peer->name));
                /* just ensure the send_event is active */
                if (!peer->send_ev_active) {
                    scon_event_add(&peer->send_event, 0);
                    peer->send_ev_active = true;
                }
                SCON_RELEASE(op);
                return;
            }

            /* Some kernels (Linux 2.6) will automatically software
               abort a connection that was ECONNREFUSED on the last
               attempt, without even trying to establish the
               connection.  Handle that case in a semi-rational
               way by trying twice before giving up */
            if (ECONNABORTED == scon_socket_errno) {
                if (addr->retries < scon_pt2pt_tcp_component.max_retries) {
                    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                        "%s connection aborted by OS to %s - retrying",
                                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                        SCON_PRINT_PROC(&peer->name));
                    goto retry_connect;
                } else {
                    /* We were unsuccessful in establishing this connection, and are
                     * not likely to suddenly become successful, so rotate to next option
                     */
                    addr->state = SCON_PT2PT_TCP_FAILED;
                    continue;
                }
            }
        } else {
            /* connection succeeded */
            addr->retries = 0;
            connected = true;
            peer->num_retries = 0;
            break;
        }
    }

    if (!connected) {
        /* it could be that the intended recipient just hasn't
         * started yet. if requested, wait awhile and try again
         * unless/until we hit the maximum number of retries */
        if (0 < scon_pt2pt_tcp_component.retry_delay) {
            if (scon_pt2pt_tcp_component.max_recon_attempts < 0 ||
                peer->num_retries < scon_pt2pt_tcp_component.max_recon_attempts) {
                struct timeval tv;
                /* reset the addr states */
                SCON_LIST_FOREACH(addr, &peer->addrs, scon_pt2pt_tcp_addr_t) {
                    addr->state = SCON_PT2PT_TCP_UNCONNECTED;
                    addr->retries = 0;
                }
                /* give it awhile and try again */
                tv.tv_sec = scon_pt2pt_tcp_component.retry_delay;
                tv.tv_usec = 0;
                ++peer->num_retries;
                SCON_RETRY_TCP_CONN_STATE(peer, scon_pt2pt_tcp_peer_try_connect, &tv);
                goto cleanup;
            }
        }
        /* no address succeeded, so we cannot reach this peer */
        peer->state = SCON_PT2PT_TCP_FAILED;
        host = scon_net_get_hostname((struct sockaddr*)&(peer->active_addr->addr));
        /* use an scon_output here instead of show_help as we may well
         * not be connected to the HNP at this point */
      /*  scon_output(scon_clean_output,
                    "------------------------------------------------------------\n"
                    "A process or daemon was unable to complete a TCP connection\n"
                    "to another process:\n"
                    "  Local host:    %s\n"
                    "  Remote host:   %s\n"
                    "This is usually caused by a firewall on the remote host. Please\n"
                    "check that any firewall (e.g., iptables) has been disabled and\n"
                    "try again.\n"
                    "------------------------------------------------------------",
                    orte_process_info.nodename,
                    (NULL == host) ? "<unknown>" : host);*/
        /* let the TCP component know that this module failed to make
         * the connection so it can do some bookkeeping and fail back
         * to the OOB level so another component can try. This will activate
         * an event in the component event base, and so it will fire async
         * from us if we are in our own progress thread
         */
        SCON_ACTIVATE_TCP_CMP_OP(&peer->name, scon_pt2pt_tcp_component_failed_to_connect);
        /* FIXME: post any messages in the send queue back to the OOB
         * level for reassignment
         */
        if (NULL != peer->send_msg) {
        }
        while (NULL != (snd = (scon_pt2pt_tcp_send_t*)scon_list_remove_first(&peer->send_queue))) {
        }
        goto cleanup;
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s pt2pt_tcp_peer_try_connect: "
                        "Connection to proc %s succeeded",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name));

    /* setup our recv to catch the return ack call */
    if (!peer->recv_ev_active) {
        scon_event_add(&peer->recv_event, 0);
        peer->recv_ev_active = true;
    }

    /* send our globally unique process identifier to the peer */
    if (SCON_SUCCESS == (rc = tcp_peer_send_connect_ack(peer))) {
        peer->state = SCON_PT2PT_TCP_CONNECT_ACK;
    } else if (SCON_ERR_UNREACH == rc) {
        /* this could happen if we are in a race condition where both
         * we and the peer are trying to connect at the same time. If I
         * am the higher rank, then retry the connection - otherwise,
         * step aside for now */
        int cmpval = scon_util_compare_name_fields(SCON_NS_CMP_ALL, SCON_PROC_MY_NAME, &peer->name);
        if (SCON_VALUE1_GREATER == cmpval) {
            peer->state = SCON_PT2PT_TCP_CONNECTING;
            SCON_ACTIVATE_TCP_CONN_STATE(peer, scon_pt2pt_tcp_peer_try_connect);
        } else {
            peer->state = SCON_PT2PT_TCP_UNCONNECTED;
        }
        return;
    } else {
        scon_output(0,
                    "%s pt2pt_tcp_peer_try_connect: "
                    "tcp_peer_send_connect_ack to proc %s on %s:%d failed: (%d)",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&(peer->name)),
                    scon_net_get_hostname((struct sockaddr*)&addr->addr),
                    scon_net_get_port((struct sockaddr*)&addr->addr),
                    rc);
        //SCON_FORCED_TERMINATE(1);
    }

 cleanup:
    SCON_RELEASE(op);
}

/* send a handshake that includes our process identifier, our
 * version string, and a security token to ensure we are talking
 * to another OMPI process
 */
static int tcp_peer_send_connect_ack(scon_pt2pt_tcp_peer_t* peer)
{
    char *msg;
    scon_pt2pt_tcp_hdr_t hdr;
    size_t sdsize;
    char *cred = NULL;
    size_t credsize = 0;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s SEND CONNECT ACK", SCON_PRINT_PROC(SCON_PROC_MY_NAME));

    /* load the header */
    hdr.origin = *SCON_PROC_MY_NAME;
    hdr.dst = peer->name;
    hdr.type = SCON_PT2PT_TCP_IDENT;
    hdr.tag = 0;

    /* get our security credential*/
    /*if (SCON_SUCCESS != (rc = scon_sec.get_my_credential(peer->auth_method,
                                                         SCON_PROC_MY_NAME,
                                                         &cred, &credsize))) {
        SCON_ERROR_LOG(rc);
        return rc;
    }*/
    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s SENDING CREDENTIAL OF SIZE %lu",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (unsigned long)credsize);

    /* set the number of bytes to be read beyond the header */
    hdr.nbytes = strlen(scon_version_string) + 1 + credsize;
    SCON_PT2PT_TCP_HDR_HTON(&hdr);

    /* create a space for our message */
    sdsize = sizeof(hdr) + strlen(scon_version_string) + 1 + credsize;
    if (NULL == (msg = (char*)malloc(sdsize))) {
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    memset(msg, 0, sdsize);

    /* load the message */
    memcpy(msg, &hdr, sizeof(hdr));
    memcpy(msg+sizeof(hdr), scon_version_string, strlen(scon_version_string));
    memcpy(msg+sizeof(hdr)+strlen(scon_version_string)+1, cred, credsize);
    /* clear the memory */
    if (NULL != cred) {
        free(cred);
    }

    /* send it */
    if (SCON_SUCCESS != tcp_peer_send_blocking(peer->sd, msg, sdsize)) {
        free(msg);
        peer->state = SCON_PT2PT_TCP_FAILED;
        scon_pt2pt_tcp_peer_close(peer);
        return SCON_ERR_UNREACH;
    }
    free(msg);

    return SCON_SUCCESS;
}

/*
 * Initialize events to be used by the peer instance for TCP select/poll callbacks.
 */
static void tcp_peer_event_init(scon_pt2pt_tcp_peer_t* peer)
{
    if (peer->sd >= 0) {
        assert(!peer->send_ev_active && !peer->recv_ev_active);
        scon_event_set(scon_pt2pt_tcp_module.ev_base,
                       &peer->recv_event,
                       peer->sd,
                       SCON_EV_READ|SCON_EV_PERSIST,
                       scon_pt2pt_tcp_recv_handler,
                       peer);
        scon_event_set_priority(&peer->recv_event, SCON_MSG_PRI);
        if (peer->recv_ev_active) {
            scon_event_del(&peer->recv_event);
            peer->recv_ev_active = false;
        }

        scon_event_set(scon_pt2pt_tcp_module.ev_base,
                       &peer->send_event,
                       peer->sd,
                       SCON_EV_WRITE|SCON_EV_PERSIST,
                       scon_pt2pt_tcp_send_handler,
                       peer);
        scon_event_set_priority(&peer->send_event, SCON_MSG_PRI);
        if (peer->send_ev_active) {
            scon_event_del(&peer->send_event);
            peer->send_ev_active = false;
        }
    }
}

/*
 * Check the status of the connection. If the connection failed, will retry
 * later. Otherwise, send this processes identifier to the peer on the
 * newly connected socket.
 */
void scon_pt2pt_tcp_peer_complete_connect(scon_pt2pt_tcp_peer_t *peer)
{
    int so_error = 0;
    scon_socklen_t so_length = sizeof(so_error);

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s:tcp:complete_connect called for peer %s on socket %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name), peer->sd);

    /* check connect completion status */
    if (getsockopt(peer->sd, SOL_SOCKET, SO_ERROR, (char *)&so_error, &so_length) < 0) {
        scon_output(0, "%s tcp_peer_complete_connect: getsockopt() to %s failed: %s (%d)\n",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&(peer->name)),
                    strerror(scon_socket_errno),
                    scon_socket_errno);
        peer->state = SCON_PT2PT_TCP_FAILED;
        scon_pt2pt_tcp_peer_close(peer);
        return;
    }

    if (so_error == EINPROGRESS) {
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s:tcp:send:handler still in progress",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        return;
    } else if (so_error == ECONNREFUSED || so_error == ETIMEDOUT) {
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s-%s tcp_peer_complete_connect: connection failed: %s (%d)",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&(peer->name)),
                            strerror(so_error),
                            so_error);
        scon_pt2pt_tcp_peer_close(peer);
        return;
    } else if (so_error != 0) {
        /* No need to worry about the return code here - we return regardless
           at this point, and if an error did occur a message has already been
           printed for the user */
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s-%s tcp_peer_complete_connect: "
                            "connection failed with error %d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&(peer->name)), so_error);
        scon_pt2pt_tcp_peer_close(peer);
        return;
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s tcp_peer_complete_connect: "
                        "sending ack to %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)));

    if (tcp_peer_send_connect_ack(peer) == SCON_SUCCESS) {
        peer->state = SCON_PT2PT_TCP_CONNECT_ACK;
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s tcp_peer_complete_connect: "
                            "setting read event on connection to %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&(peer->name)));

        if (!peer->recv_ev_active) {
            scon_event_add(&peer->recv_event, 0);
            peer->recv_ev_active = true;
        }
    } else {
        scon_output(0, "%s tcp_peer_complete_connect: unable to send connect ack to %s",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&(peer->name)));
        peer->state = SCON_PT2PT_TCP_FAILED;
        scon_pt2pt_tcp_peer_close(peer);
    }
}

/*
 * A blocking send on a non-blocking socket. Used to send the small amount of connection
 * information that identifies the peers endpoint.
 */
static int tcp_peer_send_blocking(int sd, void* data, size_t size)
{
    unsigned char* ptr = (unsigned char*)data;
    size_t cnt = 0;
    int retval;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s send blocking of %lu bytes to socket %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        size, sd);

    while (cnt < size) {
        retval = send(sd, (char*)ptr+cnt, size-cnt, 0);
        if (retval < 0) {
            if (scon_socket_errno != EINTR && scon_socket_errno != EAGAIN && scon_socket_errno != EWOULDBLOCK) {
                scon_output(0, "%s tcp_peer_send_blocking: send() to socket %d failed: (%d)\n",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), sd,
                    scon_socket_errno);
                return SCON_ERR_UNREACH;
            }
            continue;
        }
        cnt += retval;
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s blocking send complete to socket %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), sd);

    return SCON_SUCCESS;
}

/*
 *  Receive the peers globally unique process identification from a newly
 *  connected socket and verify the expected response. If so, move the
 *  socket to a connected state.
 */
static bool retry(scon_pt2pt_tcp_peer_t* peer, int sd, bool fatal)
{
    int cmpval;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s SIMUL CONNECTION WITH %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name));
    cmpval = scon_util_compare_name_fields(SCON_NS_CMP_ALL, &peer->name, SCON_PROC_MY_NAME);
    if (fatal) {
        if (peer->send_ev_active) {
            scon_event_del(&peer->send_event);
            peer->send_ev_active = false;
        }
        if (peer->recv_ev_active) {
            scon_event_del(&peer->recv_event);
            peer->recv_ev_active = false;
        }
        if (0 <= peer->sd) {
            CLOSE_THE_SOCKET(peer->sd);
            peer->sd = -1;
        }
        if (SCON_VALUE1_GREATER == cmpval) {
            /* force the other end to retry the connection */
            peer->state = SCON_PT2PT_TCP_UNCONNECTED;
        } else {
            /* retry the connection */
            peer->state = SCON_PT2PT_TCP_CONNECTING;
            SCON_ACTIVATE_TCP_CONN_STATE(peer, scon_pt2pt_tcp_peer_try_connect);
        }
        return true;
    } else {
        if (SCON_VALUE1_GREATER == cmpval) {
            /* The other end will retry the connection */
            if (peer->send_ev_active) {
                scon_event_del(&peer->send_event);
                peer->send_ev_active = false;
            }
            if (peer->recv_ev_active) {
                scon_event_del(&peer->recv_event);
                peer->recv_ev_active = false;
            }
            CLOSE_THE_SOCKET(peer->sd);
            peer->state = SCON_PT2PT_TCP_UNCONNECTED;
            return false;
        } else {
            /* The connection will be retried */
            CLOSE_THE_SOCKET(sd);
            return true;
        }
    }
}

int scon_pt2pt_tcp_peer_recv_connect_ack(scon_pt2pt_tcp_peer_t* pr,
                                      int sd, scon_pt2pt_tcp_hdr_t *dhdr)
{
    char *msg;
    char *version;
    char *cred;
    size_t credsize;
    scon_pt2pt_tcp_hdr_t hdr;
    scon_pt2pt_tcp_peer_t *peer;
    uint64_t *ui64;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s RECV CONNECT ACK FROM %s ON SOCKET %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (NULL == pr) ? "UNKNOWN" : SCON_PRINT_PROC(&pr->name), sd);

    peer = pr;
    /* get the header */
    if (tcp_peer_recv_blocking(peer, sd, &hdr, sizeof(scon_pt2pt_tcp_hdr_t))) {
        if (NULL != peer) {
            /* If the peer state is CONNECT_ACK, then we were waiting for
             * the connection to be ack'd
             */
            if (peer->state != SCON_PT2PT_TCP_CONNECT_ACK) {
                /* handshake broke down - abort this connection */
                scon_output(0, "%s RECV CONNECT BAD HANDSHAKE (%d) FROM %s ON SOCKET %d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME), peer->state,
                            SCON_PRINT_PROC(&(peer->name)), sd);
                scon_pt2pt_tcp_peer_close(peer);
                return SCON_ERR_UNREACH;
            }
        }
    } else {
        /* unable to complete the recv */
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s unable to complete recv of connect-ack from %s ON SOCKET %d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            (NULL == peer) ? "UNKNOWN" : SCON_PRINT_PROC(&peer->name), sd);
        /* check for a race condition - if I was in the process of
         * creating a connection to the peer, or have already established
         * such a connection, then we need to reject this connection. We will
         * let the higher ranked process retry - if I'm the lower ranked
         * process, I'll simply defer until I receive the request
         */
        if (NULL != peer &&
            (SCON_PT2PT_TCP_CONNECTED == peer->state ||
             SCON_PT2PT_TCP_CONNECTING == peer->state ||
             SCON_PT2PT_TCP_CONNECT_ACK == peer->state ||
             SCON_PT2PT_TCP_CLOSED == peer->state)) {
            retry(peer, sd, false);
        }
        return SCON_ERR_UNREACH;
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s connect-ack recvd from %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (NULL == peer) ? "UNKNOWN" : SCON_PRINT_PROC(&peer->name));

    /* convert the header */
    SCON_PT2PT_TCP_HDR_NTOH(&hdr);
    /* if the requestor wanted the header returned, then do so now */
    if (NULL != dhdr) {
        *dhdr = hdr;
    }

    if (SCON_PT2PT_TCP_PROBE == hdr.type) {
        /* send a header back */
        hdr.type = SCON_PT2PT_TCP_PROBE;
        hdr.dst = hdr.origin;
        hdr.origin = *SCON_PROC_MY_NAME;
        SCON_PT2PT_TCP_HDR_HTON(&hdr);
        tcp_peer_send_blocking(sd, &hdr, sizeof(scon_pt2pt_tcp_hdr_t));
        CLOSE_THE_SOCKET(sd);
        return SCON_SUCCESS;
    }

    if (hdr.type != SCON_PT2PT_TCP_IDENT) {
        scon_output(0, "tcp_peer_recv_connect_ack: invalid header type: %d\n",
                    hdr.type);
        if (NULL != peer) {
            peer->state = SCON_PT2PT_TCP_FAILED;
            scon_pt2pt_tcp_peer_close(peer);
        } else {
            CLOSE_THE_SOCKET(sd);
        }
        return SCON_ERR_COMM_FAILURE;
    }

    /* if we don't already have it, get the peer */
    if (NULL == peer) {
        peer = scon_pt2pt_tcp_peer_lookup(&hdr.origin);
        if (NULL == peer) {
            scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                "%s scon_pt2pt_tcp_recv_connect: connection from new peer",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME));
            peer = SCON_NEW(scon_pt2pt_tcp_peer_t);
            peer->name = hdr.origin;
            peer->state = SCON_PT2PT_TCP_ACCEPTING;
            ui64 = (uint64_t*)(&peer->name);
            if (SCON_SUCCESS != scon_hash_table_set_value_uint64(&scon_pt2pt_tcp_module.peers, (*ui64), peer)) {
                SCON_RELEASE(peer);
                CLOSE_THE_SOCKET(sd);
                return SCON_ERR_OUT_OF_RESOURCE;
            }
        } else {
            /* check for a race condition - if I was in the process of
             * creating a connection to the peer, or have already established
             * such a connection, then we need to reject this connection. We will
             * let the higher ranked process retry - if I'm the lower ranked
             * process, I'll simply defer until I receive the request
             */
            if (SCON_PT2PT_TCP_CONNECTED == peer->state ||
                SCON_PT2PT_TCP_CONNECTING == peer->state ||
                SCON_PT2PT_TCP_CONNECT_ACK == peer->state) {
                if (retry(peer, sd, false)) {
                    return SCON_ERR_UNREACH;
                }
            }
        }
    } else {

        /* compare the peers name to the expected value */
        if (SCON_EQUAL != scon_util_compare_name_fields(SCON_NS_CMP_ALL, &peer->name, &hdr.origin)) {
            scon_output(0, "%s tcp_peer_recv_connect_ack: "
                        "received unexpected process identifier %s from %s\n",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(hdr.origin)),
                        SCON_PRINT_PROC(&(peer->name)));
            peer->state = SCON_PT2PT_TCP_FAILED;
            scon_pt2pt_tcp_peer_close(peer);
            return SCON_ERR_CONNECTION_REFUSED;
        }
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s connect-ack header from %s is okay",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name));

    /* get the authentication and version payload */
    if (NULL == (msg = (char*)malloc(hdr.nbytes))) {
        peer->state = SCON_PT2PT_TCP_FAILED;
        scon_pt2pt_tcp_peer_close(peer);
        return SCON_ERR_OUT_OF_RESOURCE;
    }
    if (!tcp_peer_recv_blocking(peer, sd, msg, hdr.nbytes)) {
        /* unable to complete the recv but should never happen */
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s unable to complete recv of connect-ack from %s ON SOCKET %d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&peer->name), peer->sd);
        /* check for a race condition - if I was in the process of
         * creating a connection to the peer, or have already established
         * such a connection, then we need to reject this connection. We will
         * let the higher ranked process retry - if I'm the lower ranked
         * process, I'll simply defer until I receive the request
         */
        if (SCON_PT2PT_TCP_CONNECTED == peer->state ||
            SCON_PT2PT_TCP_CONNECTING == peer->state ||
            SCON_PT2PT_TCP_CONNECT_ACK == peer->state) {
            retry(peer, sd, true);
        }
        free(msg);
        return SCON_ERR_UNREACH;
    }

    /* check that this is from a matching version */
    version = (char*)(msg);
    if (0 != strcmp(version, scon_version_string)) {
        scon_output(0, "%s tcp_peer_recv_connect_ack: "
                    "received different version from %s: %s instead of %s\n",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&(peer->name)),
                    version, scon_version_string);
        peer->state = SCON_PT2PT_TCP_FAILED;
        scon_pt2pt_tcp_peer_close(peer);
        free(msg);
        return SCON_ERR_CONNECTION_REFUSED;
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s connect-ack version from %s matches ours",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name));

    /* check security token */
    cred = (char*)(msg + strlen(version) + 1);
    credsize = hdr.nbytes - strlen(version) - 1;
  /*  if (SCON_SUCCESS != (rc = scon_sec.authenticate(cred, credsize, &peer->auth_method))) {
        char *hostname;
        hostname = scon_get_proc_hostname(&peer->name);
        scon_show_help("help-pt2pt-tcp.txt", "authent-fail", true,
                       (NULL == hostname) ? "unknown" : hostname,
                       scpm_process_info.nodename);
        peer->state = SCON_PT2PT_TCP_FAILED;
        scon_pt2pt_tcp_peer_close(peer);
        free(msg);
        return SCON_ERR_CONNECTION_REFUSED;
    }*/
    free(msg);

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s connect-ack %s authenticated",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name));

    /* if the requestor wanted the header returned, then they
     * will complete their processing
     */
    if (NULL != dhdr) {
        return SCON_SUCCESS;
    }

    /* set the peer into the component and OOB-level peer tables to indicate
     * that we know this peer and we will be handling him
     */
    SCON_ACTIVATE_TCP_CMP_OP(&peer->name, scon_pt2pt_tcp_component_set_module);

    /* connected */
    tcp_peer_connected(peer);
    if (PT2PT_TCP_DEBUG_CONNECT <= scon_output_get_verbosity(scon_pt2pt_base_framework.framework_output)) {
        scon_pt2pt_tcp_peer_dump(peer, "connected");
    }
    return SCON_SUCCESS;
}

/*
 *  Setup peer state to reflect that connection has been established,
 *  and start any pending sends.
 */
static void tcp_peer_connected(scon_pt2pt_tcp_peer_t* peer)
{
    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s-%s tcp_peer_connected on socket %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)), peer->sd);

    if (peer->timer_ev_active) {
        scon_event_del(&peer->timer_event);
        peer->timer_ev_active = false;
    }
    peer->state = SCON_PT2PT_TCP_CONNECTED;
    if (NULL != peer->active_addr) {
        peer->active_addr->retries = 0;
    }
    /* initiate send of first message on queue */
    if (NULL == peer->send_msg) {
        peer->send_msg = (scon_pt2pt_tcp_send_t*)
            scon_list_remove_first(&peer->send_queue);
    }
    if (NULL != peer->send_msg && !peer->send_ev_active) {
        scon_event_add(&peer->send_event, 0);
        peer->send_ev_active = true;
    }
}

/*
 * Remove any event registrations associated with the socket
 * and update the peer state to reflect the connection has
 * been closed.
 */
void scon_pt2pt_tcp_peer_close(scon_pt2pt_tcp_peer_t *peer)
{
    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s tcp_peer_close for %s sd %d state %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)),
                        peer->sd, scon_pt2pt_tcp_state_print(peer->state));

    /* release the socket */
    close(peer->sd);
    peer->sd = -1;

    /* if we were CONNECTING, then we need to mark the address as
     * failed and cycle back to try the next address */
    if (SCON_PT2PT_TCP_CONNECTING == peer->state) {
        if (NULL != peer->active_addr) {
            peer->active_addr->state = SCON_PT2PT_TCP_FAILED;
        }
        SCON_ACTIVATE_TCP_CONN_STATE(peer, scon_pt2pt_tcp_peer_try_connect);
        return;
    }

    peer->state = SCON_PT2PT_TCP_CLOSED;
    if (NULL != peer->active_addr) {
        peer->active_addr->state = SCON_PT2PT_TCP_CLOSED;
    }

    /* unregister active events */
    if (peer->recv_ev_active) {
        scon_event_del(&peer->recv_event);
        peer->recv_ev_active = false;
    }
    if (peer->send_ev_active) {
        scon_event_del(&peer->send_event);
        peer->send_ev_active = false;
    }

    /* inform the component-level that we have lost a connection so
     * it can decide what to do about it.
     */
    SCON_ACTIVATE_TCP_CMP_OP(&peer->name, scon_pt2pt_tcp_component_lost_connection);

    /* FIXME: push any queued messages back onto the PT2PT for retry - note that
     * this must be done after the prior call to ensure that the component
     * processes the "lost connection" notice before the PT2PT begins to
     * handle these recycled messages. This prevents us from unintentionally
     * attempting to send the message again across the now-failed interface
     */
 /*   if (NULL != peer->send_msg) {
    }
    while (NULL != (snd = (scon_pt2pt_tcp_send_t*)scon_list_remove_first(&peer->send_queue))) {
    }*/
}

/*
 * A blocking recv on a non-blocking socket. Used to receive the small amount of connection
 * information that identifies the peers endpoint.
 */
static bool tcp_peer_recv_blocking(scon_pt2pt_tcp_peer_t* peer, int sd,
                                   void* data, size_t size)
{
    unsigned char* ptr = (unsigned char*)data;
    size_t cnt = 0;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s waiting for connect ack from %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (NULL == peer) ? "UNKNOWN" : SCON_PRINT_PROC(&(peer->name)));

    while (cnt < size) {
        int retval = recv(sd, (char *)ptr+cnt, size-cnt, 0);

        /* remote closed connection */
        if (retval == 0) {
            scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                "%s-%s tcp_peer_recv_blocking: "
                                "peer closed connection: peer state %d",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                (NULL == peer) ? "UNKNOWN" : SCON_PRINT_PROC(&(peer->name)),
                                (NULL == peer) ? 0 : peer->state);
            if (NULL != peer) {
                scon_pt2pt_tcp_peer_close(peer);
            } else {
                CLOSE_THE_SOCKET(sd);
            }
            return false;
        }

        /* socket is non-blocking so handle errors */
        if (retval < 0) {
            if (scon_socket_errno != EINTR &&
                scon_socket_errno != EAGAIN &&
                scon_socket_errno != EWOULDBLOCK) {
                if (NULL == peer) {
                    /* protect against things like port scanners */
                    CLOSE_THE_SOCKET(sd);
                    return false;
                } else if (peer->state == SCON_PT2PT_TCP_CONNECT_ACK) {
                    /* If we overflow the listen backlog, it's
                       possible that even though we finished the three
                       way handshake, the remote host was unable to
                       transition the connection from half connected
                       (received the initial SYN) to fully connected
                       (in the listen backlog).  We likely won't see
                       the failure until we try to receive, due to
                       timing and the like.  The first thing we'll get
                       in that case is a RST packet, which receive
                       will turn into a connection reset by peer
                       errno.  In that case, leave the socket in
                       CONNECT_ACK and propogate the error up to
                       recv_connect_ack, who will try to establish the
                       connection again */
                    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                        "%s connect ack received error %s from %s",
                                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                        strerror(scon_socket_errno),
                                        SCON_PRINT_PROC(&(peer->name)));
                    return false;
                } else {
                    scon_output(0,
                                "%s tcp_peer_recv_blocking: "
                                "recv() failed for %s: %s (%d)\n",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                SCON_PRINT_PROC(&(peer->name)),
                                strerror(scon_socket_errno),
                                scon_socket_errno);
                    peer->state = SCON_PT2PT_TCP_FAILED;
                    scon_pt2pt_tcp_peer_close(peer);
                    return false;
                }
            }
            continue;
        }
        cnt += retval;
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s connect ack received from %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (NULL == peer) ? "UNKNOWN" : SCON_PRINT_PROC(&(peer->name)));
    return true;
}

/*
 * Routine for debugging to print the connection state and socket options
 */
void scon_pt2pt_tcp_peer_dump(scon_pt2pt_tcp_peer_t* peer, const char* msg)
{
    char src[64];
    char dst[64];
    char buff[255];
    int sndbuf,rcvbuf,nodelay,flags;
    struct sockaddr_storage inaddr;
    scon_socklen_t addrlen = sizeof(struct sockaddr_storage);
    scon_socklen_t optlen;

    if (getsockname(peer->sd, (struct sockaddr*)&inaddr, &addrlen) < 0) {
        scon_output(0, "tcp_peer_dump: getsockname: %s (%d)\n",
                    strerror(scon_socket_errno),
                    scon_socket_errno);
    } else {
        snprintf(src, sizeof(src), "%s", scon_net_get_hostname((struct sockaddr*) &inaddr));
    }
    if (getpeername(peer->sd, (struct sockaddr*)&inaddr, &addrlen) < 0) {
        scon_output(0, "tcp_peer_dump: getpeername: %s (%d)\n",
                    strerror(scon_socket_errno),
                    scon_socket_errno);
    } else {
        snprintf(dst, sizeof(dst), "%s", scon_net_get_hostname((struct sockaddr*) &inaddr));
    }

    if ((flags = fcntl(peer->sd, F_GETFL, 0)) < 0) {
        scon_output(0, "tcp_peer_dump: fcntl(F_GETFL) failed: %s (%d)\n",
                    strerror(scon_socket_errno),
                    scon_socket_errno);
    }

#if defined(SO_SNDBUF)
    optlen = sizeof(sndbuf);
    if(getsockopt(peer->sd, SOL_SOCKET, SO_SNDBUF, (char *)&sndbuf, &optlen) < 0) {
        scon_output(0, "tcp_peer_dump: SO_SNDBUF option: %s (%d)\n",
                    strerror(scon_socket_errno),
                    scon_socket_errno);
    }
#else
    sndbuf = -1;
#endif
#if defined(SO_RCVBUF)
    optlen = sizeof(rcvbuf);
    if (getsockopt(peer->sd, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuf, &optlen) < 0) {
        scon_output(0, "tcp_peer_dump: SO_RCVBUF option: %s (%d)\n",
                    strerror(scon_socket_errno),
                    scon_socket_errno);
    }
#else
    rcvbuf = -1;
#endif
#if defined(TCP_NODELAY)
    optlen = sizeof(nodelay);
    if (getsockopt(peer->sd, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, &optlen) < 0) {
        scon_output(0, "tcp_peer_dump: TCP_NODELAY option: %s (%d)\n",
                    strerror(scon_socket_errno),
                    scon_socket_errno);
    }
#else
    nodelay = 0;
#endif

    snprintf(buff, sizeof(buff), "%s-%s %s: %s - %s nodelay %d sndbuf %d rcvbuf %d flags %08x\n",
        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
        SCON_PRINT_PROC(&(peer->name)),
        msg, src, dst, nodelay, sndbuf, rcvbuf, flags);
    scon_output(0, "%s", buff);
}

/*
 * Accept incoming connection - if not already connected
 */

bool scon_pt2pt_tcp_peer_accept(scon_pt2pt_tcp_peer_t* peer)
{
    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:peer_accept called for peer %s in state %s on socket %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name),
                        scon_pt2pt_tcp_state_print(peer->state), peer->sd);

    if (peer->state != SCON_PT2PT_TCP_CONNECTED) {

        tcp_peer_event_init(peer);

        if (tcp_peer_send_connect_ack(peer) != SCON_SUCCESS) {
            scon_output(0, "%s-%s tcp_peer_accept: "
                        "tcp_peer_send_connect_ack failed\n",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)));
            peer->state = SCON_PT2PT_TCP_FAILED;
            scon_pt2pt_tcp_peer_close(peer);
            return false;
        }

        /* set the peer into the component and OOB-level peer tables to indicate
         * that we know this peer and we will be handling him
         */
        SCON_ACTIVATE_TCP_CMP_OP(&peer->name, scon_pt2pt_tcp_component_set_module);

        tcp_peer_connected(peer);
        if (!peer->recv_ev_active) {
            scon_event_add(&peer->recv_event, 0);
            peer->recv_ev_active = true;
        }
        if (PT2PT_TCP_DEBUG_CONNECT <= scon_output_get_verbosity(scon_pt2pt_base_framework.framework_output)) {
            scon_pt2pt_tcp_peer_dump(peer, "accepted");
        }
        return true;
    }

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:peer_accept ignored for peer %s in state %s on socket %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name),
                        scon_pt2pt_tcp_state_print(peer->state), peer->sd);
    return false;
}
