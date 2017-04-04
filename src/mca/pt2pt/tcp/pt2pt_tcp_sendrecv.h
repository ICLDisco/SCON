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
 * Copyright (c) 2010-2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2013-2016 Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef _SCON_PT2PT_TCP_SENDRECV_H_
#define _SCON_PT2PT_TCP_SENDRECV_H_

#include "scon_config.h"

#include "src/class/scon_list.h"

#include "src/mca/pt2pt/base/base.h"
#include "pt2pt_tcp.h"
#include "pt2pt_tcp_hdr.h"

/* tcp structure for sending a message */
typedef struct {
    scon_list_item_t super;
    scon_pt2pt_tcp_hdr_t hdr;
    scon_send_t *msg;
    char *data;
    bool hdr_sent;
    int iovnum;
    char *sdptr;
    size_t sdbytes;
} scon_pt2pt_tcp_send_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_send_t);

/* tcp structure for recving a message */
typedef struct {
    scon_list_item_t super;
    scon_pt2pt_tcp_hdr_t hdr;
    bool hdr_recvd;
    char *data;
    char *rdptr;
    size_t rdbytes;
} scon_pt2pt_tcp_recv_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_recv_t);

/* Queue a message to be sent to a specified peer. The macro
 * checks to see if a message is already in position to be
 * sent - if it is, then the message provided is simply added
 * to the peer's message queue. If not, then the provided message
 * is placed in the "ready" position
 *
 * If the provided boolean is true, then the send event for the
 * peer is checked and activated if not already active. This allows
 * the macro to either immediately send the message, or to queue
 * it as "pending" for later transmission - e.g., after the
 * connection procedure is completed
 *
 * p => pointer to scon_pt2pt_tcp_peer_t
 * s => pointer to scon_pt2pt_tcp_send_t
 * f => true if send event is to be activated
 */
#define SCON_PT2PT_TCP_QUEUE_MSG(p, s, f)                                  \
    do {                                                                \
        /* if there is no message on-deck, put this one there */        \
        if (NULL == (p)->send_msg) {                                    \
            (p)->send_msg = (s);                                        \
        } else {                                                        \
            /* add it to the queue */                                   \
            scon_list_append(&(p)->send_queue, &(s)->super);            \
        }                                                               \
        if ((f)) {                                                      \
            /* if we aren't connected, then start connecting */         \
            if (SCON_PT2PT_TCP_CONNECTED != (p)->state) {                  \
                (p)->state = SCON_PT2PT_TCP_CONNECTING;                    \
                SCON_ACTIVATE_TCP_CONN_STATE((p), scon_pt2pt_tcp_peer_try_connect); \
            } else {                                                    \
                /* ensure the send event is active */                   \
                if (!(p)->send_ev_active) {                             \
                    scon_event_add(&(p)->send_event, 0);                \
                    (p)->send_ev_active = true;                         \
                }                                                       \
            }                                                           \
        }                                                               \
    }while(0);

/* queue a message to be sent by one of our modules - must
 * provide the following params:
 *
 * m - the RML message to be sent
 * p - the final recipient
 */
#define SCON_PT2PT_TCP_QUEUE_SEND(m, p)                                    \
    do {                                                                \
        scon_pt2pt_tcp_send_t *msg;                                        \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] queue send to %s",              \
                             SCON_PRINT_PROC(SCON_PROC_MY_NAME),        \
                             __FILE__, __LINE__,                        \
                            SCON_PRINT_PROC(&((m)->dst)));              \
        msg = SCON_NEW(scon_pt2pt_tcp_send_t);                          \
        /* setup the header */                                          \
        msg->hdr.origin = (m)->origin;                                  \
        msg->hdr.dst = (m)->dst;                                        \
        msg->hdr.type = SCON_PT2PT_TCP_USER;                            \
        msg->hdr.tag = (m)->tag;                                        \
        msg->hdr.scon_handle = (m)->scon_handle;                        \
        /* point to the actual message */                               \
        msg->msg = (m);                                                 \
        msg->hdr.nbytes = (m)->buf->bytes_used;                         \
        /* prep header for xmission */                                  \
        SCON_PT2PT_TCP_HDR_HTON(&msg->hdr);                             \
        /* start the send with the header */                            \
        msg->sdptr = (char*)&msg->hdr;                                  \
        msg->sdbytes = sizeof(scon_pt2pt_tcp_hdr_t);                    \
        /* add to the msg queue for this peer */                        \
        SCON_PT2PT_TCP_QUEUE_MSG((p), msg, true);                       \
    }while(0);

