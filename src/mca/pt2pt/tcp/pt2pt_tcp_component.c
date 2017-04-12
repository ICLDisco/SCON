/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
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
 * Copyright (c) 2006-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2013-2017 Intel, Inc.  All rights reserved.
 * Copyright (c) 2014      NVIDIA Corporation.  All rights reserved.
 * Copyright (c) 2015-2016 Research Organization for Information Science
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
#include "scon_socket_errno.h"
#include "src/util/if.h"
#include "src/util/net.h"
#include "src/util/name_fns.h"
#include "src/util/argv.h"
#include "src/util/parse_options.h"
#include "src/class/scon_hash_table.h"
#include "src/class/scon_list.h"
#include "src/runtime/scon_rte.h"

#include "src/include/scon_globals.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_component.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_peer.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_connection.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_listener.h"
#include "src/mca/pt2pt/tcp/pt2pt_tcp_ping.h"

/*
 * Local utility functions
 */

static int tcp_component_register(void);
static int tcp_component_open(void);
static int tcp_component_close(void);
static int tcp_component_query(scon_mca_base_module_t **module, int *priority);
static int tcp_component_startup(void);
static void tcp_component_shutdown(void);
static scon_pt2pt_module_t* tcp_component_get_module(void);
static int component_set_addr(scon_proc_t *peer,
                              char **uris);
static char* component_get_addr(void);

#define TCP_LISTEN 1

/*
 * Struct of function pointers and all that to let us be initialized
 */
scon_pt2pt_tcp_component_t mca_pt2pt_tcp_component = {
    /* First, the mca_base_component_t struct containing meta
       information about the component itself */
    .super =
    {.base_version =
    {
        SCON_PT2PT_BASE_VERSION_1_0_0,
        .scon_mca_component_name = "tcp",
        /* Component name and version */
        SCON_MCA_BASE_MAKE_VERSION(component, SCON_MAJOR_VERSION,
        SCON_MINOR_VERSION,
        SCON_RELEASE_VERSION),
        /* Component functions */
        .scon_mca_open_component = tcp_component_open,
        .scon_mca_close_component = tcp_component_close,
        .scon_mca_query_component = tcp_component_query,
        .scon_mca_register_component_params = tcp_component_register,

    },
    .priority = 80,
    .start = tcp_component_startup,
    .shutdown = tcp_component_shutdown,
    .get_addr = component_get_addr,
    .set_addr = component_set_addr,
    .get_module = tcp_component_get_module,
    },
};


static int tcp_component_startup(void)
{
    int rc = SCON_SUCCESS;

    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s TCP STARTUP",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));

    /* start the module */
    scon_pt2pt_tcp_init();
    if (SCON_SUCCESS != (rc = scon_pt2pt_tcp_start_listening())) {
        SCON_ERROR_LOG(rc);
    }
    return rc;
}



/*
 * Initialize global variables used w/in this module.
 */
static int tcp_component_open(void)
{
    /* initialize state */
    SCON_CONSTRUCT(&mca_pt2pt_tcp_component.listeners, scon_list_t);
#ifdef TCP_LISTEN
        SCON_CONSTRUCT(&mca_pt2pt_tcp_component.listen_thread, scon_thread_t);
        mca_pt2pt_tcp_component.listen_thread_active = false;
        mca_pt2pt_tcp_component.listen_thread_tv.tv_sec = 3600;
        mca_pt2pt_tcp_component.listen_thread_tv.tv_usec = 0;
#endif
    mca_pt2pt_tcp_component.addr_count = 0;
    mca_pt2pt_tcp_component.ipv4conns = NULL;
    mca_pt2pt_tcp_component.ipv4ports = NULL;
    mca_pt2pt_tcp_component.ipv6conns = NULL;
    mca_pt2pt_tcp_component.ipv6ports = NULL;
    /* if_include and if_exclude need to be mutually exclusive */
  /*  if (SCON_SUCCESS !=
        scon_mca_base_var_check_exclusive("scon",
        mca_pt2pt_tcp_component.super.base_version.scon_mca_type_name,
        mca_pt2pt_tcp_component.super.base_version.scon_mca_component_name,
        "if_include",
        mca_pt2pt_tcp_component.super.base_version.scon_mca_type_name,
        mca_pt2pt_tcp_component.super.base_version.scon_mca_component_name,
        "if_exclude")) {
        //Return ERR_NOT_AVAILABLE so that a warning message about
           //"open" failing is not printed
        return SCON_ERR_NOT_AVAILABLE;
    } */
    return SCON_SUCCESS;
}

/*
 * Cleanup of global variables used by this module.
 */
static int tcp_component_close(void)
{
    SCON_LIST_DESTRUCT(&mca_pt2pt_tcp_component.listeners);
    if (NULL != mca_pt2pt_tcp_component.ipv4conns) {
        scon_argv_free(mca_pt2pt_tcp_component.ipv4conns);
    }
    if (NULL != mca_pt2pt_tcp_component.ipv4ports) {
        scon_argv_free(mca_pt2pt_tcp_component.ipv4ports);
    }

#if SCON_ENABLE_IPV6
    if (NULL != mca_pt2pt_tcp_component.ipv6conns) {
        scon_argv_free(mca_pt2pt_tcp_component.ipv6conns);
    }
    if (NULL != mca_pt2pt_tcp_component.ipv6ports) {
        scon_argv_free(mca_pt2pt_tcp_component.ipv6ports);
    }
#endif
    return SCON_SUCCESS;
}

#if SCON_ENABLE_STATIC_PORTS
static char *static_port_string;

#if SCON_ENABLE_IPV6
static char *static_port_string6;
#endif // SCON_ENABLE_IPV6
#endif // SCON_ENABLE_STATIC_PORTS

static char *dyn_port_string;
#if SCON_ENABLE_IPV6
static char *dyn_port_string6;
#endif


