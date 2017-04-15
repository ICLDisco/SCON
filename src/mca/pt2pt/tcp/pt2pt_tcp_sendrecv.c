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
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2013-2017 Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * In windows, many of the socket functions return an EWOULDBLOCK
 * instead of \ things like EAGAIN, EINPROGRESS, etc. It has been
 * verified that this will \ not conflict with other error codes that
 * are returned by these functions \ under UNIX/Linux environments
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
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#include "scon_stdint.h"
#include "scon_types.h"
#include "scon_common.h"
#include "scon_socket_errno.h"
#include "src/util/output.h"
#include "src/util/net.h"
#include "src/util/error.h"
#include "src/class/scon_hash_table.h"
#include "src/buffer_ops/types.h"
#include "src/buffer_ops/buffer_ops.h"
#include "src/util/name_fns.h"
#include "src/include/scon_globals.h"
#include "src/mca/topology/topology.h"

#include "pt2pt_tcp.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_component.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_peer.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_common.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_connection.h"


static int send_bytes(scon_pt2pt_tcp_peer_t* peer)
{
    scon_pt2pt_tcp_send_t* msg = peer->send_msg;
    int rc;
/*
    SCON_TIMING_EVENT((&tm_pt2pt, "to %s %d bytes",
                       SCON_PRINT_PROC(&(peer->name)), msg->sdbytes));*/

    while (0 < msg->sdbytes) {
        rc = write(peer->sd, msg->sdptr, msg->sdbytes);
        if (rc < 0) {
            if (scon_socket_errno == EINTR) {
                continue;
            } else if (scon_socket_errno == EAGAIN) {
                /* tell the caller to keep this message on active,
                 * but let the event lib cycle so other messages
                 * can progress while this socket is busy
                 */
                return SCON_ERR_RESOURCE_BUSY;
            } else if (scon_socket_errno == EWOULDBLOCK) {
                /* tell the caller to keep this message on active,
                 * but let the event lib cycle so other messages
                 * can progress while this socket is busy
                 */
                return SCON_ERR_WOULD_BLOCK;
            }
            /* we hit an error and cannot progress this message */
            scon_output(0, "%s->%s scon_pt2pt_tcp_msg_send_bytes: write failed: %s (%d) [sd = %d]",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)),
                        strerror(scon_socket_errno),
                        scon_socket_errno,
                        peer->sd);
            return SCON_ERR_COMM_FAILURE;
        }
        /* update location */
        msg->sdbytes -= rc;
        msg->sdptr += rc;
    }
    /* we sent the full data block */
    return SCON_SUCCESS;
}

/*
 * A file descriptor is available/ready for send. Check the state
 * of the socket and take the appropriate action.
 */