/* queue a message to be sent by one of our modules upon completing
 * the connection process - must provide the following params:
 *
 * m - the RML message to be sent
 * p - the final recipient
 */
#define SCON_PT2PT_TCP_QUEUE_PENDING(m, p)                                 \
    do {                                                                \
        scon_pt2pt_tcp_send_t *msg;                                        \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] queue pending to %s",           \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__,                         \
                            SCON_PRINT_PROC(&((m)->dst)));              \
        msg = SCON_NEW(scon_pt2pt_tcp_send_t);                          \
        /* setup the header */                                          \
        msg->hdr.origin = (m)->origin;                                  \
        msg->hdr.dst = (m)->dst;                                        \
        msg->hdr.type = SCON_PT2PT_TCP_USER;                            \
        msg->hdr.tag = (m)->tag;                                        \
        msg->hdr.scon_handle = (m)->scon_handle;                        \
        /* point to the actual message */                               \
        msg->msg = (m);                                                 \
        /* set the total number of bytes to be sent */                  \
        msg->hdr.nbytes = (m)->buf->bytes_used;                         \
        /* prep header for xmission */                                  \
        SCON_PT2PT_TCP_HDR_HTON(&msg->hdr);                             \
        /* start the send with the header */                            \
        msg->sdptr = (char*)&msg->hdr;                                  \
        msg->sdbytes = sizeof(scon_pt2pt_tcp_hdr_t);                    \
        /* add to the msg queue for this peer */                        \
        SCON_PT2PT_TCP_QUEUE_MSG((p), msg, false);                      \
    }while(0);

/* queue a message for relay by one of our modules - must
 * provide the following params:
 *
 * m = the scon_pt2pt_tcp_recv_t that was received
 * p - the next hop
*/
#define SCON_PT2PT_TCP_QUEUE_RELAY(m, p)                                   \
    do {                                                                \
        scon_pt2pt_tcp_send_t *msg;                                        \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] queue relay to %s",             \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__,                         \
                            SCON_PRINT_PROC(&((p)->name)));             \
        msg = SCON_NEW(scon_pt2pt_tcp_send_t);                              \
        /* setup the header */                                          \
        msg->hdr.origin = (m)->hdr.origin;                              \
        msg->hdr.dst = (m)->hdr.dst;                                    \
        msg->hdr.type = SCON_PT2PT_TCP_USER;                               \
        msg->hdr.tag = (m)->hdr.tag;                                    \
        /* point to the actual message */                               \
        msg->data = (m)->data;                                          \
        /* set the total number of bytes to be sent */                  \
        msg->hdr.nbytes = (m)->hdr.nbytes;                              \
        /* prep header for xmission */                                  \
        SCON_PT2PT_TCP_HDR_HTON(&msg->hdr);                                \
        /* start the send with the header */                            \
        msg->sdptr = (char*)&msg->hdr;                                  \
        msg->sdbytes = sizeof(scon_pt2pt_tcp_hdr_t);                       \
        /* add to the msg queue for this peer */                        \
        SCON_PT2PT_TCP_QUEUE_MSG((p), msg, true);                          \
    }while(0);

/* State machine for processing message */
typedef struct {
    scon_object_t super;
    scon_event_t ev;
    scon_send_t *msg;
} scon_pt2pt_tcp_msg_op_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_msg_op_t);

#define SCON_ACTIVATE_TCP_POST_SEND(ms, cbfunc)                         \
    do {                                                                \
        scon_pt2pt_tcp_msg_op_t *mop;                                      \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] post send to %s",               \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__,                         \
                            SCON_PRINT_PROC(&((ms)->dst)));             \
        mop = SCON_NEW(scon_pt2pt_tcp_msg_op_t);                            \
        mop->msg = (ms);                                                \
        scon_event_set(scon_pt2pt_tcp_module.ev_base, &mop->ev, -1,        \
                       SCON_EV_WRITE, (cbfunc), mop);                   \
        scon_event_set_priority(&mop->ev, SCON_MSG_PRI);                \
        scon_event_active(&mop->ev, SCON_EV_WRITE, 1);                  \
    } while(0);