static int tcp_component_register(void)
{
    scon_mca_base_component_t *component = &mca_pt2pt_tcp_component.super.base_version;
    int var_id;
    /* register pt2pt module parameters */
    mca_pt2pt_tcp_component.peer_limit = -1;
    (void)scon_mca_base_component_var_register(component, "peer_limit",
                                          "Maximum number of peer connections to simultaneously maintain (-1 = infinite)",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_5,
                                          SCON_MCA_BASE_VAR_SCOPE_LOCAL,
                                          &mca_pt2pt_tcp_component.peer_limit);

    mca_pt2pt_tcp_component.max_retries = 2;
    (void)scon_mca_base_component_var_register(component, "peer_retries",
                                          "Number of times to try shutting down a connection before giving up",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_5,
                                          SCON_MCA_BASE_VAR_SCOPE_LOCAL,
                                          &mca_pt2pt_tcp_component.max_retries);

    mca_pt2pt_tcp_component.tcp_sndbuf = 128 * 1024;
    (void)scon_mca_base_component_var_register(component, "sndbuf",
                                          "TCP socket send buffering size (in bytes)",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_4,
                                          SCON_MCA_BASE_VAR_SCOPE_LOCAL,
                                          &mca_pt2pt_tcp_component.tcp_sndbuf);

    mca_pt2pt_tcp_component.tcp_rcvbuf = 128 * 1024;
    (void)scon_mca_base_component_var_register(component, "rcvbuf",
                                          "TCP socket receive buffering size (in bytes)",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_4,
                                          SCON_MCA_BASE_VAR_SCOPE_LOCAL,
                                          &mca_pt2pt_tcp_component.tcp_rcvbuf);

    mca_pt2pt_tcp_component.if_include = NULL;
    var_id = scon_mca_base_component_var_register(component, "if_include",
                                             "Comma-delimited list of devices and/or CIDR notation of TCP networks to use for Open MPI bootstrap communication (e.g., \"eth0,192.168.0.0/16\").  Mutually exclusive with pt2pt_tcp_if_exclude.",
                                             SCON_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                             SCON_INFO_LVL_2,
                                             SCON_MCA_BASE_VAR_SCOPE_LOCAL,
                                             &mca_pt2pt_tcp_component.if_include);
    (void)scon_mca_base_var_register_synonym(var_id, "scon", "pt2pt", "tcp", "include",
                                         SCON_MCA_BASE_VAR_SYN_FLAG_DEPRECATED | SCON_MCA_BASE_VAR_SYN_FLAG_INTERNAL);

    mca_pt2pt_tcp_component.if_exclude = NULL;
    var_id = scon_mca_base_component_var_register(component, "if_exclude",
                                             "Comma-delimited list of devices and/or CIDR notation of TCP networks to NOT use for Open MPI bootstrap communication -- all devices not matching these specifications will be used (e.g., \"eth0,192.168.0.0/16\").  If set to a non-default value, it is mutually exclusive with pt2pt_tcp_if_include.",
                                             SCON_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                             SCON_INFO_LVL_2,
                                             SCON_MCA_BASE_VAR_SCOPE_LOCAL,
                                             &mca_pt2pt_tcp_component.if_exclude);
    (void)scon_mca_base_var_register_synonym(var_id, "scon", "pt2pt", "tcp", "exclude",
                                        SCON_MCA_BASE_VAR_SYN_FLAG_DEPRECATED | SCON_MCA_BASE_VAR_SYN_FLAG_INTERNAL);

    /* if_include and if_exclude need to be mutually exclusive */
    if (NULL != mca_pt2pt_tcp_component.if_include &&
        NULL != mca_pt2pt_tcp_component.if_exclude) {
        /* Return ERR_NOT_AVAILABLE so that a warning message about
           "open" failing is not printed */
        scon_show_help("help-pt2pt-tcp.txt", "include-exclude", true,
                       mca_pt2pt_tcp_component.if_include,
                       mca_pt2pt_tcp_component.if_exclude);
        return SCON_ERR_NOT_AVAILABLE;
    }

#if SCON_ENABLE_STATIC_PORTS
    static_port_string = NULL;
    (void)scon_mca_base_component_var_register(component, "static_ipv4_ports",
                                          "Static ports for daemons and procs (IPv4)",
                                          SCON_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                          SCON_INFO_LVL_2,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &static_port_string);

    /* if ports were provided, parse the provided range */
    if (NULL != static_port_string) {
        scon_util_parse_range_options(static_port_string, &mca_pt2pt_tcp_component.tcp_static_ports);
        if (0 == strcmp(mca_pt2pt_tcp_component.tcp_static_ports[0], "-1")) {
            scon_argv_free(mca_pt2pt_tcp_component.tcp_static_ports);
            mca_pt2pt_tcp_component.tcp_static_ports = NULL;
        }
    } else {
        mca_pt2pt_tcp_component.tcp_static_ports = NULL;
    }
#if SCON_ENABLE_IPV6
    static_port_string6 = NULL;
    (void)scon_mca_base_component_var_register(component, "static_ipv6_ports",
                                          "Static ports for daemons and procs (IPv6)",
                                          SCON_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                          SCON_INFO_LVL_2,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &static_port_string6);

    /* if ports were provided, parse the provided range */
    if (NULL != static_port_string6) {
        scon_util_parse_range_options(static_port_string6, &mca_pt2pt_tcp_component.tcp6_static_ports);
        if (0 == strcmp(mca_pt2pt_tcp_component.tcp6_static_ports[0], "-1")) {
            scon_argv_free(mca_pt2pt_tcp_component.tcp6_static_ports);
            mca_pt2pt_tcp_component.tcp6_static_ports = NULL;
        }
    } else {
        mca_pt2pt_tcp_component.tcp6_static_ports = NULL;
    }
    if (NULL == mca_pt2pt_tcp_component.tcp_static_ports &&
        NULL == mca_pt2pt_tcp_component.tcp6_static_ports) {
        scon_static_ports = false;
    } else {
        scon_static_ports = true;
    }
#endif // SCON_ENABLE_IPV6
#endif // SCON_ENABLE_STATIC_PORTS
    dyn_port_string = NULL;
    (void)scon_mca_base_component_var_register(component, "dynamic_ipv4_ports",
                                          "Range of ports to be dynamically used by daemons and procs (IPv4)",
                                          SCON_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                          SCON_INFO_LVL_4,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &dyn_port_string);
    /* if ports were provided, parse the provided range */
    if (NULL != dyn_port_string) {
        /* can't have both static and dynamic ports! */
        if (mca_pt2pt_tcp_component.tcp_static_ports) {
            char *err = scon_argv_join(mca_pt2pt_tcp_component.tcp_static_ports, ',');
            scon_show_help("help-pt2pt-tcp.txt", "static-and-dynamic", true,
                           err, dyn_port_string);
            free(err);
            return SCON_ERROR;
        }
        scon_util_parse_range_options(dyn_port_string, &mca_pt2pt_tcp_component.tcp_dyn_ports);
        if (0 == strcmp(mca_pt2pt_tcp_component.tcp_dyn_ports[0], "-1")) {
            scon_argv_free(mca_pt2pt_tcp_component.tcp_dyn_ports);
            mca_pt2pt_tcp_component.tcp_dyn_ports = NULL;
        }
    } else {
        mca_pt2pt_tcp_component.tcp_dyn_ports = NULL;
    }

#if SCON_ENABLE_IPV6
    dyn_port_string6 = NULL;
    (void)scon_mca_base_component_var_register(component, "dynamic_ipv6_ports",
                                          "Range of ports to be dynamically used by daemons and procs (IPv6)",
                                          SCON_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                          SCON_INFO_LVL_4,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &dyn_port_string6);
    /* if ports were provided, parse the provided range */
    if (NULL != dyn_port_string6) {
        /* can't have both static and dynamic ports! */
        if (scon_static_ports) {
            char *err4=NULL, *err6=NULL;
            if (NULL != mca_pt2pt_tcp_component.tcp_static_ports) {
                err4 = scon_argv_join(mca_pt2pt_tcp_component.tcp_static_ports, ',');
            }
            if (NULL != mca_pt2pt_tcp_component.tcp6_static_ports) {
                err6 = scon_argv_join(mca_pt2pt_tcp_component.tcp6_static_ports, ',');
            }
            scon_show_help("help-pt2pt-tcp.txt", "static-and-dynamic-ipv6", true,
                           (NULL == err4) ? "N/A" : err4,
                           (NULL == err6) ? "N/A" : err6,
                           dyn_port_string6);
            if (NULL != err4) {
                free(err4);
            }
            if (NULL != err6) {
                free(err6);
            }
            return SCON_ERROR;
        }
        scon_util_parse_range_options(dyn_port_string6, &mca_pt2pt_tcp_component.tcp6_dyn_ports);
        if (0 == strcmp(mca_pt2pt_tcp_component.tcp6_dyn_ports[0], "-1")) {
            scon_argv_free(mca_pt2pt_tcp_component.tcp6_dyn_ports);
            mca_pt2pt_tcp_component.tcp6_dyn_ports = NULL;
        }
    } else {
        mca_pt2pt_tcp_component.tcp6_dyn_ports = NULL;
    }
#endif // SCON_ENABLE_IPV6

    mca_pt2pt_tcp_component.disable_ipv4_family = false;
    (void)scon_mca_base_component_var_register(component, "disable_ipv4_family",
                                          "Disable the IPv4 interfaces",
                                          SCON_MCA_BASE_VAR_TYPE_BOOL, NULL, 0, 0,
                                          SCON_INFO_LVL_4,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &mca_pt2pt_tcp_component.disable_ipv4_family);

#if SCON_ENABLE_IPV6
    mca_pt2pt_tcp_component.disable_ipv6_family = false;
    (void)scon_mca_base_component_var_register(component, "disable_ipv6_family",
                                          "Disable the IPv6 interfaces",
                                          SCON_MCA_BASE_VAR_TYPE_BOOL, NULL, 0, 0,
                                          SCON_INFO_LVL_4,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &mca_pt2pt_tcp_component.disable_ipv6_family);
#endif // SCON_ENABLE_IPV6

    // Default to keepalives every 60 seconds
    mca_pt2pt_tcp_component.keepalive_time = 60;
    (void)scon_mca_base_component_var_register(component, "keepalive_time",
                                          "Idle time in seconds before starting to send keepalives (keepalive_time <= 0 disables keepalive functionality)",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_5,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &mca_pt2pt_tcp_component.keepalive_time);

    // Default to keepalive retry interval time of 5 seconds
    mca_pt2pt_tcp_component.keepalive_intvl = 5;
    (void)scon_mca_base_component_var_register(component, "keepalive_intvl",
                                          "Time between successive keepalive pings when peer has not responded, in seconds (ignored if keepalive_time <= 0)",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_5,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &mca_pt2pt_tcp_component.keepalive_intvl);

    // Default to retrying a keepalive 3 times before declaring the
    // peer kaput
    mca_pt2pt_tcp_component.keepalive_probes = 3;
    (void)scon_mca_base_component_var_register(component, "keepalive_probes",
                                          "Number of keepalives that can be missed before declaring error (ignored if keepalive_time <= 0)",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_5,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &mca_pt2pt_tcp_component.keepalive_probes);

    mca_pt2pt_tcp_component.retry_delay = 0;
    (void)scon_mca_base_component_var_register(component, "retry_delay",
                                          "Time (in sec) to wait before trying to connect to peer again",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_4,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &mca_pt2pt_tcp_component.retry_delay);

    mca_pt2pt_tcp_component.max_recon_attempts = 10;
    (void)scon_mca_base_component_var_register(component, "max_recon_attempts",
                                          "Max number of times to attempt connection before giving up (-1 -> never give up)",
                                          SCON_MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                          SCON_INFO_LVL_4,
                                          SCON_MCA_BASE_VAR_SCOPE_READONLY,
                                          &mca_pt2pt_tcp_component.max_recon_attempts);

    return SCON_SUCCESS;
}