void scon_pt2pt_tcp_send_handler(int sd, short flags, void *cbdata)
{
    scon_pt2pt_tcp_peer_t* peer = (scon_pt2pt_tcp_peer_t*)cbdata;
    scon_pt2pt_tcp_send_t* msg = peer->send_msg;
    int rc;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:send_handler called to send to peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name));

    switch (peer->state) {
    case SCON_PT2PT_TCP_CONNECTING:
    case SCON_PT2PT_TCP_CLOSED:
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s tcp:send_handler %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            scon_pt2pt_tcp_state_print(peer->state));
        scon_pt2pt_tcp_peer_complete_connect(peer);
        /* de-activate the send event until the connection
         * handshake completes
         */
        if (peer->send_ev_active) {
            scon_event_del(&peer->send_event);
            peer->send_ev_active = false;
        }
        break;
    case SCON_PT2PT_TCP_CONNECTED:
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s tcp:send_handler SENDING TO %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            (NULL == peer->send_msg) ? "NULL" : SCON_PRINT_PROC(&peer->name));
        if (NULL != msg) {
            /* if the header hasn't been completely sent, send it */
            if (!msg->hdr_sent) {
                if (SCON_SUCCESS == (rc = send_bytes(peer))) {
                    /* header is completely sent */
                    msg->hdr_sent = true;
                    /* setup to send the data */
                    if (NULL != msg->data) {
                        /* relay msg - send that data */
                        msg->sdptr = msg->data;
                        msg->sdbytes = (int)ntohl(msg->hdr.nbytes);
                    } else if (NULL == msg->msg) {
                        /* this was a zero-byte relay - nothing more to do */
                        SCON_RELEASE(msg);
                        peer->send_msg = NULL;
                        goto next;
                    } else if (NULL != msg->msg->buf) {
                        /* send the buffer data as a single block */
                        msg->sdptr = msg->msg->buf->base_ptr;
                        msg->sdbytes = msg->msg->buf->bytes_used;
                    }
                    /* fall thru and let the send progress */
                } else if (SCON_ERR_RESOURCE_BUSY == rc ||
                           SCON_ERR_WOULD_BLOCK == rc) {
                    /* exit this event and let the event lib progress */
                    return;
                } else {
                    // report the error
                    scon_output(0, "%s-%s scon_pt2pt_tcp_peer_send_handler: unable to send header",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                SCON_PRINT_PROC(&(peer->name)));
                    scon_event_del(&peer->send_event);
                    msg->msg->status = rc;
                    PT2PT_SEND_COMPLETE(msg->msg);
                    SCON_RELEASE(msg);
                    peer->send_msg = NULL;
                    goto next;
                }
            }
            /* progress the data transmission */
            if (msg->hdr_sent) {
                if (SCON_SUCCESS == (rc = send_bytes(peer))) {
                    /* this block is complete */
                    if (NULL != msg->data || NULL == msg->msg) {
                        /* the relay is complete - release the data */
                        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                            "%s MESSAGE RELAY COMPLETE TO %s OF %d BYTES ON SOCKET %d",
                                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                            SCON_PRINT_PROC(&(peer->name)),
                                            (int)ntohl(msg->hdr.nbytes), peer->sd);
                        SCON_RELEASE(msg);
                        peer->send_msg = NULL;
                    } else if (NULL != msg->msg->buf) {
                        /* we are done - notify the pt2pt */
                        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                            "%s MESSAGE SEND COMPLETE TO %s OF %d BYTES ON SOCKET %d",
                                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                            SCON_PRINT_PROC(&(peer->name)),
                                            (int)ntohl(msg->hdr.nbytes), peer->sd);
                        msg->msg->status = SCON_SUCCESS;
                        PT2PT_SEND_COMPLETE(msg->msg);
                        SCON_RELEASE(msg);
                        peer->send_msg = NULL;
                    } else if (NULL != msg->data) {
                        /* this was a relay we have now completed - no need to
                         * notify the upper framework as the local proc didn't initiate
                         * the send
                         */
                        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                            "%s MESSAGE RELAY COMPLETE TO %s OF %d BYTES ON SOCKET %d",
                                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                            SCON_PRINT_PROC(&(peer->name)),
                                            (int)ntohl(msg->hdr.nbytes), peer->sd);
                        msg->msg->status = SCON_SUCCESS;
                        SCON_RELEASE(msg);
                        peer->send_msg = NULL;
                    }
                } else if (SCON_ERR_RESOURCE_BUSY == rc ||
                           SCON_ERR_WOULD_BLOCK == rc) {
                    /* exit this event and let the event lib progress */
                    return;
                } else {
                    // report the error
                    scon_output(0, "%s-%s scon_pt2pt_tcp_peer_send_handler: unable to send message ON SOCKET %d",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                SCON_PRINT_PROC(&(peer->name)), peer->sd);
                    scon_event_del(&peer->send_event);
                    msg->msg->status = rc;
                    PT2PT_SEND_COMPLETE(msg->msg);
                    SCON_RELEASE(msg);
                    peer->send_msg = NULL;
                    return;
                }
            }

        next:
            /* if current message completed - progress any pending sends by
             * moving the next in the queue into the "on-deck" position. Note
             * that this doesn't mean we send the message right now - we will
             * wait for another send_event to fire before doing so. This gives
             * us a chance to service any pending recvs.
             */
            peer->send_msg = (scon_pt2pt_tcp_send_t*)
                scon_list_remove_first(&peer->send_queue);
        }

        /* if nothing else to do unregister for send event notifications */
        if (NULL == peer->send_msg && peer->send_ev_active) {
            scon_event_del(&peer->send_event);
            peer->send_ev_active = false;
        }
        break;
    default:
        scon_output(0, "%s-%s scon_pt2pt_tcp_peer_send_handler: invalid connection state (%d) on socket %d",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&(peer->name)),
                    peer->state, peer->sd);
        if (peer->send_ev_active) {
            scon_event_del(&peer->send_event);
            peer->send_ev_active = false;
        }
        break;
    }
}

