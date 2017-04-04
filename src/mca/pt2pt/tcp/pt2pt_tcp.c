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
 * Copyright (c) 2009-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2013-2016 Intel, Inc.  All rights reserved.
 * Copyright (c) 2016      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include "scon_config.h"
#include "scon_types.h"
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
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <ctype.h>

#include "src/util/show_help.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/include/scon_socket_errno.h"
#include "src/util/if.h"
#include "src/util/net.h"
#include "src/util/argv.h"
#include "src/class/scon_hash_table.h"
#include "src/runtime/scon_progress_threads.h"


#include "src/mca/topology/topology.h"
#include "src/util/name_fns.h"
#include "src/util/parse_options.h"
#include "src/include/scon_globals.h"

#include "src/mca/pt2pt/tcp/pt2pt_tcp.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_component.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_peer.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_common.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_connection.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_ping.h"


static int send_nb(scon_send_t *msg);

scon_pt2pt_tcp_module_t scon_pt2pt_tcp_module = {
    .base =
    {
        .send = send_nb,
    }
};

/*
 * Local utility functions
 */
static void recv_handler(int sd, short flags, void* user);



/*
 * Initialize global variables used w/in this module.
 */
void scon_pt2pt_tcp_init(void)
{
    /* setup the module's state variables */
    SCON_CONSTRUCT(&scon_pt2pt_tcp_module.peers, scon_hash_table_t);
    scon_hash_table_init(&scon_pt2pt_tcp_module.peers, 32);
    scon_pt2pt_tcp_module.ev_active = false;

    if (scon_pt2pt_base.num_threads) {
        /* if we are to use independent progress threads at
         * the module level, start it now
         */
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s STARTING TCP PROGRESS THREAD",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        scon_pt2pt_tcp_module.ev_base = scon_progress_thread_init("pt2pt_tcp");
    }
    else {
        scon_pt2pt_tcp_module.ev_base = scon_pt2pt_base.pt2pt_evbase;
    }
}

/*
 * Module cleanup.
 */
void scon_pt2pt_tcp_fini(void)
{
    uint64_t ui64;
    scon_pt2pt_tcp_peer_t *peer;

    /* cleanup all peers */
    SCON_HASH_TABLE_FOREACH(ui64, uint64, peer, &scon_pt2pt_tcp_module.peers) {
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s RELEASING PEER OBJ %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            (NULL == peer) ? "NULL" : SCON_PRINT_PROC(&peer->name));
        if (NULL != peer) {
            SCON_RELEASE(peer);
        }
    }
    SCON_DESTRUCT(&scon_pt2pt_tcp_module.peers);

    if (scon_pt2pt_tcp_module.ev_active) {
        /* if we used an independent progress thread at
         * the module level, stop it now
         */
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s STOPPING TCP PROGRESS THREAD",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        /* stop the progress thread */
        scon_progress_thread_finalize("pt2pt_tcp");
        /* release the event base */
        scon_event_base_free(scon_pt2pt_tcp_module.ev_base);
    }
}

/* Called by scon_pt2pt_tcp_accept() and connection_handler() on
 * a socket that has been accepted.  This call finishes processing the
 * socket, including setting socket options and registering for the
 * PT2PT-level connection handshake.  Used in both the threaded and
 * event listen modes.
 */
 void scon_pt2pt_tcp_accept_connection(const int accepted_fd,
                              const struct sockaddr *addr)
{
    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s accept_connection: %s:%d\n",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        scon_net_get_hostname(addr),
                        scon_net_get_port(addr));
    scon_output(0, "%s scon_pt2pt_tcp_accept_connection: %s:%d\n",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        scon_net_get_hostname(addr),
                        scon_net_get_port(addr));

   /* setup socket options */
    scon_pt2pt_tcp_set_socket_options(accepted_fd);

    /* use a one-time event to wait for receipt of peer's
     *  process ident message to complete this connection
     */
    SCON_ACTIVATE_TCP_ACCEPT_STATE(accepted_fd, addr, recv_handler);
}

/* the host in this case is always in "dot" notation, and
 * thus we do not need to do a DNS lookup to convert it */
static int parse_uri(const uint16_t af_family,
                     const char* host,
                     const char *port,
                     struct sockaddr_storage* inaddr)
{
    struct sockaddr_in *in;