static char **split_and_resolve(char **orig_str, char *name);

static int tcp_component_query(scon_mca_base_module_t **module, int *priority)
{
    int i, rc;
    char **interfaces = NULL;
    bool including = false, excluding = false;
    char name[32];
    struct sockaddr_storage my_ss;
    int kindex;

    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "pt2pt:tcp: component_query called");

    /* if interface include was given, construct a list
     * of those interfaces which match the specifications - remember,
     * the includes could be given as named interfaces, IP addrs, or
     * subnet+mask
     */
    if (NULL != mca_pt2pt_tcp_component.if_include) {
        interfaces = split_and_resolve(&mca_pt2pt_tcp_component.if_include,
                                       "include");
        including = true;
        excluding = false;
    } else if (NULL != mca_pt2pt_tcp_component.if_exclude) {
        interfaces = split_and_resolve(&mca_pt2pt_tcp_component.if_exclude,
                                       "exclude");
        including = false;
        excluding = true;
    }

    /* look at all available interfaces */
    for (i = scon_ifbegin(); i >= 0; i = scon_ifnext(i)) {
        if (SCON_SUCCESS != scon_ifindextoaddr(i, (struct sockaddr*) &my_ss,
                                               sizeof (my_ss))) {
            scon_output (0, "pt2pt_tcp: problems getting address for index %i (kernel index %i)\n",
                         i, scon_ifindextokindex(i));
            continue;
        }
        /* ignore non-ip4/6 interfaces */
        if (AF_INET != my_ss.ss_family
#if SCON_ENABLE_IPV6
            && AF_INET6 != my_ss.ss_family
#endif
            ) {
            continue;
        }
        kindex = scon_ifindextokindex(i);
        if (kindex <= 0) {
            continue;
        }
        scon_output_verbose(10, scon_pt2pt_base_framework.framework_output,
                            "WORKING INTERFACE %d KERNEL INDEX %d FAMILY: %s", i, kindex,
                            (AF_INET == my_ss.ss_family) ? "V4" : "V6");

        /* get the name for diagnostic purposes */
        scon_ifindextoname(i, name, sizeof(name));

        /* ignore any virtual interfaces */
        if (0 == strncmp(name, "vir", 3)) {
            continue;
        }

        /* handle include/exclude directives */
        if (NULL != interfaces) {
            /* check for match */
            rc = scon_ifmatches(kindex, interfaces);
            /* if one of the network specifications isn't parseable, then
             * error out as we can't do what was requested
             */
            if (SCON_ERR_NETWORK_NOT_PARSEABLE == rc) {
                scon_show_help("help-pt2pt-tcp.txt", "not-parseable", true);
                scon_argv_free(interfaces);
                return SCON_ERR_BAD_PARAM;
            }
            /* if we are including, then ignore this if not present */
            if (including) {
                if (SCON_SUCCESS != rc) {
                    scon_output_verbose(20, scon_pt2pt_base_framework.framework_output,
                                        "%s pt2pt:tcp:init rejecting interface %s (not in include list)",
                                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), name);
                    continue;
                }
            } else {
                /* we are excluding, so ignore if present */
                if (SCON_SUCCESS == rc) {
                    scon_output_verbose(20, scon_pt2pt_base_framework.framework_output,
                                        "%s pt2pt:tcp:init rejecting interface %s (in exclude list)",
                                        SCON_PRINT_PROC(SCON_PROC_MY_NAME), name);
                    continue;
                }
            }
        } else {
            /* if no specific interfaces were provided, we ignore the loopback
             * interface unless nothing else is available
             */
            if (1 < scon_ifcount() && scon_ifisloopback(i)) {
                scon_output_verbose(20, scon_pt2pt_base_framework.framework_output,
                                    "%s pt2pt:tcp:init rejecting loopback interface %s",
                                    SCON_PRINT_PROC(SCON_PROC_MY_NAME), name);
                continue;
            }
        }

        /* Refs ticket #3019
         * it would probably be worthwhile to print out a warning if SCON detects multiple
         * IP interfaces that are "up" on the same subnet (because that's a Bad Idea). Note
         * that we should only check for this after applying the relevant include/exclude
         * list MCA params. If we detect redundant ports, we can also automatically ignore
         * them so that applications won't hang.
         */

        /* add this address to our connections */
        if (AF_INET == my_ss.ss_family) {
            scon_output_verbose(10, scon_pt2pt_base_framework.framework_output,
                                "%s pt2pt:tcp:init adding %s to our list of %s connections",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                scon_net_get_hostname((struct sockaddr*) &my_ss),
                                (AF_INET == my_ss.ss_family) ? "V4" : "V6");
            scon_argv_append_nosize(&mca_pt2pt_tcp_component.ipv4conns, scon_net_get_hostname((struct sockaddr*) &my_ss));
        } else if (AF_INET6 == my_ss.ss_family) {
#if SCON_ENABLE_IPV6
            scon_output_verbose(10, scon_pt2pt_base_framework.framework_output,
                                "%s pt2pt:tcp:init adding %s to our list of %s connections",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                scon_net_get_hostname((struct sockaddr*) &my_ss),
                                (AF_INET == my_ss.ss_family) ? "V4" : "V6");
            scon_argv_append_nosize(&mca_pt2pt_tcp_component.ipv6conns, scon_net_get_hostname((struct sockaddr*) &my_ss));
#endif // SCON_ENABLE_IPV6
        } else {
            scon_output_verbose(10, scon_pt2pt_base_framework.framework_output,
                                "%s pt2pt:tcp:init ignoring %s from out list of connections",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                                scon_net_get_hostname((struct sockaddr*) &my_ss));
        }
    }

    /* cleanup */
    if (NULL != interfaces) {
        scon_argv_free(interfaces);
    }

    if (0 == scon_argv_count(mca_pt2pt_tcp_component.ipv4conns)
#if SCON_ENABLE_IPV6
        && 0 == scon_argv_count(mca_pt2pt_tcp_component.ipv6conns)
#endif
        ) {
        if (including) {
            scon_show_help("help-pt2pt-tcp.txt", "no-included-found", true, mca_pt2pt_tcp_component.if_include);
        } else if (excluding) {
            scon_show_help("help-pt2pt-tcp.txt", "excluded-all", true, mca_pt2pt_tcp_component.if_exclude);
        }
        return SCON_ERR_NOT_AVAILABLE;
    }

    /* set the module event base - this is where we would spin off a separate
     * progress thread if so desired */
   //scon_pt2pt_tcp_module.ev_base = scon_globals.evbase;
    *priority = mca_pt2pt_tcp_component.super.priority;
    *module = &scon_pt2pt_tcp_module.base.super;
    return SCON_SUCCESS;
}