static int read_bytes(scon_pt2pt_tcp_peer_t* peer)
{
    int rc;
#if SCON_ENABLE_TIMING
    int to_read = peer->recv_msg->rdbytes;
#endif

    /* read until all bytes recvd or error */
    while (0 < peer->recv_msg->rdbytes) {
        rc = read(peer->sd, peer->recv_msg->rdptr, peer->recv_msg->rdbytes);
        if (rc < 0) {
            if(scon_socket_errno == EINTR) {
                continue;
            } else if (scon_socket_errno == EAGAIN) {
                /* tell the caller to keep this message on active,
                 * but let the event lib cycle so other messages
                 * can progress while this socket is busy
                 */
                return SCON_ERR_RESOURCE_BUSY;
            } else if (scon_socket_errno == EWOULDBLOCK) {
                /* tell the caller to keep this message on active,
                 * but let the event lib cycle so other messages
                 * can progress while this socket is busy
                 */
                return SCON_ERR_WOULD_BLOCK;
            }
            /* we hit an error and cannot progress this message - report
             * the error back to the RML and let the caller know
             * to abort this message
             */
            scon_output_verbose(PT2PT_TCP_DEBUG_FAIL, scon_pt2pt_base_framework.framework_output,
                                "%s-%s scon_pt2pt_tcp_msg_recv: readv failed: %s (%d)",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                SCON_PRINT_PROC(&(peer->name)),
                                strerror(scon_socket_errno),
                                scon_socket_errno);
            // scon_pt2pt_tcp_peer_close(peer);
            // if (NULL != scon_pt2pt_tcp.pt2pt_exception_callback) {
            // scon_pt2pt_tcp.pt2pt_exception_callback(&peer->name, SCON_RML_PEER_DISCONNECTED);
            //}
            return SCON_ERR_COMM_FAILURE;
        } else if (rc == 0)  {
            /* the remote peer closed the connection - report that condition
             * and let the caller know
             */
            scon_output_verbose(PT2PT_TCP_DEBUG_FAIL, scon_pt2pt_base_framework.framework_output,
                                "%s-%s scon_pt2pt_tcp_msg_recv: peer closed connection",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                SCON_PRINT_PROC(&(peer->name)));
            /* stop all events */
            if (peer->recv_ev_active) {
                scon_event_del(&peer->recv_event);
                peer->recv_ev_active = false;
            }
            if (peer->timer_ev_active) {
                scon_event_del(&peer->timer_event);
                peer->timer_ev_active = false;
            }
            if (peer->send_ev_active) {
                scon_event_del(&peer->send_event);
                peer->send_ev_active = false;
            }
            if (NULL != peer->recv_msg) {
                SCON_RELEASE(peer->recv_msg);
                peer->recv_msg = NULL;
            }
            scon_pt2pt_tcp_peer_close(peer);
            //if (NULL != scon_pt2pt_tcp.pt2pt_exception_callback) {
            //   scon_pt2pt_tcp.pt2pt_exception_callback(&peer->peer_name, SCON_RML_PEER_DISCONNECTED);
            //}
            return SCON_ERR_WOULD_BLOCK;
        }
        /* we were able to read something, so adjust counters and location */
        peer->recv_msg->rdbytes -= rc;
        peer->recv_msg->rdptr += rc;
    }
    /* we read the full data block */
    return SCON_SUCCESS;
}

/*
 * Dispatch to the appropriate action routine based on the state
 * of the connection with the peer.
 */

