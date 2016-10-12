/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * scon.h -  SCalable Overlay Network (SCON) Interface
 * $HEADER$
 */
#ifndef SCON_H
#define SCON_H

#include <stdint.h>
#include <string.h>
#include "orte/scon/scon_types.h"

BEGIN_C_DECLS

/***** SCON INFO KEYS *****/
/* The infos are used to describes the operational behavior of the SCON, some info keys
 * are specific to a operation, while others are generic. The infos serve as both
 * in/out parameters in the SCON API. When used as input, they typically describe the
 * behavior/requirements of the operation, when used as output the info objects
 * provide the results/information about the performed operation.
 *
 * define strings for keys - comments contain hints of  purpose
 * and value type. The keys are prefixed with the operation that they are applicable
 * to but some of those keys can be used for multiple operations as it
 * applies
 */

/* Value is a comma separated string (char *) of required topologies
 * example "binomtree,radixtree", use query interface to get string
 * names of supported topologies
 */
#define SCON_INIT_TOPO_LIST        "scon.init.topo.list"

/** Create Info keys **/

/* The maximum time to wait for all SCON members to
 * join the create handshake. value is a uint32 specifying timeout
 * in milliseconds.
 */
#define SCON_CREATE_TIMEOUT        "scon.create.timeout"
/* Verify that the SCON is consistently configured (same configuration set by all members)
 * before completing SCON create. Value is a bool.
 */
#define SCON_CREATE_VERIFY_CONFIG  "scon.create.verify.config"

/** Send Info Keys **/

/* The priority of the msg being sent. Value is a enum that corresponds to the priority
 * classes supported by the underlying fabric. The application may query the library
 * to obatin priority classes info
 */
#define SCON_SEND_MSG_PRIORITY     "scon.send.msg.priority"
/* describes the number of times the sender must retry the message in the event
 *of a transmission failure. The value is an uint32 which is greater than 0.
 */
#define SCON_SEND_NUM_RETRIES      "scon.send.num_retries"
/* describes the ack requirements for this message. The application can confirm
 * receipt of messages at the other end if needed by setting this info key
 * The SCON lib at the other end will ack the request. The send request
 * is failed if the receiver does not support acks*. Value is a bool
 */
#define SCON_SEND_REQUEST_ACK      "scon.send.req.ack"

/** Recv Info keys **/

/* Request the library to send an ACK to the sender when a message
 * matching the posted receive has arrived. Value is a bool
 */
#define SCON_RECV_SEND_ACK         "scon.recv.send.ack"
/* Describes the maximum number of messages expected to be
 * received on a persistent recv, before automatically cancelling the
 * recv request
 */
#define SCON_RECV_MAX              "scon.recv.max"

/** Xcast Info keys**/

/* The caller can request the SCON library to notify the calling process via a
 * previously registered error handler, when an error occurs while sending an
 * xcast on the tree. This is to accommodate for use cases which don't need
 * notification of a successful xcast for individual messages ( cbfunc is NULL)
 * but may need to handle the errors occurring on the SCON while relaying the xcast.
 */
#define SCON_XCAST_NOTIFY_ERROR    "scon.xcast.notify.error"
/* By setting this key the caller instructs the SCON library to enable all the hooks
 * for reliably broadcasting the message to all participants. When this key is set
 * the SCON shall try to route around failures to send the message to all participants.
 */
#define SCON_XCAST_RELIABLE        "scon.xcast.reliable"

/** Barrier Info keys **/

/* specifies timeout for a barrier operation in seconds, it is recommended that all
 * barrier participants specify the same timeout value if enabled. When the operation
 * times out at a local process, the SCON library cancels the barrier operation,
 * broadcasts a barrier release with a timeout error message to all participants.
 */
#define SCON_BARRIER_TIMEOUT       "scon.barrier.timeout"
/* the application can set this info key to request a blocking barrier operation,
 * in which case the callback function should be set to NULL
 */
#define SCON_BARRIER_BLOCKING      "scon.barrier.blocking"
/* the barrier algorithm to be used for this operation. The value is a enum
 * scon_barrier_algo_t that enumerates the various barrier algorithms supported by
 * the library.
 */