static void cleanup(int sd, short args, void *cbdata)
{
    scon_list_item_t * item;
    bool *active = (bool*)cbdata;
    while (NULL != (item = scon_list_remove_first(&mca_pt2pt_tcp_component.listeners))) {
        SCON_RELEASE(item);
    }
    if (NULL != active) {
        *active = false;
    }
}

static scon_pt2pt_module_t* tcp_component_get_module(void)
{
    return &scon_pt2pt_tcp_module.base;
}

static void tcp_component_shutdown(void)
{
    bool active;

    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s TCP SHUTDOWN",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));
#if 0
    if (mca_pt2pt_tcp_component.listen_thread_active) {
        mca_pt2pt_tcp_component.listen_thread_active = false;
        /* tell the thread to exit */
        write(mca_pt2pt_tcp_component.stop_thread[1], &i, sizeof(int));
        scon_thread_join(&mca_pt2pt_tcp_component.listen_thread, NULL);
    } else {
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "listening thread not active");
    }
#endif
    scon_event_t ev;
    active = true;
    scon_event_set(scon_pt2pt_base.pt2pt_evbase, &ev, -1,
                   SCON_EV_WRITE, cleanup, &active);
    scon_event_set_priority(&ev, SCON_ERROR_PRI);
    scon_event_active(&ev, SCON_EV_WRITE, 1);
    SCON_WAIT_FOR_COMPLETION(active);
    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                    "all listeners released");

    /* shutdown the module */
    scon_pt2pt_tcp_fini();
   /* if (NULL != scon_pt2pt_tcp_module.api.finalize) {
        scon_pt2pt_tcp_module.api.finalize();
    }*/
    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s TCP SHUTDOWN done",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME));
}

