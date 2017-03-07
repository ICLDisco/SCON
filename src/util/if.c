/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2009 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2010-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2014      Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "scon_config.h"

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#if defined(__APPLE__) && defined(_LP64)
/* Apple engineering suggested using options align=power as a
   workaround for a bug in OS X 10.4 (Tiger) that prevented ioctl(...,
   SIOCGIFCONF, ...) from working properly in 64 bit mode on Power PC.
   It turns out that the underlying issue is the size of struct
   ifconf, which the kernel expects to be 12 and natural 64 bit
   alignment would make 16.  The same bug appears in 64 bit mode on
   Intel macs, but align=power is a no-op there, so instead, use the
   pack pragma to instruct the compiler to pack on 4 byte words, which
   has the same effect as align=power for our needs and works on both
   Intel and Power PC Macs. */
#pragma pack(push,4)
#endif
#include <net/if.h>
#if defined(__APPLE__) && defined(_LP64)
#pragma pack(pop)
#endif
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif
#include <ctype.h>

#include "src/class/scon_list.h"
#include "src/util/if.h"
#include "src/util/net.h"
#include "src/util/output.h"
#include "src/util/argv.h"
#include "src/util/show_help.h"
#include "src/util/error.h"

#include "src/mca/if/base/base.h"

#ifdef HAVE_STRUCT_SOCKADDR_IN

#ifndef MIN
#  define MIN(a,b)                ((a) < (b) ? (a) : (b))
#endif

/*
 *  Look for interface by name and returns its address
 *  as a dotted decimal formatted string.
 */

int scon_ifnametoaddr(const char* if_name, struct sockaddr* addr, int length)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (strcmp(intf->if_name, if_name) == 0) {
            memcpy(addr, &intf->if_addr, length);
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}


/*
 *  Look for interface by name and returns its
 *  corresponding scon_list index.
 */

int scon_ifnametoindex(const char* if_name)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return -1;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (strcmp(intf->if_name, if_name) == 0) {
            return intf->if_index;
        }
    }
    return -1;
}


/*
 *  Look for interface by name and returns its
 *  corresponding kernel index.
 */

int16_t scon_ifnametokindex(const char* if_name)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return -1;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (strcmp(intf->if_name, if_name) == 0) {
            return intf->if_kernel_index;
        }
    }
    return -1;
}


/*
 *  Look for interface by scon_list index and returns its
 *  corresponding kernel index.
 */

SCON_EXPORT int scon_ifindextokindex(int if_index)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return -1;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (if_index == intf->if_index) {
            return intf->if_kernel_index;
        }
    }
    return -1;
}


/*
 *  Attempt to resolve the adddress (given as either IPv4/IPv6 string
 *  or hostname) and lookup corresponding interface.
 */

int scon_ifaddrtoname(const char* if_addr, char* if_name, int length)
{
    scon_if_t* intf;
    int error;
    struct addrinfo hints, *res = NULL, *r;

    /* if the user asked us not to resolve interfaces, then just return */
    if (scon_if_do_not_resolve) {
        /* return not found so ifislocal will declare
         * the node to be non-local
         */
        return SCON_ERR_NOT_FOUND;
    }

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(if_addr, NULL, &hints, &res);

    if (error) {
        if (NULL != res) {
            freeaddrinfo (res);
        }
        return SCON_ERR_NOT_FOUND;
    }

    for (r = res; r != NULL; r = r->ai_next) {
        for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
            intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
            intf =  (scon_if_t*)scon_list_get_next(intf)) {

            if (AF_INET == r->ai_family) {
                struct sockaddr_in ipv4;
                struct sockaddr_in *inaddr;

                inaddr = (struct sockaddr_in*) &intf->if_addr;
                memcpy (&ipv4, r->ai_addr, r->ai_addrlen);

                if (inaddr->sin_addr.s_addr == ipv4.sin_addr.s_addr) {
                    strncpy(if_name, intf->if_name, length);
                    freeaddrinfo (res);
                    return SCON_SUCCESS;
                }
            }
#if SCON_ENABLE_IPV6
            else {
                if (IN6_ARE_ADDR_EQUAL(&((struct sockaddr_in6*) &intf->if_addr)->sin6_addr,
                    &((struct sockaddr_in6*) r->ai_addr)->sin6_addr)) {
                    strncpy(if_name, intf->if_name, length);
                    freeaddrinfo (res);
                    return SCON_SUCCESS;
                }
            }
#endif
        }
    }
    if (NULL != res) {
        freeaddrinfo (res);
    }

    /* if we get here, it wasn't found */
    return SCON_ERR_NOT_FOUND;
}

/*
 *  Attempt to resolve the address (given as either IPv4/IPv6 string
 *  or hostname) and return the kernel index of the interface
 *  on the same network as the specified address
 */