    if (AF_INET == af_family) {
        memset(inaddr, 0, sizeof(struct sockaddr_in));
        in = (struct sockaddr_in*) inaddr;
        in->sin_family = AF_INET;
        in->sin_addr.s_addr = inet_addr(host);
        if (in->sin_addr.s_addr == INADDR_NONE) {
            return SCON_ERR_BAD_PARAM;
        }
        ((struct sockaddr_in*) inaddr)->sin_port = htons(atoi(port));
    }
#if SCON_ENABLE_IPV6
    else if (AF_INET6 == af_family) {
        struct sockaddr_in6 *in6;
        memset(inaddr, 0, sizeof(struct sockaddr_in6));
        in6 = (struct sockaddr_in6*) inaddr;

        if (0 == inet_pton(AF_INET6, host, (void*)&in6->sin6_addr)) {
            scon_output (0, "pt2pt_tcp_parse_uri: Could not convert %s\n", host);
            return SCON_ERR_BAD_PARAM;
        }
    }
#endif
    else {
        return SCON_ERR_NOT_SUPPORTED;
    }
    return SCON_SUCCESS;
}

/*
 * Record listening address for this peer - the connection
 * is created on first-send
 */
static void process_set_peer(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_peer_op_t *pop = (scon_pt2pt_tcp_peer_op_t*)cbdata;
    scon_pt2pt_tcp_peer_t *peer;
    int rc=SCON_SUCCESS;
    uint64_t ui64;
    scon_pt2pt_tcp_addr_t *maddr;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s:tcp:processing set_peer cmd",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));

    if (AF_INET != pop->af_family) {
            scon_output_verbose(20, scon_pt2pt_base_framework.framework_output,
                            "%s NOT AF_INET", SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        goto cleanup;
    }

    if (NULL == (peer = scon_pt2pt_tcp_peer_lookup(&pop->peer))) {
        peer = SCON_NEW(scon_pt2pt_tcp_peer_t);
        strncpy(peer->name.job_name, pop->peer.job_name, SCON_MAX_JOBLEN);
        peer->name.rank = pop->peer.rank;
        scon_util_convert_process_name_to_uint64(&ui64, &pop->peer);
        scon_output_verbose(20, scon_pt2pt_base_framework.framework_output,
                            "%s SET_PEER ADDING PEER %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&pop->peer));
        scon_output(0, "process_set_peer %s setting tcp hash table %p, key %llu, value %p",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    (void*)&scon_pt2pt_tcp_module.peers,
                    ui64, (void*) peer);
        if (SCON_SUCCESS != scon_hash_table_set_value_uint64(&scon_pt2pt_tcp_module.peers, ui64, peer)) {
            SCON_RELEASE(peer);
            return;
        }
        /* we have to initiate the connection because otherwise the
         * daemon has no way to communicate to us via this component
         * as the app doesn't have a listening port */
        peer->state = SCON_PT2PT_TCP_CONNECTING;
        SCON_ACTIVATE_TCP_CONN_STATE(peer, scon_pt2pt_tcp_peer_try_connect);

    }

    maddr = SCON_NEW(scon_pt2pt_tcp_addr_t);
    if (SCON_SUCCESS != (rc = parse_uri(pop->af_family, pop->net, pop->port, (struct sockaddr_storage*) &(maddr->addr)))) {
        SCON_ERROR_LOG(rc);
        SCON_RELEASE(maddr);
        goto cleanup;
    }

    scon_output_verbose(20, scon_pt2pt_base_framework.framework_output,
                        "%s set_peer: peer %s is listening on net %s port %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&pop->peer),
                        (NULL == pop->net) ? "NULL" : pop->net,
                        (NULL == pop->port) ? "NULL" : pop->port);
    scon_list_append(&peer->addrs, &maddr->super);

  cleanup:
    SCON_RELEASE(pop);
}

void scon_pt2pt_tcp_set_peer(const scon_proc_t *name,
                             const uint16_t af_family,
                             const char *net, const char *ports)
{
    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s:tcp set addr for peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(name));

    /* have to push this into our event base for processing */
    SCON_ACTIVATE_TCP_PEER_OP(name, af_family, net, ports, process_set_peer);
}


/* API functions */
#if 0
static void process_ping(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_ping_t *op = (scon_pt2pt_tcp_ping_t*)cbdata;
    scon_pt2pt_tcp_peer_t *peer;

    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s:[%s:%d] processing ping to peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        __FILE__, __LINE__,
                        SCON_PRINT_PROC(&op->peer));

    /* do we know this peer? */
    if (NULL == (peer = scon_pt2pt_tcp_peer_lookup(&op->peer))) {
        /* push this back to the component so it can try
         * another module within this transport. If no
         * module can be found, the component can push back
         * to the framework so another component can try
         */
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s:[%s:%d] hop %s unknown",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            __FILE__, __LINE__,
                            SCON_PRINT_PROC(&op->peer));
        SCON_ACTIVATE_TCP_MSG_ERROR(NULL, NULL, &op->peer, scon_pt2pt_tcp_component_hop_unknown);
        goto cleanup;
    }

    /* if we are already connected, there is nothing to do */
    if (scon_pt2pt_TCP_CONNECTED == peer->state) {
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s:[%s:%d] already connected to peer %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            __FILE__, __LINE__,
                            SCON_PRINT_PROC(&op->peer));
        goto cleanup;
    }

    /* if we are already connecting, there is nothing to do */
    if (scon_pt2pt_TCP_CONNECTING == peer->state ||
        scon_pt2pt_TCP_CONNECT_ACK == peer->state) {
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s:[%s:%d] already connecting to peer %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            __FILE__, __LINE__,
                            SCON_PRINT_PROC(&op->peer));
        goto cleanup;
    }

    /* attempt the connection */
    peer->state = scon_pt2pt_TCP_CONNECTING;
    SCON_ACTIVATE_TCP_CONN_STATE(peer, scon_pt2pt_tcp_peer_try_connect);

 cleanup:
    SCON_RELEASE(op);
}

