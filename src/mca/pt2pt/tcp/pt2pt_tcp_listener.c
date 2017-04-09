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
 * Copyright (c) 2013-2016 Intel, Inc.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
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
#include "scon_types.h"

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
#include "src/include/scon_globals.h"
#include "src/util/if.h"
#include "src/util/net.h"
#include "src/util/argv.h"
#include "src/util/fd.h"
#include "src/class/scon_hash_table.h"
#include "src/class/scon_list.h"

#include "src/util/name_fns.h"
#include "src/util/parse_options.h"

#include "src/mca/pt2pt/tcp/pt2pt_tcp.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_component.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_peer.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_connection.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_listener.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_common.h"


/* To do :: need to make this per SCON? */
bool scon_static_ports = false;
static void connection_event_handler(int incoming_sd, short flags, void* cbdata);
static void connection_event_handler(int sd, short flags, void* cbdata);

static int create_listen(void);
#if SCON_ENABLE_IPV6
static int create_listen6(void);
#endif
/*
static void* listen_thread(scon_object_t *obj);
static void connection_handler(int sd, short flags, void* cbdata);
*/

/*
 * Component initialization - create a module for each available
 * TCP interface and initialize the static resources associated
 * with that module.
 *
 * Also initializes the list of devices that will be used/supported by
 * the module, using the if_include and if_exclude variables.  This is
 * the only place that this sorting should occur -- all other places
 * should use the tcp_avaiable_devices list.  This is a change from
 * previous versions of this component.
 */