int16_t scon_ifaddrtokindex(const char* if_addr)
{
    scon_if_t* intf;
    int error;
    struct addrinfo hints, *res = NULL, *r;
    int if_kernel_index;
    size_t len;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(if_addr, NULL, &hints, &res);

    if (error) {
        if (NULL != res) {
            freeaddrinfo (res);
        }
        return SCON_ERR_NOT_FOUND;
    }

    for (r = res; r != NULL; r = r->ai_next) {
        SCON_LIST_FOREACH(intf, &scon_if_list, scon_if_t) {
            if (AF_INET == r->ai_family && AF_INET == intf->af_family) {
                struct sockaddr_in ipv4;
                len = (r->ai_addrlen < sizeof(struct sockaddr_in)) ? r->ai_addrlen : sizeof(struct sockaddr_in);
                memcpy(&ipv4, r->ai_addr, len);
                if (scon_net_samenetwork((struct sockaddr*)&ipv4, (struct sockaddr*)&intf->if_addr, intf->if_mask)) {
                    if_kernel_index = intf->if_kernel_index;
                    freeaddrinfo (res);
                    return if_kernel_index;
                }
            }
#if SCON_ENABLE_IPV6
            else if (AF_INET6 == r->ai_family && AF_INET6 == intf->af_family) {
                struct sockaddr_in6 ipv6;
                len = (r->ai_addrlen < sizeof(struct sockaddr_in6)) ? r->ai_addrlen : sizeof(struct sockaddr_in6);
                memcpy(&ipv6, r->ai_addr, len);
                if (scon_net_samenetwork((struct sockaddr*)((struct sockaddr_in6*)&intf->if_addr),
                                         (struct sockaddr*)&ipv6, intf->if_mask)) {
                    if_kernel_index = intf->if_kernel_index;
                    freeaddrinfo (res);
                    return if_kernel_index;
                }
            }
#endif
        }
    }
    if (NULL != res) {
        freeaddrinfo (res);
    }
    return SCON_ERR_NOT_FOUND;
}

/*
 *  Return the number of discovered interface.
 */

SCON_EXPORT int scon_ifcount(void)
{
    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return 0;
    }

    return scon_list_get_size(&scon_if_list);
}


/*
 *  Return the scon_list interface index for the first
 *  interface in our list.
 */

SCON_EXPORT int scon_ifbegin(void)
{
    scon_if_t *intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return -1;
    }

    intf = (scon_if_t*)scon_list_get_first(&scon_if_list);
    if (NULL != intf)
        return intf->if_index;
    return (-1);
}


/*
 *  Located the current position in the list by if_index and
 *  return the interface index of the next element in our list
 *  (if it exists).
 */

SCON_EXPORT int scon_ifnext(int if_index)
{
    scon_if_t *intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return -1;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_index == if_index) {
            do {
                scon_if_t* if_next = (scon_if_t*)scon_list_get_next(intf);
                scon_if_t* if_end =  (scon_if_t*)scon_list_get_end(&scon_if_list);
                if (if_next == if_end) {
                    return -1;
                }
                intf = if_next;
            } while(intf->if_index == if_index);
            return intf->if_index;
        }
    }
    return (-1);
}


/*
 *  Lookup the interface by scon_list index and return the
 *  primary address assigned to the interface.
 */

SCON_EXPORT int scon_ifindextoaddr(int if_index, struct sockaddr* if_addr, unsigned int length)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
         intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
         intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_index == if_index) {
            memcpy(if_addr, &intf->if_addr, MIN(length, sizeof (intf->if_addr)));
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}


/*
 *  Lookup the interface by scon_list kindex and return the
 *  primary address assigned to the interface.
 */
int scon_ifkindextoaddr(int if_kindex, struct sockaddr* if_addr, unsigned int length)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
         intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
         intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_kernel_index == if_kindex) {
            memcpy(if_addr, &intf->if_addr, MIN(length, sizeof (intf->if_addr)));
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}


/*
 *  Lookup the interface by scon_list index and return the
 *  network mask assigned to the interface.
 */

int scon_ifindextomask(int if_index, uint32_t* if_mask, int length)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_index == if_index) {
            memcpy(if_mask, &intf->if_mask, length);
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}

/*
 *  Lookup the interface by scon_list index and return the
 *  MAC assigned to the interface.
 */