static void ping(const scon_proc_t *proc)
{
    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s:[%s:%d] pinging peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        __FILE__, __LINE__,
                        SCON_PRINT_PROC(proc));

    /* push this into our event base for processing */
    SCON_ACTIVATE_TCP_PING(proc, process_ping);
}
#endif
static void process_send(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_msg_op_t *op = (scon_pt2pt_tcp_msg_op_t*)cbdata;
    scon_comm_scon_t *scon = scon_comm_base_get_scon (op->msg->scon_handle);
    scon_pt2pt_tcp_peer_t *peer;
    scon_proc_t hop;

    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s:[%s:%d] processing send to peer %s:%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        __FILE__, __LINE__,
                        SCON_PRINT_PROC(&op->msg->dst), op->msg->tag);

    /* do we have a route to this peer (could be direct)? */
    hop = scon->topology_module->api.get_nexthop(&scon->topology_module->topology, &op->msg->dst);
    /* do we know this hop? */
    if (NULL == (peer = scon_pt2pt_tcp_peer_lookup(&hop))) {
        /* push this back to the component so it can try
         * another module within this transport. If no
         * module can be found, the component can push back
         * to the framework so another component can try
         */
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s:[%s:%d] hop %s unknown",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            __FILE__, __LINE__,
                            SCON_PRINT_PROC(&hop));
        scon_output(0,   "%s:[%s:%d] hop %s unknown",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            __FILE__, __LINE__,
                            SCON_PRINT_PROC(&hop));
        SCON_ACTIVATE_TCP_NO_ROUTE(op->msg, &hop, scon_pt2pt_tcp_component_no_route);
        goto cleanup;
    }

    /* add the msg to the hop's send queue */
    if (SCON_PT2PT_TCP_CONNECTED == peer->state) {
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s tcp:send_nb: already connected to %s - queueing for send",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&peer->name));
        SCON_PT2PT_TCP_QUEUE_SEND(op->msg, peer);
        goto cleanup;
    }

    /* add the message to the queue for sending after the
     * connection is formed
     */
    SCON_PT2PT_TCP_QUEUE_PENDING(op->msg, peer);

    if (SCON_PT2PT_TCP_CONNECTING != peer->state &&
        SCON_PT2PT_TCP_CONNECT_ACK != peer->state) {
        /* we have to initiate the connection - again, we do not
         * want to block while the connection is created.
         * So throw us into an event that will create
         * the connection via a mini-state-machine :-)
         */
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s tcp:send_nb: initiating connection to %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&peer->name));
        peer->state = SCON_PT2PT_TCP_CONNECTING;
        SCON_ACTIVATE_TCP_CONN_STATE(peer, scon_pt2pt_tcp_peer_try_connect);
    }

 cleanup:
    SCON_RELEASE(op);
}

static int send_nb(scon_send_t *msg)
{
    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:send_nb to peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&msg->dst));

    /* push this into our event base for processing */
    SCON_ACTIVATE_TCP_POST_SEND(msg, process_send);
    return SCON_SUCCESS;
}