void scon_pt2pt_tcp_recv_handler(int sd, short flags, void *cbdata)
{
    scon_pt2pt_tcp_peer_t* peer = (scon_pt2pt_tcp_peer_t*)cbdata;
    int rc;
    scon_send_t *snd;
    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s:tcp:recv:handler called for peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&peer->name));

    switch (peer->state) {
    case SCON_PT2PT_TCP_CONNECT_ACK:
        if (SCON_SUCCESS == (rc = scon_pt2pt_tcp_peer_recv_connect_ack(peer, peer->sd, NULL))) {
            scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                "%s:tcp:recv:handler starting send/recv events",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME));
            /* we connected! Start the send/recv events */
            if (!peer->recv_ev_active) {
                scon_event_add(&peer->recv_event, 0);
                peer->recv_ev_active = true;
            }
            if (peer->timer_ev_active) {
                scon_event_del(&peer->timer_event);
                peer->timer_ev_active = false;
            }
            /* if there is a message waiting to be sent, queue it */
            if (NULL == peer->send_msg) {
                peer->send_msg = (scon_pt2pt_tcp_send_t*)scon_list_remove_first(&peer->send_queue);
            }
            if (NULL != peer->send_msg && !peer->send_ev_active) {
                scon_event_add(&peer->send_event, 0);
                peer->send_ev_active = true;
            }
            /* update our state */
            peer->state = SCON_PT2PT_TCP_CONNECTED;
        } else if (SCON_ERR_UNREACH != rc) {
            /* we get an unreachable error returned if a connection
             * completes but is rejected - otherwise, we don't want
             * to terminate as we might be retrying the connection */
            scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                "%s UNABLE TO COMPLETE CONNECT ACK WITH %s",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                SCON_PRINT_PROC(&peer->name));
            scon_event_del(&peer->recv_event);
            return;
        }
        break;
    case SCON_PT2PT_TCP_CONNECTED:
        scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                            "%s:tcp:recv:handler CONNECTED",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        /* allocate a new message and setup for recv */
        if (NULL == peer->recv_msg) {
            scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                "%s:tcp:recv:handler allocate new recv msg",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME));
            peer->recv_msg = SCON_NEW(scon_pt2pt_tcp_recv_t);
            if (NULL == peer->recv_msg) {
                scon_output(0, "%s-%s scon_pt2pt_tcp_peer_recv_handler: unable to allocate recv message\n",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&(peer->name)));
                return;
            }
            /* start by reading the header */
            peer->recv_msg->rdptr = (char*)&peer->recv_msg->hdr;
            peer->recv_msg->rdbytes = sizeof(scon_pt2pt_tcp_hdr_t);
        }
        /* if the header hasn't been completely read, read it */
        if (!peer->recv_msg->hdr_recvd) {
            scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                "%s:tcp:recv:handler read hdr",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME));

            if (SCON_SUCCESS == (rc = read_bytes(peer))) {
                /* completed reading the header */
                peer->recv_msg->hdr_recvd = true;
                /* convert the header */
                SCON_PT2PT_TCP_HDR_NTOH(&peer->recv_msg->hdr);
                /* if this is a zero-byte message, then we are done */
                if (0 == peer->recv_msg->hdr.nbytes) {
                    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                        "%s RECVD ZERO-BYTE MESSAGE FROM %s for tag %d",
                                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                        SCON_PRINT_PROC(&peer->name), peer->recv_msg->hdr.tag);
                    peer->recv_msg->data = NULL;  // make sure
                } else {
                    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                        "%s:tcp:recv:handler allocate data region of size %lu",
                                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), (unsigned long)peer->recv_msg->hdr.nbytes);
                    /* allocate the data region */
                    peer->recv_msg->data = (char*)malloc(peer->recv_msg->hdr.nbytes);
                    /* point to it */
                    peer->recv_msg->rdptr = peer->recv_msg->data;
                    peer->recv_msg->rdbytes = peer->recv_msg->hdr.nbytes;
                }
                /* fall thru and attempt to read the data */
            } else if (SCON_ERR_RESOURCE_BUSY == rc ||
                       SCON_ERR_WOULD_BLOCK == rc) {
                /* exit this event and let the event lib progress */
                return;
            } else {
                /* close the connection */
                scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                    "%s:tcp:recv:handler error reading bytes - closing connection",
                                    SCON_PRINT_PROC(SCON_PROC_MY_NAME));
                scon_pt2pt_tcp_peer_close(peer);
                return;
            }
        }

        if (peer->recv_msg->hdr_recvd) {
            /* continue to read the data block - we start from
             * wherever we left off, which could be at the
             * beginning or somewhere in the message
             */
            if (SCON_SUCCESS == (rc = read_bytes(peer))) {
                /* we recvd all of the message */
                scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                    "%s RECVD COMPLETE MESSAGE FROM %s (ORIGIN %s) OF %d BYTES FOR DEST %s TAG %d",
                                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                    SCON_PRINT_PROC(&peer->name),
                                    SCON_PRINT_PROC(&peer->recv_msg->hdr.origin),
                                    (int)peer->recv_msg->hdr.nbytes,
                                    SCON_PRINT_PROC(&peer->recv_msg->hdr.dst),
                                    peer->recv_msg->hdr.tag);

                /* am I the intended recipient (header was already converted back to host order)? */
                if (SCON_EQUAL == scon_util_compare_name_fields(SCON_NS_CMP_ALL, &peer->recv_msg->hdr.dst,
                            SCON_PROC_MY_NAME)) {
                    /* yes - post it to the base for delivery */
                    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                        "%s DELIVERING msg tag = %d scon_handle = %d",
                                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                        peer->recv_msg->hdr.tag,
                                        peer->recv_msg->hdr.scon_handle);
                    PT2PT_POST_MESSAGE(&peer->recv_msg->hdr.origin,
                                          peer->recv_msg->hdr.tag,
                                          peer->recv_msg->hdr.scon_handle,
                                          peer->recv_msg->data,
                                          peer->recv_msg->hdr.nbytes);
                    SCON_RELEASE(peer->recv_msg);
                } else {
                    /* promote this to the PT2PT as some other transport might
                     * be the next best hop */
                    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                        "%s TCP PROMOTING ROUTED MESSAGE FOR %s TO PT2PT",
                                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                        SCON_PRINT_PROC(&peer->recv_msg->hdr.dst));
                    snd = SCON_NEW(scon_send_t);
                    snd->buf = malloc(sizeof(scon_buffer_t));
                    scon_buffer_construct(snd->buf);
                    scon_buffer_load(snd->buf, (void*) peer->recv_msg->data, peer->recv_msg->hdr.nbytes);
                    snd->dst.rank = peer->recv_msg->hdr.dst.rank;
                    strncpy(snd->dst.job_name, peer->recv_msg->hdr.dst.job_name,
                            SCON_MAX_JOBLEN);

                    snd->origin.rank = peer->recv_msg->hdr.origin.rank;
                    strncpy(snd->origin.job_name, peer->recv_msg->hdr.origin.job_name,
                            SCON_MAX_JOBLEN);
                    snd->tag = peer->recv_msg->hdr.tag;
                    snd->scon_handle = peer->recv_msg->hdr.scon_handle;
                   // snd->data = peer->recv_msg->data;
                    snd->cbfunc = NULL;
                    snd->cbdata = NULL;
                    /* activate the PT2PT send state */
                    PT2PT_SEND_MESSAGE(snd);
                    /* protect the data */
                    peer->recv_msg->data = NULL;
                    /* cleanup */
                    SCON_RELEASE(peer->recv_msg);
                }
                peer->recv_msg = NULL;
                return;
            } else if (SCON_ERR_RESOURCE_BUSY == rc ||
                       SCON_ERR_WOULD_BLOCK == rc) {
                /* exit this event and let the event lib progress */
                return;
            } else {
                // report the error
                scon_output(0, "%s-%s scon_pt2pt_tcp_peer_recv_handler: unable to recv message",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&(peer->name)));
                /* turn off the recv event */
                scon_event_del(&peer->recv_event);
                return;
            }
        }
        break;
    default:
        scon_output(0, "%s-%s scon_pt2pt_tcp_peer_recv_handler: invalid socket state(%d)",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&(peer->name)),
                    peer->state);
        // scon_pt2pt_tcp_peer_close(peer);
        break;
    }
}