#if 0
static int component_send(scon_send_t *msg)
{
    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s pt2pt:tcp:send_nb to peer %s:%d",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&msg->dst), msg->tag );

    /* the module is potentially running on its own event
     * base, so all it can do is push our send request
     * onto an event - it cannot tell us if it will
     * succeed. The module will first see if it knows
     * of a way to send the data to the target, and then
     * attempt to send the data. It  will call the cbfunc
     * with the status upon completion - if it can't do it for
     * some reason, it will call the component error
     * function so we can do something about it
     */
    scon_pt2pt_tcp_module.base.send (msg);
    //scon_pt2pt_tcp_module.api.send_nb(msg);
    return SCON_SUCCESS;
}
#endif

static char* component_get_addr(void)
{
    char *cptr=NULL, *tmp, *tp;

    if (!mca_pt2pt_tcp_component.disable_ipv4_family &&
        NULL != mca_pt2pt_tcp_component.ipv4conns) {
        tmp = scon_argv_join(mca_pt2pt_tcp_component.ipv4conns, ',');
        tp = scon_argv_join(mca_pt2pt_tcp_component.ipv4ports, ',');
        asprintf(&cptr, "tcp://%s:%s", tmp, tp);
        free(tmp);
        free(tp);
    }
#if SCON_ENABLE_IPV6
    if (!mca_pt2pt_tcp_component.disable_ipv6_family &&
        NULL != mca_pt2pt_tcp_component.ipv6conns) {
        char *tmp2;

        /* Fixes #2498
         * RFC 3986, section 3.2.2
         * The notation in that case is to encode the IPv6 IP number in square brackets:
         * "http://[2001:db8:1f70::999:de8:7648:6e8]:100/"
         * A host identified by an Internet Protocol literal address, version 6 [RFC3513]
         * or later, is distinguished by enclosing the IP literal within square brackets.
         * This is the only place where square bracket characters are allowed in the URI
         * syntax. In anticipation of future, as-yet-undefined IP literal address formats,
         * an implementation may use an optional version flag to indicate such a format
         * explicitly rather than rely on heuristic determination.
         */
        tmp = scon_argv_join(mca_pt2pt_tcp_component.ipv6conns, ',');
        tp = scon_argv_join(mca_pt2pt_tcp_component.ipv6ports, ',');
        if (NULL == cptr) {
            /* no ipv4 stuff */
            asprintf(&cptr, "tcp6://[%s]:%s", tmp, tp);
        } else {
            asprintf(&tmp2, "%s;tcp6://[%s]:%s", cptr, tmp, tp);
            free(cptr);
            cptr = tmp2;
        }
        free(tmp);
        free(tp);
    }
#endif // SCON_ENABLE_IPV6

    /* return our uri */
    return cptr;
}

static int component_set_addr(scon_proc_t *peer,
                              char **uris)
{
    char **addrs, *hptr;
    char *tcpuri=NULL, *host, *ports;
    int i, j;
    uint16_t af_family = AF_UNSPEC;
    bool found;

    //memcpy(&ui64, (char*)peer, sizeof(uint64_t));
    /* cycle across component parts and see if one belongs to us */
    found = false;

    for (i=0; NULL != uris[i]; i++) {
        tcpuri = strdup(uris[i]);
        if (NULL == tcpuri) {
            scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                "%s pt2pt:tcp: out of memory",
                                 SCON_PRINT_PROC(SCON_PROC_MY_NAME));
            continue;
        }
        if (0 == strncmp(uris[i], "tcp:", 4)) {
            af_family = AF_INET;
            host = tcpuri + strlen("tcp://");
        } else if (0 == strncmp(uris[i], "tcp6:", 5)) {
#if SCON_ENABLE_IPV6
            af_family = AF_INET6;
            host = tcpuri + strlen("tcp6://");
#else // SCON_ENABLE_IPV6
            /* we don't support this connection type */
            scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                "%s pt2pt:tcp: address %s not suppscond",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME), uris[i]);
            free(tcpuri);
            continue;