static void process_resend(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_msg_error_t *op = (scon_pt2pt_tcp_msg_error_t*)cbdata;
    scon_pt2pt_tcp_peer_t *peer;

    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s:tcp processing resend to peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&op->hop));

    /* do we know this peer? */
    if (NULL == (peer = scon_pt2pt_tcp_peer_lookup(&op->hop))) {
        /* push this back to the component so it can try
         * another module within this transport. If no
         * module can be found, the component can push back
         * to the framework so another component can try
         */
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s:[%s:%d] peer %s unknown",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            __FILE__, __LINE__,
                            SCON_PRINT_PROC(&op->hop));
        SCON_ACTIVATE_TCP_MSG_ERROR(op->snd, NULL, &op->hop, scon_pt2pt_tcp_component_hop_unknown);
        goto cleanup;
    }

    /* add the msg to this peer's send queue */
    if (SCON_PT2PT_TCP_CONNECTED == peer->state) {
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s tcp:resend: already connected to %s - queueing for send",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&peer->name));
        SCON_PT2PT_TCP_QUEUE_MSG(peer, op->snd, true);
        goto cleanup;
    }

    if (SCON_PT2PT_TCP_CONNECTING != peer->state &&
        SCON_PT2PT_TCP_CONNECT_ACK != peer->state) {
        /* add the message to the queue for sending after the
         * connection is formed
         */
        SCON_PT2PT_TCP_QUEUE_MSG(peer, op->snd, false);
        /* we have to initiate the connection - again, we do not
         * want to block while the connection is created.
         * So throw us into an event that will create
         * the connection via a mini-state-machine :-)
         */
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s tcp:send_nb: initiating connection to %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&peer->name));
        peer->state = SCON_PT2PT_TCP_CONNECTING;
        SCON_ACTIVATE_TCP_CONN_STATE(peer, scon_pt2pt_tcp_peer_try_connect);
    }

 cleanup:
    SCON_RELEASE(op);
}

static void resend(struct scon_pt2pt_tcp_msg_error_t *mp)
{
    scon_pt2pt_tcp_msg_error_t *mop = (scon_pt2pt_tcp_msg_error_t*)mp;

    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:resend to peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&mop->hop));

    /* push this into our event base for processing */
    SCON_ACTIVATE_TCP_POST_RESEND(mop, process_resend);
}

/*
 * Event callback when there is data available on the registered
 * socket to recv.  This is called for the listen sockets to accept an
 * incoming connection, on new sockets trying to complete the software
 * connection process, and for probes.  Data on an established
 * connection is handled elsewhere.
 */
static void recv_handler(int sd, short flg, void *cbdata)
{
    scon_pt2pt_tcp_conn_op_t *op = (scon_pt2pt_tcp_conn_op_t*)cbdata;
    int flags;
    uint64_t *ui64;
    scon_pt2pt_tcp_hdr_t hdr;
    scon_pt2pt_tcp_peer_t *peer;
    int rc;

    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s:tcp:recv:handler called",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));
    scon_output(0, "%s:tcp:recv:handler called",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));

    /* get the handshake */
    if (SCON_SUCCESS != (rc = scon_pt2pt_tcp_peer_recv_connect_ack(NULL, sd, &hdr))) {
        scon_output(0, "%s tcp:recv:handler: scon_pt2pt_tcp_peer_recv_connect_ack failed status %d",
                      SCON_PRINT_PROC(SCON_PROC_MY_NAME), rc);
        goto cleanup;
    }
    /* finish processing ident */
    if (SCON_PT2PT_TCP_IDENT == hdr.type) {
        if (NULL == (peer = scon_pt2pt_tcp_peer_lookup(&hdr.origin))) {
            /* should never happen */
            scon_pt2pt_tcp_peer_close(peer);
            goto cleanup;
        }
        /* set socket up to be non-blocking */
        if ((flags = fcntl(sd, F_GETFL, 0)) < 0) {
            scon_output(0, "%s scon_pt2pt_tcp_recv_connect: fcntl(F_GETFL) failed: %s (%d)",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), strerror(scon_socket_errno), scon_socket_errno);
        } else {
            flags |= O_NONBLOCK;
            if (fcntl(sd, F_SETFL, flags) < 0) {
                scon_output(0, "%s scon_pt2pt_tcp_recv_connect: fcntl(F_SETFL) failed: %s (%d)",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME), strerror(scon_socket_errno), scon_socket_errno);
            }
        }
        /* is the peer instance willing to accept this connection */
        peer->sd = sd;
        if (scon_pt2pt_tcp_peer_accept(peer) == false) {
            if (PT2PT_TCP_DEBUG_CONNECT <= scon_output_get_verbosity(scon_pt2pt_base_framework.framework_output)) {
                scon_output(0, "%s-%s scon_pt2pt_tcp_recv_connect: "
                            "rejected connection from %s connection state %d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(&(peer->name)),
                            SCON_PRINT_PROC(&(hdr.origin)),
                            peer->state);
            }
            scon_output(0, "%s-%s scon_pt2pt_tcp_recv_connect: "
                        "rejected connection from %s connection state %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&(peer->name)),
                        SCON_PRINT_PROC(&(hdr.origin)),
                        peer->state);
            CLOSE_THE_SOCKET(sd);
            //ui64 = (uint64_t*)(&peer->name);
            scon_util_convert_process_name_to_uint64(ui64, &peer->name);
            (void)scon_hash_table_set_value_uint64(&scon_pt2pt_tcp_module.peers, (*ui64), NULL);
            SCON_RELEASE(peer);
        }
    }

 cleanup:
    SCON_RELEASE(op);
}