int scon_pt2pt_tcp_start_listening(void)
{
    int rc;
    scon_pt2pt_tcp_listener_t *listener;

    /* if we don't have any TCP interfaces, we shouldn't be here */
    if (NULL == mca_pt2pt_tcp_component.ipv4conns
#if SCON_ENABLE_IPV6
        && NULL == mca_pt2pt_tcp_component.ipv6conns
#endif
        ) {
        SCON_ERROR_LOG(SCON_ERR_NOT_FOUND);
        return SCON_ERR_NOT_FOUND;
    }

    /* create listen socket(s) for incoming connection attempts */
    if (SCON_SUCCESS != (rc = create_listen())) {
        SCON_ERROR_LOG(rc);
        return rc;
    }

#if SCON_ENABLE_IPV6
    /* create listen socket(s) for incoming connection attempts */
    if (SCON_SUCCESS != (rc = create_listen6())) {
        SCON_ERROR_LOG(rc);
        return rc;
    }
#endif

    /* if I am the master, start a listening thread so we can
     * harvest connection requests as rapidly as possible
     */
#if 0
   if (0 == SCON_PROC_MY_RANK) {
        if (0 > pipe(mca_pt2pt_tcp_component.stop_thread)) {
            SCON_ERROR_LOG(SCON_ERR_OUT_OF_RESOURCE);
            return SCON_ERR_OUT_OF_RESOURCE;
        }

        /* Make sure the pipe FDs are set to close-on-exec so that
           they don't leak into children */
        if (scon_fd_set_cloexec(mca_pt2pt_tcp_component.stop_thread[0]) != SCON_SUCCESS ||
            scon_fd_set_cloexec(mca_pt2pt_tcp_component.stop_thread[1]) != SCON_SUCCESS) {
            close(mca_pt2pt_tcp_component.stop_thread[0]);
            close(mca_pt2pt_tcp_component.stop_thread[1]);
            SCON_ERROR_LOG(SCON_ERR_IN_ERRNO);
            return SCON_ERR_IN_ERRNO;
        }

        mca_pt2pt_tcp_component.listen_thread_active = true;
        mca_pt2pt_tcp_component.listen_thread.t_run = listen_thread;
        mca_pt2pt_tcp_component.listen_thread.t_arg = NULL;
        if (SCON_SUCCESS != (rc = scon_progressthread_start(&mca_pt2pt_tcp_component.listen_thread))) {
            SCON_ERROR_LOG(rc);
            scon_output(0, "%s Unable to start listen thread", SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        }
        return rc;
    }
#endif
    /* otherwise, setup to listen via the event lib */
    SCON_LIST_FOREACH(listener, &mca_pt2pt_tcp_component.listeners, scon_pt2pt_tcp_listener_t) {
        listener->ev_active = true;
        scon_event_set(scon_globals.evbase, &listener->event,
                       listener->sd,
                       SCON_EV_READ|SCON_EV_PERSIST,
                       connection_event_handler,
                       0);
        scon_event_set_priority(&listener->event, SCON_ERROR_PRI);
        scon_event_add(&listener->event, 0);
    }

    return SCON_SUCCESS;
}

/*
 * Create an IPv4 listen socket and bind to all interfaces.
 *
 * At one time, this also registered a callback with the event library
 * for when connections were received on the listen socket.  This is
 * no longer the case -- the caller must register any events required.
 *
 * Called by both the threaded and event based listen modes.
 */
static int create_listen(void)
{
    int flags, i;
    uint16_t port=0;
    struct sockaddr_storage inaddr;
    scon_socklen_t addrlen;
    char **ports=NULL;
    int sd = -1;
    char *tconn;
    scon_pt2pt_tcp_listener_t *conn;
    if (NULL != mca_pt2pt_tcp_component.tcp_static_ports) {
        /* if static ports were provided, take the
         * first entry in the list
         */
        scon_argv_append_nosize(&ports, mca_pt2pt_tcp_component.tcp_static_ports[0]);
        /* flag that we are using static ports */
        scon_static_ports = true;
    } else if (NULL != mca_pt2pt_tcp_component.tcp_dyn_ports) {
        /* take the entire range */
        ports = scon_argv_copy(mca_pt2pt_tcp_component.tcp_dyn_ports);
        scon_static_ports = false;
    } else {
        /* flag the system to dynamically take any available port */
        scon_argv_append_nosize(&ports, "0");
        scon_static_ports = false;
    }
    /* bozo check - this should be impossible, but... */
    if (NULL == ports) {
        return SCON_ERROR;
    }

    /* get the address info for this interface */
    ((struct sockaddr_in*) &inaddr)->sin_family = AF_INET;
    ((struct sockaddr_in*) &inaddr)->sin_addr.s_addr = INADDR_ANY;
    addrlen = sizeof(struct sockaddr_in);

    /* loop across all the specified ports, establishing a socket
     * for each one - note that application procs will ONLY have
     * one socket, but that the master process may have multiple
     * sockets to support more flexible wireup protocols
     */
    for (i=0; i < scon_argv_count(ports); i++) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "%s attempting to bind to IPv4 port %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            ports[i]);
        /* get the port number */
        port = strtol(ports[i], NULL, 10);
        /* convert it to network-byte-order */
        port = htons(port);

        ((struct sockaddr_in*) &inaddr)->sin_port = port;

        /* create a listen socket for incoming connections on this port */
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if (sd < 0) {
            if (EAFNOSUPPORT != scon_socket_errno) {
                scon_output(0,"mca_pt2pt_tcp_component_init: socket() failed: %s (%d)",
                            strerror(scon_socket_errno), scon_socket_errno);
            }
            scon_argv_free(ports);
            return SCON_ERR_IN_ERRNO;
        }

        /* Enable/disable reusing ports */
        if (scon_static_ports) {
            flags = 1;
        } else {
            flags = 0;
        }
        if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&flags, sizeof(flags)) < 0) {
            scon_output(0, "scon_pt2pt_tcp_create_listen: unable to set the "
                        "SO_REUSEADDR option (%s:%d)\n",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }

        /* Set the socket to close-on-exec so that no children inherit
           this FD */
        if (scon_fd_set_cloexec(sd) != SCON_SUCCESS) {
            scon_output(0, "scon_pt2pt_tcp_create_listen: unable to set the "
                        "listening socket to CLOEXEC (%s:%d)\n",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }

        if (bind(sd, (struct sockaddr*)&inaddr, addrlen) < 0) {
            if( (EADDRINUSE == scon_socket_errno) || (EADDRNOTAVAIL == scon_socket_errno) ) {
                continue;
            }
            scon_output(0, "%s bind() failed for port %d: %s (%d)",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)ntohs(port),
                        strerror(scon_socket_errno),
                        scon_socket_errno );
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }
        /* resolve assigned port */
        if (getsockname(sd, (struct sockaddr*)&inaddr, &addrlen) < 0) {
            scon_output(0, "scon_pt2pt_tcp_create_listen: getsockname(): %s (%d)",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }

        /* setup listen backlog to maximum allowed by kernel */
        if (listen(sd, SOMAXCONN) < 0) {
            scon_output(0, "mca_pt2pt_tcp_component_init: listen(): %s (%d)",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }

        /* set socket up to be non-blocking, otherwise accept could block */
        if ((flags = fcntl(sd, F_GETFL, 0)) < 0) {
            scon_output(0, "mca_pt2pt_tcp_component_init: fcntl(F_GETFL) failed: %s (%d)",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }
        flags |= O_NONBLOCK;
        if (fcntl(sd, F_SETFL, flags) < 0) {
            scon_output(0, "mca_pt2pt_tcp_component_init: fcntl(F_SETFL) failed: %s (%d)",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }

        /* add this port to our connections */
        conn = SCON_NEW(scon_pt2pt_tcp_listener_t);
        conn->sd = sd;
        conn->port = ntohs(((struct sockaddr_in*) &inaddr)->sin_port);
        scon_list_append(&mca_pt2pt_tcp_component.listeners, &conn->item);
        /* and to our ports */
        asprintf(&tconn, "%d", ntohs(((struct sockaddr_in*) &inaddr)->sin_port));
        scon_argv_append_nosize(&mca_pt2pt_tcp_component.ipv4ports, tconn);
        free(tconn);
        if (PT2PT_TCP_DEBUG_CONNECT <= scon_output_get_verbosity(scon_pt2pt_base_framework.framework_output)) {
            port = ntohs(((struct sockaddr_in*) &inaddr)->sin_port);
            scon_output(0, "%s assigned IPv4 port %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), port);
        }
        /*** TO DO check if we need to bind to multiple static ports  ***/
    }
    /* done with this, so release it */
    scon_argv_free(ports);

    if (0 == scon_list_get_size(&mca_pt2pt_tcp_component.listeners)) {
        /* cleanup */
        if (0 <= sd) {
            CLOSE_THE_SOCKET(sd);
        }
        return SCON_ERR_SOCKET_NOT_AVAILABLE;
    }
    return SCON_SUCCESS;
}

#if SCON_ENABLE_IPV6
/*
 * Create an IPv6 listen socket and bind to all interfaces.
 *
 * At one time, this also registered a callback with the event library
 * for when connections were received on the listen socket.  This is
 * no longer the case -- the caller must register any events required.
 *
 * Called by both the threaded and event based listen modes.
 */
static int create_listen6(void)
{
    int flags, i;
    uint16_t port=0;
    struct sockaddr_storage inaddr;
    scon_socklen_t addrlen;
    char **ports=NULL;
    int sd;
    char *tconn;
    scon_pt2pt_tcp_listener_t *conn;
    if (NULL != mca_pt2pt_tcp_component.tcp_static_ports) {
        /* if static ports were provided, take the
         * first entry in the list
         */
        scon_argv_append_nosize(&ports, mca_pt2pt_tcp_component.tcp_static_ports[0]);
        /* flag that we are using static ports */
        scon_static_ports = true;
    } else if (NULL != mca_pt2pt_tcp_component.tcp_dyn_ports) {
        /* take the entire range */
        ports = scon_argv_copy(mca_pt2pt_tcp_component.tcp_dyn_ports);
        scon_static_ports = false;
    } else {
        /* flag the system to dynamically take any available port */
        scon_argv_append_nosize(&ports, "0");
        scon_static_ports = false;
    }
    /* bozo check - this should be impossible, but... */
    if (NULL == ports) {
        return SCON_ERROR;
    }

    /* get the address info for this interface */
    ((struct sockaddr_in6*) &inaddr)->sin6_family = AF_INET6;
    ((struct sockaddr_in6*) &inaddr)->sin6_addr = in6addr_any;
    addrlen = sizeof(struct sockaddr_in6);

    /* loop across all the specified ports, establishing a socket
     * for each one - note that application procs will ONLY have
     * one socket, but that orterun and daemons will have multiple
     * sockets to support more flexible wireup protocols
     */
    for (i=0; i < scon_argv_count(ports); i++) {
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                            "%s attempting to bind to IPv6 port %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            ports[i]);
        /* get the port number */
        port = strtol(ports[i], NULL, 10);
        /* convert it to network-byte-order */
        port = htons(port);

        ((struct sockaddr_in6*) &inaddr)->sin6_port = port;

        /* create a listen socket for incoming connections on this port */
        sd = socket(AF_INET6, SOCK_STREAM, 0);
        if (sd < 0) {
            if (EAFNOSUPPORT != scon_socket_errno) {
                scon_output(0,"mca_pt2pt_tcp_component_init: socket() failed: %s (%d)",
                            strerror(scon_socket_errno), scon_socket_errno);
            }
            return SCON_ERR_IN_ERRNO;
        }
        /* Set the socket to close-on-exec so that no children inherit
           this FD */
        if (scon_fd_set_cloexec(sd) != SCON_SUCCESS) {
            scon_output(0, "scon_pt2pt_tcp_create_listen6: unable to set the "
                        "listening socket to CLOEXEC (%s:%d)\n",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }

        /* Enable/disable reusing ports */
        if (scon_static_ports) {
            flags = 1;
        } else {
            flags = 0;
        }
        if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&flags, sizeof(flags)) < 0) {
            scon_output(0, "scon_pt2pt_tcp_create_listen: unable to set the "
                        "SO_REUSEADDR option (%s:%d)\n",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }

        if (bind(sd, (struct sockaddr*)&inaddr, addrlen) < 0) {
            if( (EADDRINUSE == scon_socket_errno) || (EADDRNOTAVAIL == scon_socket_errno) ) {
                continue;
            }
            scon_output(0, "%s bind() failed for port %d: %s (%d)",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)ntohs(port),
                        strerror(scon_socket_errno),
                        scon_socket_errno );
            CLOSE_THE_SOCKET(sd);
            scon_argv_free(ports);
            return SCON_ERROR;
        }
        /* resolve assigned port */
        if (getsockname(sd, (struct sockaddr*)&inaddr, &addrlen) < 0) {
            scon_output(0, "scon_pt2pt_tcp_create_listen: getsockname(): %s (%d)",
                        strerror(scon_socket_errno), scon_socket_errno);
            CLOSE_THE_SOCKET(sd);
            return SCON_ERROR;
        }

        /* setup listen backlog to maximum allowed by kernel */
        if (listen(sd, SOMAXCONN) < 0) {
            scon_output(0, "mca_pt2pt_tcp_component_init: listen(): %s (%d)",
                        strerror(scon_socket_errno), scon_socket_errno);
            return SCON_ERROR;
        }

        /* set socket up to be non-blocking, otherwise accept could block */
        if ((flags = fcntl(sd, F_GETFL, 0)) < 0) {
            scon_output(0, "mca_pt2pt_tcp_component_init: fcntl(F_GETFL) failed: %s (%d)",
                        strerror(scon_socket_errno), scon_socket_errno);
            return SCON_ERROR;
        }
        flags |= O_NONBLOCK;
        if (fcntl(sd, F_SETFL, flags) < 0) {
            scon_output(0, "mca_pt2pt_tcp_component_init: fcntl(F_SETFL) failed: %s (%d)",
                        strerror(scon_socket_errno), scon_socket_errno);
            return SCON_ERROR;
        }

        /* add this port to our connections */
        conn = SCON_NEW(scon_pt2pt_tcp_listener_t);
        conn->tcp6 = true;
        conn->sd = sd;
        conn->port = ntohs(((struct sockaddr_in6*) &inaddr)->sin6_port);
        scon_list_append(&mca_pt2pt_tcp_component.listeners, &conn->item);
        /* and to our ports */
        asprintf(&tconn, "%d", ntohs(((struct sockaddr_in6*) &inaddr)->sin6_port));
        scon_argv_append_nosize(&mca_pt2pt_tcp_component.ipv6ports, tconn);
        free(tconn);
        if (PT2PT_TCP_DEBUG_CONNECT <= scon_output_get_verbosity(scon_pt2pt_base_framework.framework_output)) {
            scon_output(0, "%s assigned IPv6 port %d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        (int)ntohs(((struct sockaddr_in6*) &inaddr)->sin6_port));
        }

        /*** TO DO check if we need to bind to multiple static ports  ***/
    }
    if (0 == scon_list_get_size(&mca_pt2pt_tcp_component.listeners)) {
        /* cleanup */
        CLOSE_THE_SOCKET(sd);
        scon_argv_free(ports);
        return SCON_ERR_SOCKET_NOT_AVAILABLE;
    }

    /* done with this, so release it */
    scon_argv_free(ports);
    return SCON_SUCCESS;
}
#endif

#if 0
/*
 * The listen thread created when listen_mode is threaded.  Accepts
 * incoming connections and places them in a queue for further
 * processing
 *
 * Runs until scon_pt2pt_tcp_compnent.shutdown is set to true.
 */
static void* listen_thread(scon_object_t *obj)
{
    int rc, max, accepted_connections, sd;
    scon_socklen_t addrlen = sizeof(struct sockaddr_storage);
    scon_pt2pt_tcp_pending_connection_t *pending_connection;
    struct timeval timeout;
    fd_set readfds;
    scon_pt2pt_tcp_listener_t *listener;
    /* only execute during the initial VM startup stage - once
     * all the initial daemons have reported in, we will revert
     * to the event method for handling any further connections
     * so as to minimize overhead
     */
    while (mca_pt2pt_tcp_component.listen_thread_active) {
        FD_ZERO(&readfds);
        max = -1;
        SCON_LIST_FOREACH(listener, &mca_pt2pt_tcp_component.listeners, scon_pt2pt_tcp_listener_t) {
            FD_SET(listener->sd, &readfds);
            max = (listener->sd > max) ? listener->sd : max;
        }
        /* add the stop_thread fd */
        FD_SET(mca_pt2pt_tcp_component.stop_thread[0], &readfds);
        max = (mca_pt2pt_tcp_component.stop_thread[0] > max) ? mca_pt2pt_tcp_component.stop_thread[0] : max;

        /* set timeout interval */
        timeout.tv_sec = mca_pt2pt_tcp_component.listen_thread_tv.tv_sec;
        timeout.tv_usec = mca_pt2pt_tcp_component.listen_thread_tv.tv_usec;

        /* Block in a select to avoid hammering the cpu.  If a connection
         * comes in, we'll get woken up right away.
         */
        rc = select(max + 1, &readfds, NULL, NULL, &timeout);
        if (!mca_pt2pt_tcp_component.listen_thread_active) {
            /* we've been asked to terminate */
            close(mca_pt2pt_tcp_component.stop_thread[0]);
            close(mca_pt2pt_tcp_component.stop_thread[1]);
            return NULL;
        }
        if (rc < 0) {
            if (EAGAIN != scon_socket_errno && EINTR != scon_socket_errno) {
                perror("select");
            }
            continue;
        }

        /* Spin accepting connections until all active listen sockets
         * do not have any incoming connections, pushing each connection
         * onto the event queue for processing
         */
        do {
            accepted_connections = 0;
            SCON_LIST_FOREACH(listener, &mca_pt2pt_tcp_component.listeners, scon_pt2pt_tcp_listener_t) {
                sd = listener->sd;

                /* according to the man pages, select replaces the given descriptor
                 * set with a subset consisting of those descriptors that are ready
                 * for the specified operation - in this case, a read. So we need to
                 * first check to see if this file descriptor is included in the
                 * returned subset
                 */
                if (0 == FD_ISSET(sd, &readfds)) {
                    /* this descriptor is not included */
                    continue;
                }

                /* this descriptor is ready to be read, which means a connection
                 * request has been received - so harvest it. All we want to do
                 * here is accept the connection and push the info onto the event
                 * library for subsequent processing - we don't want to actually
                 * process the connection here as it takes too long, and so the
                 * OS might start rejecting connections due to timeout.
                 */
                pending_connection = SCON_NEW(scon_pt2pt_tcp_pending_connection_t);
                scon_event_set(scon_pt2pt_base.pt2pt_evbase, &pending_connection->ev, -1,
                               SCON_EV_WRITE, connection_handler, pending_connection);
                scon_event_set_priority(&pending_connection->ev, SCON_MSG_PRI);
                pending_connection->fd = accept(sd,
                                                (struct sockaddr*)&(pending_connection->addr),
                                                &addrlen);

                /* check for < 0 as indicating an error upon accept */
                if (pending_connection->fd < 0) {
                    SCON_RELEASE(pending_connection);

                    /* Non-fatal errors */
                    if (EAGAIN == scon_socket_errno ||
                        EWOULDBLOCK == scon_socket_errno) {
                        continue;
                    }

                    /* If we run out of file descriptors, log an extra
                       warning (so that the user can know to fix this
                       problem) and abandon all hope. */
                    else if (EMFILE == scon_socket_errno) {
                        CLOSE_THE_SOCKET(sd);
                        SCON_ERROR_LOG(SCON_ERR_SYS_LIMITS_SOCKETS);
                        scon_show_help("help-pt2pt-tcp.txt",
                                       "accept failed",
                                       true,
                                       scon_socket_errno,
                                       strerror(scon_socket_errno),
                                       "Out of file descriptors");
                        goto done;
                    }

                    /* For all other cases, print a
                       warning but try to continue */
                    else {
                        scon_show_help("help-pt2pt-tcp.txt",
                                       "accept failed",
                                       true,
                                       scon_socket_errno,
                                       strerror(scon_socket_errno),
                                       "Unknown cause; job will try to continue");
                        continue;
                    }
                }

                scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                                    "%s scon_pt2pt_tcp_listen_thread: incoming connection: "
                                    "(%d, %d) %s:%d\n",
                                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                    pending_connection->fd, scon_socket_errno,
                                    scon_net_get_hostname((struct sockaddr*) &pending_connection->addr),
                                    scon_net_get_port((struct sockaddr*) &pending_connection->addr));

                /* if we are on a privileged port, we only accept connections
                 * from other privileged sockets. A privileged port is one
                 * whose port is less than 1024 on Linux, so we'll check for that. */
                if (1024 >= listener->port) {
                    uint16_t inport;
                    inport = scon_net_get_port((struct sockaddr*) &pending_connection->addr);
                    if (1024 < inport) {
                        /* someone tried to cross-connect privileges,
                         * say something */
                        scon_show_help("help-pt2pt-tcp.txt",
                                       "privilege failure", true, listener->port,
                                       scon_net_get_hostname((struct sockaddr*) &pending_connection->addr),
                                       inport);
                        CLOSE_THE_SOCKET(pending_connection->fd);
                        SCON_RELEASE(pending_connection);
                        continue;
                    }
                }

                /* activate the event */
                scon_event_active(&pending_connection->ev, SCON_EV_WRITE, 1);
                accepted_connections++;
            }
        } while (accepted_connections > 0);
    }

 done:

    /* once we complete the initial launch, the "flood" of connections
     * will end - only connection requests from local procs, connect/accept
     * operations across mpirun instances, or the occasional tool will need
     * to be serviced. As these are relatively small events, we can easily
     * handle them in the context of the event library and no longer require
     * a separate connection harvesting thread. So switch over to the event
     * lib handler now
     */
    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s scon_pt2pt_tcp_listen_thread: switching to event lib",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));
    /* setup to listen via event library */
    SCON_LIST_FOREACH(listener, &mca_pt2pt_tcp_component.listeners, scon_pt2pt_tcp_listener_t) {
        listener->ev_active = true;
        scon_event_set(scon_pt2pt_base.pt2pt_evbase, &listener->event,
                       listener->sd,
                       SCON_EV_READ|SCON_EV_PERSIST,
                       connection_event_handler,
                       0);
        scon_event_set_priority(&listener->event, SCON_MSG_PRI);
        scon_event_add(&listener->event, 0);
    }
    return NULL;
}

/*
 * Handler for accepting connections from the listen thread
 */
static void connection_handler(int sd, short flags, void* cbdata)
{
    scon_pt2pt_tcp_pending_connection_t *new_connection;

    new_connection = (scon_pt2pt_tcp_pending_connection_t*)cbdata;

    scon_output_verbose(4, scon_pt2pt_base_framework.framework_output,
                        "%s connection_handler: working connection "
                        "(%d, %d) %s:%d\n",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        new_connection->fd, scon_socket_errno,
                        scon_net_get_hostname((struct sockaddr*) &new_connection->addr),
                        scon_net_get_port((struct sockaddr*) &new_connection->addr));

    /* process the connection */
    scon_pt2pt_tcp_accept_connection(new_connection->fd,
                                    (struct sockaddr*) &(new_connection->addr));
    /* cleanup */
    SCON_RELEASE(new_connection);

}
#endif
/*
 * Handler for accepting connections from the event library
 */
static void connection_event_handler(int incoming_sd, short flags, void* cbdata)
{
    struct sockaddr addr;
    scon_socklen_t addrlen = sizeof(struct sockaddr);
    int sd;

    sd = accept(incoming_sd, (struct sockaddr*)&addr, &addrlen);
    scon_output_verbose(PT2PT_TCP_DEBUG_CONNECT, scon_pt2pt_base_framework.framework_output,
                        "%s connection_event_handler: working connection "
                        "(%d, %d) %s:%d\n",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        sd, scon_socket_errno,
                        scon_net_get_hostname((struct sockaddr*) &addr),
                        scon_net_get_port((struct sockaddr*) &addr));
    if (sd < 0) {
        /* Non-fatal errors */
        if (EINTR == scon_socket_errno ||
            EAGAIN == scon_socket_errno ||
            EWOULDBLOCK == scon_socket_errno) {
            return;
        }

        /* If we run out of file descriptors, log an extra warning (so
           that the user can know to fix this problem) and abandon all
           hope. */
        else if (EMFILE == scon_socket_errno) {
            CLOSE_THE_SOCKET(incoming_sd);
            SCON_ERROR_LOG(SCON_ERR_SYS_LIMITS_SOCKETS);
            scon_show_help("help-pt2pt-tcp.txt",
                           "accept failed",
                           true,
                           scon_socket_errno,
                           strerror(scon_socket_errno),
                           "Out of file descriptors");
            return;
        }

        /* For all other cases, close the socket, print a warning but
           try to continue */
        else {
            CLOSE_THE_SOCKET(incoming_sd);
            scon_show_help("help-pt2pt-tcp.txt",
                           "accept failed",
                           true,
                           scon_socket_errno,
                           strerror(scon_socket_errno),
                           "Unknown cause; job will try to continue");
            return;
        }
    }

    /* process the connection */
    scon_pt2pt_tcp_accept_connection(sd, &addr);
}


static void tcp_ev_cons(scon_pt2pt_tcp_listener_t* event)
{
    event->ev_active = false;
    event->tcp6 = false;
    event->sd = -1;
    event->port = 0;
}
static void tcp_ev_des(scon_pt2pt_tcp_listener_t* event)
{
    if (event->ev_active) {
        scon_event_del(&event->event);
    }
    event->ev_active = false;
    if (0 <= event->sd) {
        CLOSE_THE_SOCKET(event->sd);
        event->sd = -1;
    }
}

SCON_CLASS_INSTANCE(scon_pt2pt_tcp_listener_t,
                   scon_list_item_t,
                   tcp_ev_cons, tcp_ev_des);

SCON_CLASS_INSTANCE(scon_pt2pt_tcp_pending_connection_t,
                   scon_object_t,
                   NULL,
                   NULL);