#define SCON_BARRIER_ALGORITHM    "scon.barrier.algorithm"

/** Query Info keys **/

/* describes the key or group of keys requested in the query interface. Value
 * of this key is a string an info key name which can be any of the supported keys
 * or a group of keys (for eg SCON_QUERY_INFO_ALL ("scon.query.info.all") - to
 * request all available info, SCON_QUERY_TOPO_ALL "scon.query.topo.all"- to request
 * all topology info, SCON_INFO_MASTER to query the identity of the master process.).
 * The info[] param can contain more than one element with this key.
 */
#define SCON_QUERY_KEY_NAME        "scon.query.key.name"
/* describes the address of the return info buffer. Value is a 64 bit ptr, if the
 * ptr is set to NULL, then the library allocates the return info blob, sets the
 * value of this key to the address of that blob. The info[] array should contain
 * only one element with this key.
 */
#define SCON_QUERY_RET_INFO_PTR    "scon.query.ret.info.ptr"
/* describes the size of the return info array. If the return info blob is allocated
 * by the caller, then the value of this key is set to the size of that blob. The
 * library sets the value of this key equal to the number of info elements in the return
 * info that it has allocated for the response
 */
#define SCON_QUERY_RET_INFO_SIZE   "scon.query.ret.info.size"

/** Delete Info Keys **/

/* This option is applicable for an unilateral teardown of the SCON i.e. when a
 * member wishes to exit the SCON because it encountered an error and is about to exit
 * or it has completed all its SCON operations. When this key is set the library
 * notifies the remaining members that the process is exiting from the SCON via
 * broadcast. The local delete operation completes, once the process has relayed
 * the xcast message corresponding to its exit.
 */
#define SCON_DELETE_NOTIFY_ALL     "scon.del.notify.all"
/* The caller can set this key to request the library to exit from the SCON silently
 * without coordinating with or notifying others.
 */
#define SCON_DELETE_SILENT          "scon.del.silent"
/* specifies the timeout for a coordinated teardown of the SCON. By default the
 * library coordinates the teardown operation by synchronizing all members on an internal
 * delete barrier. The value of this key is an unsigned int representing the timeout in
 * milliseconds for that barrier. */
#define SCON_DELETE_TIMEOUT         "scon.del.timeout"

/** JOB and PROC keys **/

/* job name key, value type char * - name of participating job.
 */
#define SCON_JOB_NAME              "scon.job.name"
/* value type bool, set to true if all ranks of job participate in scon
 * users can set this key instead of listing all ranks in job
 */
#define SCON_JOB_ALL               "scon.job.all"
/* array of job ranks participating in scon. value - scon_job_rank_info_t
 */
#define SCON_JOB_RANKS             "scon.job.ranks"
/* number of jobs participating in scon, applicable only for a multi job
 * SCON.
 */
#define SCON_NUM_JOBS              "scon.numjobs"

/** SCON Topology keys **/
/* topology type  value is enum  scon_topo_type
 */
#define SCON_TOPO_TYPE             "scon.topo.type"
/* value - scon_proc_t name of the root process (default is rank 0 of the job)
 */
#define SCON_TOPO_TREE_ROOT        "scon.topo.tree.root"
/* The degree/fan out of the topology tree applies only to a radix tree
 */
#define SCON_TOPO_TREE_FAN_OUT     "scon_topo.tree.fanout"
/* topology file name, value is string correponding to the absolute path and name
 * of the file
 */
#define SCON_TOPO_FILENAME         "scon.topo.filename"

/* SCON attribute keys */

/* int relative priority of this scon wrt to other scons that the process participates in.
 * For example a monitoring process can create two scons one for transporting events and
 * the other for regular sensor data..then it can set the event scon higher relative
 * to the sensor data
 */
#define SCON_PRIORITY              "scon.priority"
/* Transport protocols applicable to the SCON reliable protocols like TCP/IP ,USOCK,
 * or UD or  MQ or other
 */
#define SCON_PROTOCOLS             "scon.protocols"
/* describes fabrics used for transporting msgs on the wire, value is a comma separated
 * string of fabrics to use.
 */