static void snd_cons(scon_pt2pt_tcp_send_t *ptr)
{
    ptr->msg = NULL;
    ptr->data = NULL;
    ptr->hdr_sent = false;
    ptr->iovnum = 0;
    ptr->sdptr = NULL;
    ptr->sdbytes = 0;
}
/* we don't destruct any RML msg that is
 * attached to our send as the RML owns
 * that memory. However, if we relay a
 * msg, the data in the relay belongs to
 * us and must be free'd
 */
static void snd_des(scon_pt2pt_tcp_send_t *ptr)
{
    if (NULL != ptr->data) {
        free(ptr->data);
    }
}
SCON_CLASS_INSTANCE(scon_pt2pt_tcp_send_t,
                   scon_list_item_t,
                   snd_cons, snd_des);

static void rcv_cons(scon_pt2pt_tcp_recv_t *ptr)
{
    ptr->hdr_recvd = false;
    ptr->rdptr = NULL;
    ptr->rdbytes = 0;
}
SCON_CLASS_INSTANCE(scon_pt2pt_tcp_recv_t,
                   scon_list_item_t,
                   rcv_cons, NULL);

static void err_cons(scon_pt2pt_tcp_msg_error_t *ptr)
{
    ptr->rmsg = NULL;
    ptr->snd = NULL;
}
SCON_CLASS_INSTANCE(scon_pt2pt_tcp_msg_error_t,
                   scon_object_t,
                   err_cons, NULL);

