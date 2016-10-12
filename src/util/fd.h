/*
 * Copyright (c) 2008-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2009      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2014-2015 Intel, Inc. All rights reserved.
 *
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/* @file */

#ifndef SCON_UTIL_FD_H_
#define SCON_UTIL_FD_H_

#include <src/include/scon_config.h>


BEGIN_C_DECLS

/**
 * Read a complete buffer from a file descriptor.
 *
 * @param fd File descriptor
 * @param len Number of bytes to read
 * @param buffer Pre-allocated buffer (large enough to hold len bytes)
 *
 * @returns SCON_SUCCESS upon success.
 * @returns SCON_ERR_TIMEOUT if the fd closes before reading the full amount.
 * @returns SCON_ERR_IN_ERRNO otherwise.
 *
 * Loop over reading from the fd until len bytes are read or an error
 * occurs.  EAGAIN and EINTR are transparently handled.
 */
scon_status_t scon_fd_read(int fd, int len, void *buffer);

/**
 * Write a complete buffer to a file descriptor.
 *
 * @param fd File descriptor
 * @param len Number of bytes to write
 * @param buffer Buffer to write from
 *
 * @returns SCON_SUCCESS upon success.
 * @returns SCON_ERR_IN_ERRNO otherwise.
 *
 * Loop over writing to the fd until len bytes are written or an error
 * occurs.  EAGAIN and EINTR are transparently handled.
 */
scon_status_t scon_fd_write(int fd, int len, const void *buffer);

/**
 * Convenience function to set a file descriptor to be close-on-exec.
 *
 * @param fd File descriptor
 *
 * @returns SCON_SUCCESS upon success (or if the system does not
 * support close-on-exec behavior).
 * @returns SCON_ERR_IN_ERRNO otherwise.
 *
 * This is simply a convenience function because there's a few steps
 * to setting a file descriptor to be close-on-exec.
 */
scon_status_t scon_fd_set_cloexec(int fd);

END_C_DECLS

#endif