#define SCON_SELECTED_FABRICS      "scon.fabrics.sel"
/*
 * scon_proc_t of the master process for the scon, the master process is like the
 * hnp, it contains the master configuration (topology etc..) information of the SCON.
 * and is responsible for distributing config etc information during create and
 * in operation. By Default the process with rank 0 is treated as the master process
 */
#define SCON_MASTER_PROC      "scon.master.proc"
/*
 * The size of the internal ring buffer for queuing
 * incoming messages
 */
#define SCON_RECV_QUEUE_LENGTH     "scon.recv.queue.length"

/*@desc callback fn for scon_create operation.  */
typedef void (*scon_create_cbfunc_t) (scon_status_t status,
                                      scon_handle_t scon_handle,
                                      void *cbdata);
/* scon send callback funtion. */
typedef void (*scon_send_cbfunc_t) (scon_status_t status,
                                    scon_handle_t scon_handle,
                                    scon_proc_t *peer,
                                    scon_buffer_t *buf,
                                    scon_msg_tag_t tag,
                                    void *cbdata);
/* scon recv callback function */
typedef void (*scon_recv_cbfunc_t) (scon_status_t status,
                                    scon_handle_t scon_handle,
                                    scon_proc_t *peer,
                                    scon_buffer_t *buf,
                                    scon_msg_tag_t tag,
                                    void *cbdata);
/* scon operation callback function */
typedef void (*scon_op_cbfunc_t) (scon_status_t status,
                                  void *cbdata);

/* scon xcast callback function */
typedef void (*scon_xcast_cbfunc_t) (scon_status_t status,
                                     scon_handle_t scon_handle,
                                     scon_proc_t procs[],
                                     size_t nprocs,
                                     scon_buffer_t *buf,
                                     scon_msg_tag_t tag,
                                     void *cbdata);

/* scon barrier callback function */
typedef void (*scon_barrier_cbfunc_t) (scon_status_t status,
                                       scon_handle_t scon_handle,
                                       scon_proc_t procs[],
                                       size_t nprocs,
                                       void *cbdata);
/**
 * @func scon_init initializes the scon library.
 * @param info   - array of info keys specifying the requested SCON library capabilities.
 *                 The caller can specify capabilities of all its SCONs here. This is an
 *                 optimization param that lets the caller selectively initialize the
 *                 required capabilities.
 * @param ninfo  - size of info array.
 *
 * @return init status.
 */
scon_status_t scon_init( scon_info_t info[],
                         size_t ninfo);

/**
 * @func scon_create creates a scon as specified by input params
 * @param procs    -  array of member processes participating in the SCON. Three
 *                    methods to specify the members are supported:
 *                    1.specify member processes explicitly by populating the procs array.
 *                    2.set procs to NULL which means all ranks of this job are
 *                      members of the SCON. It is not required for the caller to
 *                      explicitly specify the job information. The SCON library obtains
 *                      the job information from PMIx by default, in the absence of a PMIx
 *                      client/server the caller must provide the job info directly
 *                      using the job info keys, or topology info keys or point to a
 *                      config file where the SCON library can obtain that information.
 *                    3.set procs to NULL and populate the master process info
 *                      (SCON_MASTER_PROC) key, this is for cases where the required SCON
 *                      config info is provided to a designated process such as rank 0 and
 *                      the rest of the processes are told the identity of that master
 *                      process.
 * @param nprocs   -  number of participating processes, size of procs[] array can
 *                    be set to 0 when procs = null.
 * @param info[]   -  array of scon_info_t describing SCON configuration/properties
 *                    and the behavior of the create operation. Applicable info keyss
 *                    include scon.create.***, scon.topo.*** and other scon attribute
 *                    scon.*** keys.
 * @param ninfo    -  number of info keys in info[].
 * @return            returns handle to the newly created SCON. Note that the SCON creation
 *                    operation is not complete when this fn returns, however a handle is
 *                    returned so the caller can start queuing sends and recvs.
 */
scon_handle_t scon_create(scon_proc_t procs[],
                          size_t nprocs,
                          scon_info_t info[],
                          size_t ninfo,
                          scon_create_cbfunc_t cbfunc,
                          void *cbdata);