typedef struct {
    scon_object_t super;
    scon_event_t ev;
    scon_send_t *rmsg;
    scon_pt2pt_tcp_send_t *snd;
    scon_proc_t hop;
} scon_pt2pt_tcp_msg_error_t;
SCON_CLASS_DECLARATION(scon_pt2pt_tcp_msg_error_t);

#define SCON_ACTIVATE_TCP_MSG_ERROR(s, r, h, cbfunc)                    \
    do {                                                                \
        scon_pt2pt_tcp_msg_error_t *mop;                                   \
        scon_pt2pt_tcp_send_t *snd;                                        \
        scon_pt2pt_tcp_recv_t *proxy;                                      \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] post msg error to %s",          \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__,                         \
                            SCON_PRINT_PROC((h)));                      \
        mop = SCON_NEW(scon_pt2pt_tcp_msg_error_t);                      \
        if (NULL != (s)) {                                              \
            mop->snd = (s);                                             \
        } else if (NULL != (r)) {                                       \
            /* use a proxy so we can pass NULL into the macro */        \
            proxy = (r);                                                \
            /* create a send object for this message */                 \
            snd = SCON_NEW(scon_pt2pt_tcp_send_t);                       \
            mop->snd = snd;                                             \
            /* transfer and prep the header */                          \
            snd->hdr = proxy->hdr;                                      \
            SCON_PT2PT_TCP_HDR_HTON(&snd->hdr);                          \
            /* point to the data */                                     \
            snd->data = proxy->data;                                    \
            /* start the message with the header */                     \
            snd->sdptr = (char*)&snd->hdr;                              \
            snd->sdbytes = sizeof(scon_pt2pt_tcp_hdr_t);                 \
            /* protect the data */                                      \
            proxy->data = NULL;                                         \
        }                                                               \
        strncpy(mop->hop.job_name, (h)->job_name,                     \
                  SCON_MAX_JOBLEN);                              \
        mop->hop.rank = (h)->rank;                                      \
      /* this goes to the Pt2pt framework, so use that event base */    \
        scon_event_set(scon_pt2pt_base.pt2pt_evbase, &mop->ev, -1,                   \
                       SCON_EV_WRITE, (cbfunc), mop);                   \
        scon_event_set_priority(&mop->ev, SCON_MSG_PRI);                \
        scon_event_active(&mop->ev, SCON_EV_WRITE, 1);                  \
    } while(0);

#define SCON_ACTIVATE_TCP_POST_RESEND(mop, cbfunc)                      \
    do {                                                                \
        scon_pt2pt_tcp_msg_error_t *mp;                                    \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] post resend to %s",             \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__,                         \
                            SCON_PRINT_PROC(&((mop)->hop)));            \
        mp = SCON_NEW(scon_pt2pt_tcp_msg_error_t);                          \
        mp->snd = (mop)->snd;                                           \
        mp->hop = (mop)->hop;                                           \
        scon_event_set(scon_pt2pt_tcp_module.ev_base, &mp->ev, -1,         \
                       SCON_EV_WRITE, (cbfunc), mp);                    \
        scon_event_set_priority(&mp->ev, SCON_MSG_PRI);                 \
        scon_event_active(&mp->ev, SCON_EV_WRITE, 1);                   \
    } while(0);

#define SCON_ACTIVATE_TCP_NO_ROUTE(r, h, c)                             \
    do {                                                                \
        scon_pt2pt_tcp_msg_error_t *mop;                                 \
        scon_output_verbose(5, scon_pt2pt_base_framework.framework_output, \
                            "%s:[%s:%d] post no route to %s",           \
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),         \
                            __FILE__, __LINE__,                         \
                            SCON_PRINT_PROC((h)));                      \
        mop = SCON_NEW(scon_pt2pt_tcp_msg_error_t);                     \
        mop->rmsg = (r);                                                \
        strncpy( mop->hop.job_name, (h)->job_name,                      \
                  SCON_MAX_JOBLEN);                                     \
        mop->hop.rank = (h)->rank;                                      \
      /* this goes to the Pt2pt framework, so use that event base */    \
        scon_event_set(scon_pt2pt_base.pt2pt_evbase, &mop->ev, -1,               \
                       SCON_EV_WRITE, (c), mop);                        \
        scon_event_set_priority(&mop->ev, SCON_MSG_PRI);                \
        scon_event_active(&mop->ev, SCON_EV_WRITE, 1);                  \
    } while(0);
#endif /* _SCON_PT2PT_TCP_SENDRECV_H_ */
