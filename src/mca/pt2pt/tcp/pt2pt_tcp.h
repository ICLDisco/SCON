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
 * Copyright (c) 2014-2016     Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _SCON_PT2PT_TCP_H_
#define _SCON_PT2PT_TCP_H_

#include "scon_config.h"
#include "scon_common.h"
#include "scon_types.h"

#include "src/mca/base/base.h"
#include "src/class/scon_hash_table.h"
#include "src/runtime/scon_progress_threads.h"
#include "src/mca/pt2pt/pt2pt.h"
#include "src/mca/pt2pt/base/base.h"
#include "src/mca/comm/base/base.h"


BEGIN_C_DECLS

/* define some debug levels */
#define PT2PT_TCP_DEBUG_FAIL      2
#define PT2PT_TCP_DEBUG_CONNECT   7

#define SCON_ENABLE_STATIC_PORTS 0
/* forward declare a couple of structures */
struct scon_pt2pt_tcp_module_t;
struct scon_pt2pt_tcp_msg_error_t;

/* define a struct for tracking NIC addresses */
typedef struct {
    scon_list_item_t super;
    uint16_t af_family;
    struct sockaddr addr;
} scon_pt2pt_tcp_nicaddr_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_pt2pt_tcp_nicaddr_t);

void scon_pt2pt_tcp_init(void);
void scon_pt2pt_tcp_fini(void);
void scon_pt2pt_tcp_accept_connection(const int accepted_fd,
                                      const struct sockaddr *addr);
void scon_pt2pt_tcp_set_peer (const scon_proc_t* name,
                                     const uint16_t af_family,
                                     const char *net, const char *ports);
void scon_pt2pt_tcp_ping(const scon_proc_t *proc);
void scon_pt2pt_tcp_resend_nb_fn_t(scon_send_t *msg);
typedef void (*scon_pt2pt_tcp_module_resend_nb_fn_t)(struct scon_pt2pt_tcp_msg_error_t *mop);
/* Module definition */

/*typedef void (*scon_pt2pt_tcp_module_init_fn_t)(void);
typedef void (*scon_pt2pt_tcp_module_fini_fn_t)(void);
typedef void (*scon_pt2pt_tcp_module_accept_connection_fn_t)(const int accepted_fd,
                                                          const struct sockaddr *addr);
typedef void (*scon_pt2pt_tcp_module_set_peer_fn_t)(const scon_proc_t* name,
                                                 const uint16_t af_family,
                                                 const char *net, const char *ports);
typedef void (*scon_pt2pt_tcp_module_ping_fn_t)(const scon_proc_t *proc);
typedef void (*scon_pt2pt_tcp_module_send_nb_fn_t)(scon_send_t *msg);
typedef void (*scon_pt2pt_tcp_module_resend_nb_fn_t)(struct scon_pt2pt_tcp_msg_error_t *mop);
typedef void (*scon_pt2pt_tcp_module_ft_event_fn_t)(int state);

typedef struct {
    scon_pt2pt_tcp_module_init_fn_t               init;
    scon_pt2pt_tcp_module_fini_fn_t               finalize;
    scon_pt2pt_tcp_module_accept_connection_fn_t  accept_connection;
    scon_pt2pt_tcp_module_set_peer_fn_t           set_peer;
    scon_pt2pt_tcp_module_ping_fn_t               ping;
    scon_pt2pt_tcp_module_send_nb_fn_t            send_nb;
    scon_pt2pt_tcp_module_resend_nb_fn_t          resend;
    scon_pt2pt_tcp_module_ft_event_fn_t           ft_event;
} scon_pt2pt_tcp_module_api_t;*/

typedef struct {
    //scon_pt2pt_tcp_module_api_t  api;
    scon_pt2pt_module_t        base;
    scon_event_base_t          *ev_base;      /* event base for the module progress thread */
    bool                       ev_active;
    scon_thread_t              progress_thread;
    scon_hash_table_t          peers;         // connection addresses for peers
} scon_pt2pt_tcp_module_t;
SCON_EXPORT extern scon_pt2pt_tcp_module_t scon_pt2pt_tcp_module;

/**
 * the state of the connection
 */
typedef enum {
    SCON_PT2PT_TCP_UNCONNECTED,
    SCON_PT2PT_TCP_CLOSED,
    SCON_PT2PT_TCP_RESOLVE,
    SCON_PT2PT_TCP_CONNECTING,
    SCON_PT2PT_TCP_CONNECT_ACK,
    SCON_PT2PT_TCP_CONNECTED,
    SCON_PT2PT_TCP_FAILED,
    SCON_PT2PT_TCP_ACCEPTING
} scon_pt2pt_tcp_conn_state_t;

/* module-level shared functions */
void scon_pt2pt_tcp_send_handler(int fd, short args, void *cbdata);
void scon_pt2pt_tcp_recv_handler(int fd, short args, void *cbdata);

END_C_DECLS

#endif /* SCON_PT2PT_TCP_H_ */