/**
 * @func scon_send_nb send a message to a member process asynchronously.
 * @param scon_handle - The handle (reference) to the SCON
 * @param peer        - destination process where the msg should be sent
 * @param buffer      - ptr to the message buffer to send
 * @param tag         - A user specified tag to match the sends and recvs (tags 1-100
 *                      are reserved for system defined messages)
 * @param cbfunc      - pointer to the send callback function. The callback function is
 *                      called once the message was sent on the wire by the sender at which
 *                      point the caller is free to release the message buffer.
 * @param cbdata      - opaque pointer to users data to be returned with the callback.
 * @param info        - optional array of info keys specifying the behavior of the send operation.
 *                      The info[] is expected to be NULL for normal sends, in which case the
 *                      overall SCON level behavior (properties and configuration) is applied
 *                      to the send operation. However the caller may populate the array to request
 *                      special treatment of a send message by specifying scon.send.*** info keys.
 *                      Note that the current implementation does not support send specific
 *                      info keys.
 * @param ninfo       - number of info keys in info[].
 * @return            - returns status of the operation (input validation & request queuing for
 *                      further processing).
 */
scon_status_t scon_send_nb (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_buffer_t *buffer,
                            scon_msg_tag_t tag,
                            scon_send_cbfunc_t cbfunc,
                            void *cbdata,
                            scon_info_t info[],
                            size_t ninfo);

/**
 * @func scon_recv_nb receive a message from a member process asynchronously.
 * @param scon_handle - The handle (reference) to the SCON
 * @param peer        - The sending process. A wild card process can be specified to
 *                      recv msg from any member on that tag.
 * @param tag         - A user specified tag to match the sends and recvs (tags 1-100
 *                      are reserved for system defined messages)
 * @param persistent  - boolean flag that specifies whether the posted recv is persistent
 *                      or one time only
 * @param cbfunc      - pointer to the recv callback function. The callback function is
 *                      called once the message was with that tag is received on the SCON
 *                      from the specified sender.
 * @param cbdata      - opaque pointer to users data to be returned with the callback.
 * @param info        - optional array of info keys specifying the behavior of the recv operation.
 *                      The info[] is expected to be NULL for normal sends, in which case the
 *                      overall SCON level behavior (properties and configuration) is applied
 *                      to the send operation. However the caller may populate the array to request
 *                      special treatment of a send message by specifying scon.send.*** info keys.
 *                      Note that the current implementation does not support send specific
 *                      info keys.
 * @param ninfo       - number of info keys in info[].
 * @return            - returns status of the operation (input validation & request queuing for
 *                      further processing).
 */
scon_status_t scon_recv_nb (scon_handle_t scon_handle,
                            scon_proc_t *peer,
                            scon_msg_tag_t tag,
                            bool persistent,
                            scon_recv_cbfunc_t cbfunc,
                            void *cbdata,
                            scon_info_t info[],
                            size_t ninfo);

/**
 * @func scon_recv_cancel Cancel a previously posted persistent recv
 * @param scon_handle - The handle (reference) to the SCON
 * @param peer        - The sending process.
 * @param tag         - The tag associated with the recv to cancel
 * @return            - returns success if a matching persistent recv was found.
 */
scon_status_t scon_recv_cancel (scon_handle_t scon_handle,
                                scon_proc_t *peer,
                                scon_msg_tag_t tag);

/**
 * @func scon_xcast Xcast a message asynchronously to all(broadcast) or a subset
 *       (multicast) of SCON members.
 * @param scon_handle - The handle (reference) to the SCON
 * @param procs[]     - array of member processes to which the xcast must be sent to.
 *                      Default value is NULL, in which case the message is sent to all
 *                      members.
 * @param nprocs      - number of elements in the procs array.
 * @param buffer      - ptr to the message buffer to send
 * @param tag         - A user specified tag to match the sends and recvs (tags 1-100
 *                      are reserved for system defined messages)
 * @param cbfunc      - pointer to the xcast callback function. Requesting a callback on
 *                      xcast completion is optional and it is only recommended when the
 *                      caller needs to ensure reliable xcast. The callback function is
 *                      called once the message is sent to all the participating processes
 *                      and an ACK was received from all the edge processes of the xcast
 *                      tree or mesh.
 * @param cbdata      - opaque pointer to users data to be returned with the callback.
 * @param info        - optional array of info keys specifying the behavior of the operation.
 *                      The default behavior is to send the xcast request along the SCON topology
 *                      routing around failures if needed. The caller can override the default
 *                      behavior by specifying the scon.xcast.**** info keys. Current implementation
 *                      does not support xcast specific info keys.
 * @param ninfo       - number of info keys in info[].
 * @return            - returns status of the operation (input validation & request queuing for
 *                      further processing).
 */