#endif // SCON_ENABLE_IPV6
        } else {
            /* not one of ours */
            scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                                "%s pt2pt:tcp: ignoring address %s",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME), uris[i]);
            free(tcpuri);
            continue;
        }

        /* this one is ours - record the peer */
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s pt2pt:tcp: working peer %s address %s",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            SCON_PRINT_PROC(peer), uris[i]);
        /* separate the ports from the network addrs */
        ports = strrchr(tcpuri, ':');
        if (NULL == ports) {
            SCON_ERROR_LOG(SCON_ERR_NOT_FOUND);
            free(tcpuri);
            continue;
        }
        *ports = '\0';
        ports++;

        /* split the addrs */
        /* if this is a tcp6 connection, the first one will have a '['
         * at the beginning of it, and the last will have a ']' at the
         * end - we need to remove those extra characters
         */
        hptr = host;
#if SCON_ENABLE_IPV6
        if (AF_INET6 == af_family) {
            if ('[' == host[0]) {
                hptr = &host[1];
            }
            if (']' == host[strlen(host)-1]) {
                host[strlen(host)-1] = '\0';
            }
        }
#endif // SCON_ENABLE_IPV6
        addrs = scon_argv_split(hptr, ',');


        /* cycle across the provided addrs */
        for (j=0; NULL != addrs[j]; j++) {
            /* if they gave us "localhost", then just take the first conn on our list */
            if (0 == strcasecmp(addrs[j], "localhost")) {
#if SCON_ENABLE_IPV6
                if (AF_INET6 == af_family) {
                    if (NULL == mca_pt2pt_tcp_component.ipv6conns ||
                        NULL == mca_pt2pt_tcp_component.ipv6conns[0]) {
                        continue;
                    }
                    host = mca_pt2pt_tcp_component.ipv6conns[0];
                } else {
#endif // SCON_ENABLE_IPV6
                    if (NULL == mca_pt2pt_tcp_component.ipv4conns ||
                        NULL == mca_pt2pt_tcp_component.ipv4conns[0]) {
                        continue;
                    }
                    host = mca_pt2pt_tcp_component.ipv4conns[0];
#if SCON_ENABLE_IPV6
                }
#endif
            } else {
                host = addrs[j];
            }

            /* pass this proc, and its ports, to the
             * module for handling - this module will be responsible
             * for communicating with the proc via this network.
             * Note that the modules are *not* necessarily running
             * on our event base - thus, the modules will push this
             * call into their own event base for processing.
             */
            scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                                "%s PASSING ADDR %s TO MODULE",
                                SCON_PRINT_PROC(SCON_PROC_MY_NAME), host);
            scon_pt2pt_tcp_set_peer(peer, af_family, host, ports);
            //scon_pt2pt_tcp_module.api.set_peer(peer, af_family, host, ports);
            found = true;
        }
        scon_argv_free(addrs);
        free(tcpuri);
    }
    if (found) {
        /* indicate that this peer is addressable by this component */
        return SCON_SUCCESS;
    }

    /* otherwise indicate that it is not addressable by us */
    return SCON_ERR_TAKE_NEXT_OPTION;
}
#if 0
static bool component_is_reachable(scon_proc_t *peer)
{
    scon_proc_t hop;

    /* if we have a route to this peer, then we can reach it */
    hop = scon_topology.get_route(peer);
    if (SCON_RANK_INVALID == hop.rank) {
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s is NOT reachable by TCP",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME));
        return false;
    }
    /* assume we can reach the hop - the module will tell us if it can't
     * when we try to send the first time, and then we'll correct it */
    return true;
}

#endif
void scon_pt2pt_tcp_component_set_module(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_peer_op_t *pop = (scon_pt2pt_tcp_peer_op_t*)cbdata;
    uint64_t proc_name_ui64;
    int rc;
    scon_pt2pt_base_peer_t *bpr;

    scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:set_module called for peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&pop->peer));

    /* make sure the pt2pt knows that we can reach this peer - we
     * are in the same event base as the pt2pt base, so we can
     * directly access its storage
     */
    //memcpy(&ui64, (char*)&pop->peer, sizeof(uint64_t));
    proc_name_ui64 = scon_util_convert_process_name_to_uint64(&pop->peer);
    if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_base.peers,
                                                         proc_name_ui64, (void**)&bpr) || NULL == bpr) {
        bpr = SCON_NEW(scon_pt2pt_base_peer_t);
    }
    scon_bitmap_set_bit(&bpr->addressable, mca_pt2pt_tcp_component.super.idx);
    bpr->module = (scon_pt2pt_module_t*)&scon_pt2pt_tcp_module;
    scon_output(0, "mca_pt2pt_tcp_component_set_module %s setting base hash table %p, key %llu, value %p",
                SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                (void*)&scon_pt2pt_base.peers,
                proc_name_ui64, (void*) bpr);
    if (SCON_SUCCESS != (rc = scon_hash_table_set_value_uint64(&scon_pt2pt_base.peers,
                                                               proc_name_ui64, bpr))) {
        SCON_ERROR_LOG(rc);
    }

    SCON_RELEASE(pop);
}

void scon_pt2pt_tcp_component_lost_connection(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_peer_op_t *pop = (scon_pt2pt_tcp_peer_op_t*)cbdata;
    uint64_t proc_name_ui64;
    scon_pt2pt_base_peer_t *bpr;
    int rc;

    scon_output_verbose(3, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:lost connection called for peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&pop->peer));

    /* Mark that we no longer support this peer */
    proc_name_ui64 = scon_util_convert_process_name_to_uint64(&pop->peer);
    if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_base.peers,
                                                         proc_name_ui64, (void**)&bpr) || NULL == bpr) {
        bpr = SCON_NEW(scon_pt2pt_base_peer_t);
    }
    scon_bitmap_clear_bit(&bpr->addressable, mca_pt2pt_tcp_component.super.idx);

    if (SCON_SUCCESS != (rc = scon_hash_table_set_value_uint64(&scon_pt2pt_base.peers,
                                                               proc_name_ui64, NULL))) {
        SCON_ERROR_LOG(rc);
    }
    /* TO DO */
    /* update topology of the SCON */
   // scon_topology->route.lost(&pop->peer);
    SCON_RELEASE(pop);
}

