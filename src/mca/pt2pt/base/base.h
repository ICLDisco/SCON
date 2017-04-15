/*
 * Copyright (c) 2016-2017     Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**
 * @file
 *
 * Pt2Pt Framework maintenence interface
 *
 *
 *
 */

#ifndef SCON_PT2PT_BASE_H
#define SCON_PT2PT_BASE_H

#include <scon_config.h>
#include <scon_types.h>
#include "src/mca/base/base.h"
#include "src/class/scon_hash_table.h"
#include "src/class/scon_bitmap.h"
#include "src/mca/pt2pt/pt2pt.h"

SCON_EXPORT extern scon_mca_base_framework_t scon_pt2pt_base_framework;

/* a global struct tracking framework level objects */
typedef struct {
    scon_list_t actives;
    int max_uri_length;
    scon_hash_table_t peers;
    bool num_threads;
    scon_event_base_t *pt2pt_evbase;
} scon_pt2pt_base_t;
SCON_EXPORT extern scon_pt2pt_base_t scon_pt2pt_base;

typedef struct {
    scon_object_t super;
    scon_pt2pt_module_t *module;
    scon_bitmap_t addressable;
} scon_pt2pt_base_peer_t;
SCON_EXPORT SCON_CLASS_DECLARATION(scon_pt2pt_base_peer_t);

/* select a component */
int scon_pt2pt_base_select(void);
/* get a module of the selected component */
scon_pt2pt_module_t * scon_pt2pt_base_get_module(char *comp_name);
#define SCON_PT2PT_NUM_THREADS 8

#define PT2PT_SEND_MESSAGE(m)                                        \
    do {                                                             \
    scon_send_req_t *cd;                                             \
    scon_output_verbose(1,                                           \
                       scon_pt2pt_base_framework.framework_output,   \
                       "%s PT2PT_SEND: %s:%d",                       \
                       SCON_PRINT_PROC(SCON_PROC_MY_NAME),           \
                        __FILE__, __LINE__);                         \
    cd =  SCON_NEW(scon_send_req_t);                                 \
    cd->post.send.scon_handle = m->scon_handle;                     \
    strncpy(cd->post.send.origin.job_name, m->origin.job_name,      \
               SCON_MAX_JOBLEN);                                    \
    cd->post.send.origin.rank = m->origin.rank;                     \
    cd->post.send.buf = m->buf;                                     \
    cd->post.send.tag = m->tag;                                     \
    cd->post.send.dst.rank = m->dst.rank;                           \
    cd->post.send.cbfunc = m->cbfunc;                               \
    cd->post.send.cbdata = m->cbdata;                               \
    strncpy(cd->post.send.dst.job_name, m->dst.job_name,            \
        SCON_MAX_JOBLEN);                                           \
    scon_event_set(scon_pt2pt_base.pt2pt_evbase, &cd->ev, -1,        \
                   SCON_EV_WRITE,                                    \
                   pt2pt_base_process_send, cd);                     \
    scon_event_set_priority(&cd->ev, SCON_MSG_PRI);                  \
    scon_event_active(&cd->ev, SCON_EV_WRITE, 1);                    \
}while(0);

#define PT2PT_POST_MESSAGE(p, t, h, b, l )                                 \
    do {                                                                   \
    scon_recv_t *msg;                                                      \
    scon_output_verbose (1,                                                \
                 scon_pt2pt_base_framework.framework_output,               \
                 "%s Message from %s posted at %s:%d",                     \
                 SCON_PRINT_PROC(SCON_PROC_MY_NAME),                       \
                 SCON_PRINT_PROC(p),                                       \
                 __FILE__, __LINE__);                                      \
    msg = SCON_NEW(scon_recv_t);                                           \
    strncpy(msg->sender.job_name, (p)->job_name,                           \
            SCON_MAX_JOBLEN);                                              \
    msg->sender.rank = (p)->rank;                                          \
    msg->tag = (t);                                                        \
    msg->scon_handle = (h);                                                \
    msg->iov.iov_base = (IOVBASE_TYPE*)(b);                                \
    msg->iov.iov_len = (l);                                                \
    /* setup the event */                                                  \
    scon_event_set(scon_pt2pt_base.pt2pt_evbase, &msg->ev, -1,             \
                   SCON_EV_WRITE,                                          \
                   pt2pt_base_process_recv_msg, msg);                      \
    scon_event_set_priority(&msg->ev, SCON_MSG_PRI);                       \
    scon_event_active(&msg->ev, SCON_EV_WRITE, 1);                         \
} while(0);

#define PT2PT_SEND_COMPLETE(m)                                       \
do {                                                                 \
    scon_output_verbose(5,                                           \
                    scon_pt2pt_base_framework.framework_output,      \
                    "%s-%s Send message complete at %s:%d",          \
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),              \
                    SCON_PRINT_PROC(&((m)->dst)),                    \
                    __FILE__, __LINE__);                             \
   if (NULL != (m)->cbfunc) {                                        \
        /* non-blocking buffer send */                               \
        (m)->cbfunc((m)->status, (m)->scon_handle,                  \
                       &((m)->dst),                                  \
                       (m)->buf,                                     \
                       (m)->tag, (m)->cbdata);                       \
     }                                                               \
}while(0);

/* stub function declarations */
SCON_EXPORT int pt2pt_base_api_send_nb (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_buffer_t *buf,
                            scon_msg_tag_t tag,
                            scon_send_cbfunc_t cbfunc,
                            void *cbdata,
                            scon_info_t info[],
                            size_t ninfo);

SCON_EXPORT int pt2pt_base_api_recv_nb (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_msg_tag_t tag,
                            bool persistent,
                            scon_recv_cbfunc_t cbfunc,
                            void *cbdata,
                            scon_info_t info[],
                            size_t ninfo);

SCON_EXPORT int pt2pt_base_api_recv_cancel (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_msg_tag_t tag);

/* internal pt2pt helper functions */
SCON_EXPORT void pt2pt_base_post_recv(int sd, short args, void *cbdata);
SCON_EXPORT void pt2pt_base_process_recv_msg(int fd, short flags, void *cbdata);
SCON_EXPORT void scon_pt2pt_base_get_contact_info(char **uri);
SCON_EXPORT void scon_pt2pt_base_set_contact_info(char *uri);
SCON_EXPORT void pt2pt_base_process_send (int fd, short flags, void *cbdata);
#endif /* SCON_PT2PT_BASE_H */