int scon_ifindextomac(int if_index, uint8_t mac[6])
{
    scon_if_t* intf;

    for (intf = (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf = (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_index == if_index) {
            memcpy(mac, &intf->if_mac, 6);
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}

/*
 *  Lookup the interface by scon_list index and return the
 *  MTU assigned to the interface.
 */

int scon_ifindextomtu(int if_index, int *mtu)
{
    scon_if_t* intf;

    for (intf = (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf = (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_index == if_index) {
            *mtu = intf->ifmtu;
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}

/*
 *  Lookup the interface by scon_list index and return the
 *  flags assigned to the interface.
 */

int scon_ifindextoflags(int if_index, uint32_t* if_flags)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_index == if_index) {
            memcpy(if_flags, &intf->if_flags, sizeof(uint32_t));
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}



/*
 *  Lookup the interface by scon_list index and return
 *  the associated name.
 */

SCON_EXPORT int scon_ifindextoname(int if_index, char* if_name, int length)
{
    scon_if_t *intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_index == if_index) {
            strncpy(if_name, intf->if_name, length);
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}


/*
 *  Lookup the interface by kernel index and return
 *  the associated name.
 */

int scon_ifkindextoname(int if_kindex, char* if_name, int length)
{
    scon_if_t *intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_kernel_index == if_kindex) {
            strncpy(if_name, intf->if_name, length);
            return SCON_SUCCESS;
        }
    }
    return SCON_ERROR;
}


#define ADDRLEN 100
bool
scon_ifislocal(const char *hostname)
{
#if SCON_ENABLE_IPV6
    char addrname[NI_MAXHOST]; /* should be larger than ADDRLEN, but I think
                                  they really mean IFNAMESIZE */
#else
    char addrname[ADDRLEN + 1];
#endif

    if (SCON_SUCCESS == scon_ifaddrtoname(hostname, addrname, ADDRLEN)) {
        return true;
    }

    return false;
}

static int parse_ipv4_dots(const char *addr, uint32_t* net, int* dots)
{
    const char *start = addr, *end;
    uint32_t n[]={0,0,0,0};
    int i;

    /* now assemble the address */
    for( i = 0; i < 4; i++ ) {
        n[i] = strtoul(start, (char**)&end, 10);
        if( end == start ) {
            /* this is not an error, but indicates that
             * we were given a partial address - e.g.,
             * 192.168 - usually indicating an IP range
             * in CIDR notation. So just return what we have
             */
            break;
        }
        /* did we read something sensible? */
        if( n[i] > 255 ) {
            return SCON_ERR_NETWORK_NOT_PARSEABLE;
        }
        /* skip all the . */
        for( start = end; '\0' != *start; start++ )
            if( '.' != *start ) break;
    }
    *dots = i;
    *net = SCON_IF_ASSEMBLE_NETWORK(n[0], n[1], n[2], n[3]);
    return SCON_SUCCESS;
}

int
scon_iftupletoaddr(const char *inaddr, uint32_t *net, uint32_t *mask)
{
    int pval, dots, rc = SCON_SUCCESS;
    const char *ptr;

    /* if a mask was desired... */
    if (NULL != mask) {
        /* set default */
        *mask = 0xFFFFFFFF;

        /* if entry includes mask, split that off */
        if (NULL != (ptr = strchr(inaddr, '/'))) {
            ptr = ptr + 1;  /* skip the / */
            /* is the mask a tuple? */
            if (NULL != strchr(ptr, '.')) {
                /* yes - extract mask from it */
                rc = parse_ipv4_dots(ptr, mask, &dots);
            } else {
                /* no - must be an int telling us how much of the addr to use: e.g., /16
                 * For more information please read http://en.wikipedia.org/wiki/Subnetwork.
                 */
                pval = strtol(ptr, NULL, 10);
                if ((pval > 31) || (pval < 1)) {
                    scon_output(0, "scon_iftupletoaddr: unknown mask");
                    return SCON_ERR_NETWORK_NOT_PARSEABLE;
                }
                *mask = 0xFFFFFFFF << (32 - pval);
            }
        } else {
            /* use the number of dots to determine it */
            for (ptr = inaddr, pval = 0; '\0'!= *ptr; ptr++) {
                if ('.' == *ptr) {
                    pval++;
                }
            }
            /* if we have three dots, then we have four
             * fields since it is a full address, so the
             * default netmask is fine
             */
            if (3 == pval) {
                *mask = 0xFFFFFFFF;
            } else if (2 == pval) {         /* 2 dots */
                *mask = 0xFFFFFF00;
            } else if (1 == pval) {  /* 1 dot */
                *mask = 0xFFFF0000;
            } else if (0 == pval) {  /* no dots */
                *mask = 0xFF000000;
            } else {
                scon_output(0, "scon_iftupletoaddr: unknown mask");
                return SCON_ERR_NETWORK_NOT_PARSEABLE;
            }
        }
    }

    /* if network addr is desired... */
    if (NULL != net) {
        /* now assemble the address */
        rc = parse_ipv4_dots(inaddr, net, &dots);
    }

    return rc;
}

/*
 *  Determine if the specified interface is loopback
 */

SCON_EXPORT bool scon_ifisloopback(int if_index)
{
    scon_if_t* intf;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return SCON_ERROR;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        if (intf->if_index == if_index) {
            if ((intf->if_flags & IFF_LOOPBACK) != 0) {
                return true;
            }
        }
    }
    return false;
}

/* Determine if an interface matches any entry in the given list, taking
 * into account that the list entries could be given as named interfaces,
 * IP addrs, or subnet+mask
 */
int scon_ifmatches(int kidx, char **nets)
{
    bool named_if;
    int i, rc;
    size_t j;
    int kindex;
    struct sockaddr_in inaddr;
    uint32_t addr, netaddr, netmask;

    /* get the address info for the given network in case we need it */
    if (SCON_SUCCESS != (rc = scon_ifkindextoaddr(kidx, (struct sockaddr*)&inaddr, sizeof(inaddr)))) {
        return rc;
    }
    addr = ntohl(inaddr.sin_addr.s_addr);

    for (i=0; NULL != nets[i]; i++) {
        /* if the specified interface contains letters in it, then it
         * was given as an interface name and not an IP tuple
         */
        named_if = false;
        for (j=0; j < strlen(nets[i]); j++) {
            if (isalpha(nets[i][j]) && '.' != nets[i][j]) {
                named_if = true;
                break;
            }
        }
        if (named_if) {
            if (0 > (kindex = scon_ifnametokindex(nets[i]))) {
                continue;
            }
            if (kindex == kidx) {
                return SCON_SUCCESS;
            }
        } else {
            if (SCON_SUCCESS != (rc = scon_iftupletoaddr(nets[i], &netaddr, &netmask))) {
                scon_show_help("help-scon-util.txt", "invalid-net-mask", true, nets[i]);
                return rc;
            }
            if (netaddr == (addr & netmask)) {
                return SCON_SUCCESS;
            }
        }
    }
    /* get here if not found */
    return SCON_ERR_NOT_FOUND;
}

void scon_ifgetaliases(char ***aliases)
{
    scon_if_t* intf;
    char ipv4[INET_ADDRSTRLEN];
    struct sockaddr_in *addr;
#if SCON_ENABLE_IPV6
    char ipv6[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *addr6;
#endif

    /* set default answer */
    *aliases = NULL;

    if (SCON_SUCCESS != scon_mca_base_framework_open(&scon_if_base_framework, 0)) {
        return;
    }

    for (intf =  (scon_if_t*)scon_list_get_first(&scon_if_list);
        intf != (scon_if_t*)scon_list_get_end(&scon_if_list);
        intf =  (scon_if_t*)scon_list_get_next(intf)) {
        addr = (struct sockaddr_in*) &intf->if_addr;
        /* ignore purely loopback interfaces */
        if ((intf->if_flags & IFF_LOOPBACK) != 0) {
            continue;
        }
        if (addr->sin_family == AF_INET) {
            inet_ntop(AF_INET, &(addr->sin_addr.s_addr), ipv4, INET_ADDRSTRLEN);
            scon_argv_append_nosize(aliases, ipv4);
        }
#if SCON_ENABLE_IPV6
        else {
            addr6 = (struct sockaddr_in6*) &intf->if_addr;
            inet_ntop(AF_INET6, &(addr6->sin6_addr), ipv6, INET6_ADDRSTRLEN);
            scon_argv_append_nosize(aliases, ipv6);
        }
#endif
    }
}

#else /* HAVE_STRUCT_SOCKADDR_IN */

/* if we don't have struct sockaddr_in, we don't have traditional
   ethernet devices.  Just make everything a no-op error call */

int
scon_ifnametoaddr(const char* if_name,
                  struct sockaddr* if_addr, int size)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifaddrtoname(const char* if_addr,
                  char* if_name, int size)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifnametoindex(const char* if_name)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int16_t
scon_ifnametokindex(const char* if_name)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifindextokindex(int if_index)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifcount(void)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifbegin(void)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifnext(int if_index)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifindextoname(int if_index, char* if_name, int length)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifkindextoname(int kif_index, char* if_name, int length)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifindextoaddr(int if_index, struct sockaddr* if_addr, unsigned int length)
{
    return SCON_ERR_NOT_SUPPORTED;
}

int
scon_ifindextomask(int if_index, uint32_t* if_addr, int length)
{
    return SCON_ERR_NOT_SUPPORTED;
}

bool
scon_ifislocal(const char *hostname)
{
    return false;
}

int
scon_iftupletoaddr(const char *inaddr, uint32_t *net, uint32_t *mask)
{
    return 0;
}

int scon_ifmatches(int idx, char **nets)
{
    return SCON_ERR_NOT_SUPPORTED;
}

void scon_ifgetaliases(char ***aliases)
{
    /* set default answer */
    *aliases = NULL;
}

#endif /* HAVE_STRUCT_SOCKADDR_IN */