void scon_pt2pt_tcp_component_no_route(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_msg_error_t *mop = (scon_pt2pt_tcp_msg_error_t*)cbdata;
    uint64_t proc_name_ui64;
    int rc;
    scon_pt2pt_base_peer_t *bpr;
    scon_comm_scon_t *scon = scon_comm_base_get_scon(mop->rmsg->scon_handle);
    scon_output_verbose(3, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:no route called for peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&mop->hop));

    /* mark that we cannot reach this hop */
    proc_name_ui64 = scon_util_convert_process_name_to_uint64( &mop->hop);
    if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_base.peers,
                                                         proc_name_ui64, (void**)&bpr) || NULL == bpr) {
        bpr = SCON_NEW(scon_pt2pt_base_peer_t);
    }
    scon_bitmap_clear_bit(&bpr->addressable, mca_pt2pt_tcp_component.super.idx);
    if (SCON_SUCCESS != (rc = scon_hash_table_set_value_uint64(&scon_pt2pt_base.peers,
                                                               proc_name_ui64, NULL))) {
        SCON_ERROR_LOG(rc);
    }

    /* report the error back to the pt2pt and let it try other components
     * or declare a problem
     */
   // if (!scon_finalizing && !scon_abnormal_term_ordered) {
        /* if this was a lifeline, then alert */
        if (SCON_SUCCESS != scon->topology_module->api.route_lost(&scon->topology_module->topology,
                                    &mop->hop))
   // }

    SCON_RELEASE(mop);
}

void scon_pt2pt_tcp_component_hop_unknown(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_msg_error_t *mop = (scon_pt2pt_tcp_msg_error_t*)cbdata;
    uint64_t proc_name_ui64;
    scon_send_t *snd;
    scon_comm_scon_t *scon = scon_comm_base_get_scon(mop->snd->msg->scon_handle);
    scon_pt2pt_base_peer_t *bpr;

    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:unknown hop called for peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&mop->hop));

    if (SCON_STATE_DELETING == scon->state ) {
        /* just ignore the problem */
        SCON_RELEASE(mop);
        return;
    }

   /* mark that this component cannot reach this hop */
    proc_name_ui64 =scon_util_convert_process_name_to_uint64(&mop->hop);
    if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_base.peers,
                                                         proc_name_ui64, (void**)&bpr) ||
        NULL == bpr) {
        /* the overall pt2pt has no knowledge of this hop. Only
         * way this could happen is if the peer contacted us
         * via this component, and it wasn't entered into the
         * pt2pt framework hash table. We have no way of knowing
         * what to do next, so just output an error message and
         * abort */
        scon_output(0, "%s ERROR: message to %s requires routing and the pt2pt has no knowledge of the reqd hop %s",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&mop->snd->hdr.dst),
                    SCON_PRINT_PROC(&mop->hop));
        SCON_RELEASE(mop);
        return;
    }
    scon_bitmap_clear_bit(&bpr->addressable, mca_pt2pt_tcp_component.super.idx);

    /* mark that this component cannot reach this destination either */
     proc_name_ui64 = scon_util_convert_process_name_to_uint64(&mop->snd->hdr.dst);
    if (SCON_SUCCESS != scon_hash_table_get_value_uint64(&scon_pt2pt_base.peers,
                                                         proc_name_ui64, (void**)&bpr) ||
        NULL == bpr) {
        scon_output(0, "%s ERROR: message to %s requires routing and the pt2pt has no knowledge of this process",
                    SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                    SCON_PRINT_PROC(&mop->snd->hdr.dst));
        SCON_RELEASE(mop);
        return;
    }
    scon_bitmap_clear_bit(&bpr->addressable, mca_pt2pt_tcp_component.super.idx);

    /* post the message to the pt2pt so it can see
     * if another component can transfer it
     */
    SCON_PT2PT_TCP_HDR_NTOH(&mop->snd->hdr);
    snd = SCON_NEW(scon_send_t);
    snd->dst = mop->snd->hdr.dst;
    snd->origin = mop->snd->hdr.origin;
    snd->tag = mop->snd->hdr.tag;
    snd->scon_handle = mop->snd->hdr.scon_handle;
   // snd->seq_num = mop->snd->hdr.seq_num;
 //   snd->data = mop->snd->data;
 //   snd->count = mop->snd->hdr.nbytes;
    snd->cbfunc = NULL;
    snd->cbdata = NULL;
    /* activate the pt2pt send state */
    PT2PT_SEND_MESSAGE(snd);
    /* protect the data */
   // mop->snd->data = NULL;

    SCON_RELEASE(mop);
}

void scon_pt2pt_tcp_component_failed_to_connect(int fd, short args, void *cbdata)
{
    scon_pt2pt_tcp_peer_op_t *pop = (scon_pt2pt_tcp_peer_op_t*)cbdata;

    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:failed_to_connect called for peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&pop->peer));

   /* if we are terminating, then don't attempt to reconnect */
    if ( SCON_STATE_DELETING == pop->scon->state) {
        SCON_RELEASE(pop);
        return;
    }

    /* activate the proc state */
    scon_output_verbose(5, scon_pt2pt_base_framework.framework_output,
                        "%s tcp:failed_to_connect unable to reach peer %s",
                        SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                        SCON_PRINT_PROC(&pop->peer));

    /* if this was a lifeline, then alert */
    if (SCON_SUCCESS != pop->scon->topology_module->api.route_lost(&pop->scon->topology_module->topology,
         &pop->peer)) {

    }
    SCON_RELEASE(pop);
}

/*
 * Go through a list of argv; if there are any subnet specifications
 * (a.b.c.d/e), resolve them to an interface name (Currently only
 * supporting IPv4).  If unresolvable, warn and remove.
 */
