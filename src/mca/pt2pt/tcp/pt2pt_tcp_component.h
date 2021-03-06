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
 * Copyright (c) 2014-2017 Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _SCON_PT2PT_TCP_COMPONENT_H_
#define _SCON_PT2PT_TCP_COMPONENT_H_

#include "scon_config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "src/class/scon_bitmap.h"
#include "src/class/scon_list.h"
#include "src/class/scon_pointer_array.h"

#include "src/mca/pt2pt/pt2pt.h"
#include "pt2pt_tcp.h"
#include "pt2pt_tcp_listener.h"
/**
 *  pt2pt tcp component
 */
typedef struct {
    scon_pt2pt_base_component_t super;          /**< base pt2pt component */
    uint32_t addr_count;                     /**< total number of addresses */
    int num_links;                           /**< number of logical links per physical device */
    int                  max_retries;        /**< max number of retries before declaring peer gone */
    scon_list_t          events;             /**< events for monitoring connections */
    int                  peer_limit;         /**< max size of tcp peer cache */
    scon_pointer_array_t ev_bases;           /**array of event bases serving the tcp component*/
    char**               ev_threads;         /** names of our progress threads */
    int                  next_base;          /** counter to load-level thread use */
    scon_hash_table_t    peers;              /** connection address for peers */
    /* Port specifications */
    char*              if_include;           /**< list of ip interfaces to include */
    char*              if_exclude;           /**< list of ip interfaces to exclude */
    int                tcp_sndbuf;           /**< socket send buffer size */
    int                tcp_rcvbuf;           /**< socket recv buffer size */

    /* IPv4 support */
    bool               disable_ipv4_family;  /**< disable this AF */
    char**             tcp_static_ports;    /**< Static ports - IPV4 */
    char**             tcp_dyn_ports;       /**< Dynamic ports - IPV4 */
    char**             ipv4conns;
    char**             ipv4ports;

    /* IPv6 support */
    bool               disable_ipv6_family;  /**< disable this AF */
    char**             tcp6_static_ports;    /**< Static ports - IPV6 */
    char**             tcp6_dyn_ports;       /**< Dynamic ports - IPV6 */
    char**             ipv6conns;
    char**             ipv6ports;

    /* connection support */
    scon_list_t        listeners;      /** the listener object*/
    scon_thread_t      listen_thread;          /**< handle to the listening thread */
    bool               listen_thread_active;
    struct timeval     listen_thread_tv;       /**< Timeout when using listen thread */
    int                stop_thread[2];         /**< pipe used to exit the listen thread */
    int                keepalive_probes;       /**< number of keepalives that can be missed before declaring error */
    int                keepalive_time;         /**< idle time in seconds before starting to send keepalives */
    int                keepalive_intvl;        /**< time between keepalives, in seconds */
    int                retry_delay;            /**< time to wait before retrying connection */
    int                max_recon_attempts;     /**< maximum number of times to attempt connect before giving up (-1 for never) */

} scon_pt2pt_tcp_component_t;

SCON_EXPORT extern scon_pt2pt_tcp_component_t mca_pt2pt_tcp_component;

void scon_pt2pt_tcp_component_set_module(int fd, short args, void *cbdata);
void scon_pt2pt_tcp_component_lost_connection(int fd, short args, void *cbdata);
void scon_pt2pt_tcp_component_failed_to_connect(int fd, short args, void *cbdata);
void scon_pt2pt_tcp_component_no_route(int fd, short args, void *cbdata);
void scon_pt2pt_tcp_component_hop_unknown(int fd, short args, void *cbdata);

#define SCON_PT2PT_TCP_NEXT_BASE(p)                                                       \
    do {                                                                                \
         ++scon_pt2pt_tcp_component.next_base;                                              \
        if (scon_pt2pt_base.num_threads <= scon_pt2pt_tcp_component.next_base) {             \
        scon_pt2pt_tcp_component.next_base = 0;                                        \
    }                                                                               \
    (p)->ev_base = (scon_event_base_t*)scon_pointer_array_get_item(&scon_pt2pt_tcp_component.ev_bases, \
                    scon_pt2pt_tcp_component.next_base); \
} while(0)



#endif /* _SCON_PT2PT_TCP_COMPONENT_H_ */