scon_status_t scon_xcast (scon_handle_t scon_handle,
                          scon_proc_t procs[],
                          size_t nprocs,
                          scon_buffer_t *buf,
                          scon_msg_tag_t tag,
                          scon_xcast_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo);
/**
 * @func scon_barrier Non blocking barrier (synchronization) function among all or specified
 *       members of the SCON.
 * @param scon_handle - The handle (reference) to the SCON
 * @param procs[]     - array of member processes participating in the barrier. Default
 *                      value is NULL, in which case the barrier is executed across all
 *                      SCON members.
 * @param nprocs      - number of elements in the procs array.
 * @param cbfunc      - pointer to the barrier release callback function.
 * @param cbdata      - opaque pointer to users data to be returned with the callback.
 * @param info        - optional array of info keys specifying the behavior of the barrier
 *                      operation. The caller can override the default behavior by
 *                      specifying the scon.barrier.**** info keys.
 * @param ninfo       - number of info keys in info[].
 * @return            - returns status of the operation (input validation & request queuing for
 *                      further processing).
 */
scon_status_t scon_barrier(scon_handle_t scon_handle,
                           scon_proc_t procs[],
                           size_t nprocs,
                           scon_barrier_cbfunc_t cbfunc,
                           void *cbdata,
                           scon_info_t info[],
                           size_t ninfo);

/**
 * @func scon_get_info get configuration and properties of a SCON or SCONs
 * @param scon_handle - handle to the SCON whose information is being queried. If an invalid
 *                      handle is given then the query applies to all the SCONs that the process
 *                      is a member. If no SCONs exist then the query is applied to obtain
 *                      supported configuration information.
 * @param info        - array of info keys that specifies what info is being queried and how
 *                      to return the requested information. It is required for the caller to
 *                      specify if the memory for the requested information was preallocated by
 *                      the caller or should the library allocate a memory blob to hold the
 *                      return information. If the library is required to allocate memory for
 *                      the return info then the caller's info array must contain an element
 *                      with the info key SCON_QUERY_RETURN_INFO_PTR and an element with the info
 *                      key SCON_QUERY_RETURN_INFO_SIZE. In any case the caller is responsible for
 *                      freeing the memory containing the return info (i.e. the query response).
 *                      Refer to SCON_QUERY.*** info keys for more.
 * @param ninfo       - number of info keys in info[].
 * @return            -  returns success if the requested information is provided.
 */
scon_status_t scon_get_info (scon_handle_t scon_handle,
                             scon_info_t info[],
                             size_t *ninfo);

/**
 * @func scon_delete delete/teardown a SCON asynchronously.
 * @param scon_handle - The handle (reference) to the SCON to delete
 * @param cbfunc      - pointer to the delete callback function.
 * @param cbdata      - opaque pointer to users data to be returned with the callback.
 * @param info[]      - optional array of info keys specificing delete behavior. Refer
 *                      to SCON_DELETE.*** keys for more.
 * @param ninfo       - number of info keys in info[].
 * @return            - returns success if the SCON was found and the delete operation
 *                      has started.
 */
scon_status_t scon_delete(scon_handle_t scon_handle,
                          scon_op_cbfunc_t cbfunc,
                          void *cbdata,
                          scon_info_t info[],
                          size_t ninfo);

/**
 * @func scon_finalize - finalize the SCON library
 * @params: None
 * @return - returns success if the lib resources are released successfully and shutdown
 *           is successful.
 */
scon_status_t scon_finalize(void);



END_C_DECLS
#endif