static char **split_and_resolve(char **orig_str, char *name)
{
    int i, ret, save, if_index;
    char **argv, *str, *tmp;
    char if_name[IF_NAMESIZE];
    struct sockaddr_storage argv_inaddr, if_inaddr;
    uint32_t argv_prefix;

    /* Sanity check */
    if (NULL == orig_str || NULL == *orig_str) {
        return NULL;
    }

    argv = scon_argv_split(*orig_str, ',');
    if (NULL == argv) {
        return NULL;
    }
    for (save = i = 0; NULL != argv[i]; ++i) {
        if (isalpha(argv[i][0])) {
            argv[save++] = argv[i];
            continue;
        }

        /* Found a subnet notation.  Convert it to an IP
           address/netmask.  Get the prefix first. */
        argv_prefix = 0;
        tmp = strdup(argv[i]);
        str = strchr(argv[i], '/');
        if (NULL == str) {
            scon_show_help("help-pt2pt-tcp.txt", "invalid if_inexclude",
                           true, name,
                           tmp, "Invalid specification (missing \"/\")");
            free(argv[i]);
            free(tmp);
            continue;
        }
        *str = '\0';
        argv_prefix = atoi(str + 1);

        /* Now convert the IPv4 address */
        ((struct sockaddr*) &argv_inaddr)->sa_family = AF_INET;
        ret = inet_pton(AF_INET, argv[i],
                        &((struct sockaddr_in*) &argv_inaddr)->sin_addr);
        free(argv[i]);

        if (1 != ret) {
            scon_show_help("help-pt2pt-tcp.txt", "invalid if_inexclude",
                           true, name, tmp,
                           "Invalid specification (inet_pton() failed)");
            free(tmp);
            continue;
        }
        scon_output_verbose(20, scon_pt2pt_base_framework.framework_output,
                            "%s pt2pt:tcp: Searching for %s address+prefix: %s / %u",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            name,
                            scon_net_get_hostname((struct sockaddr*) &argv_inaddr),
                            argv_prefix);

        /* Go through all interfaces and see if we can find a match */
        for (if_index = scon_ifbegin(); if_index >= 0;
                           if_index = scon_ifnext(if_index)) {
            scon_ifindextoaddr(if_index,
                               (struct sockaddr*) &if_inaddr,
                               sizeof(if_inaddr));
            if (scon_net_samenetwork((struct sockaddr*) &argv_inaddr,
                                     (struct sockaddr*) &if_inaddr,
                                     argv_prefix)) {
                break;
            }
        }
        /* If we didn't find a match, keep trying */
        if (if_index < 0) {
            scon_show_help("help-pt2pt-tcp.txt", "invalid if_inexclude",
                           true, name, tmp,
                           "Did not find interface matching this subnet");
            free(tmp);
            continue;
        }

        /* We found a match; get the name and replace it in the
           argv */
        scon_ifindextoname(if_index, if_name, sizeof(if_name));
        scon_output_verbose(20, scon_pt2pt_base_framework.framework_output,
                            "%s pt2pt:tcp: Found match: %s (%s)",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            scon_net_get_hostname((struct sockaddr*) &if_inaddr),
                            if_name);
        argv[save++] = strdup(if_name);
        free(tmp);
    }

    /* The list may have been compressed if there were invalid
       entries, so ensure we end it with a NULL entry */
    argv[save] = NULL;
    free(*orig_str);
    *orig_str = scon_argv_join(argv, ',');
    return argv;
}

/* pt2pt TCP Class instances */

static void peer_cons(scon_pt2pt_tcp_peer_t *peer)
{
    peer->auth_method = NULL;
    peer->sd = -1;
    SCON_CONSTRUCT(&peer->addrs, scon_list_t);
    peer->active_addr = NULL;
    peer->state = SCON_PT2PT_TCP_UNCONNECTED;
    peer->num_retries = 0;
    SCON_CONSTRUCT(&peer->send_queue, scon_list_t);
    peer->send_msg = NULL;
    peer->recv_msg = NULL;
    peer->send_ev_active = false;
    peer->recv_ev_active = false;
    peer->timer_ev_active = false;
}
static void peer_des(scon_pt2pt_tcp_peer_t *peer)
{
    if (NULL != peer->auth_method) {
        free(peer->auth_method);
    }
    if (peer->send_ev_active) {
        scon_event_del(&peer->send_event);
    }
    if (peer->recv_ev_active) {
        scon_event_del(&peer->recv_event);
    }
    if (peer->timer_ev_active) {
        scon_event_del(&peer->timer_event);
    }
    if (0 <= peer->sd) {
        scon_output_verbose(2, scon_pt2pt_base_framework.framework_output,
                            "%s CLOSING SOCKET %d",
                            SCON_PRINT_PROC(SCON_PROC_MY_NAME),
                            peer->sd);
        CLOSE_THE_SOCKET(peer->sd);
    }
    SCON_LIST_DESTRUCT(&peer->addrs);
    SCON_LIST_DESTRUCT(&peer->send_queue);
}
SCON_CLASS_INSTANCE(scon_pt2pt_tcp_peer_t,
                   scon_list_item_t,
                   peer_cons, peer_des);

static void padd_cons(scon_pt2pt_tcp_addr_t *ptr)
{
    memset(&ptr->addr, 0, sizeof(ptr->addr));
    ptr->retries = 0;
    ptr->state = SCON_PT2PT_TCP_UNCONNECTED;
}
SCON_CLASS_INSTANCE(scon_pt2pt_tcp_addr_t,
                   scon_list_item_t,
                   padd_cons, NULL);


static void pop_cons(scon_pt2pt_tcp_peer_op_t *pop)
{
    pop->net = NULL;
    pop->port = NULL;
}
static void pop_des(scon_pt2pt_tcp_peer_op_t *pop)
{
    if (NULL != pop->net) {
        free(pop->net);
    }
    if (NULL != pop->port) {
        free(pop->port);
    }
}
SCON_CLASS_INSTANCE(scon_pt2pt_tcp_peer_op_t,
                   scon_object_t,
                   pop_cons, pop_des);

SCON_CLASS_INSTANCE(scon_pt2pt_tcp_msg_op_t,
                   scon_object_t,
                   NULL, NULL);

SCON_CLASS_INSTANCE(scon_pt2pt_tcp_conn_op_t,
                   scon_object_t,
                   NULL, NULL);

SCON_CLASS_INSTANCE(scon_pt2pt_tcp_ping_t,
                   scon_object_t,
                   NULL, NULL);

static void nicaddr_cons(scon_pt2pt_tcp_nicaddr_t *ptr)
{
    ptr->af_family = PF_UNSPEC;
    memset(&ptr->addr, 0, sizeof(ptr->addr));
}
SCON_CLASS_INSTANCE(scon_pt2pt_tcp_nicaddr_t,
                   scon_list_item_t,
                   nicaddr_cons, NULL);
