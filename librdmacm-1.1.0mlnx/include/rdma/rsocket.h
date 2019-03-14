/*
 * Copyright (c) 2011-2012 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined(RSOCKET_H)
#define RSOCKET_H

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/mman.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \note rsocket's domain includes PF_INET, PF_INET6, SOCK_STREAM and SOCK_DGRAM.
 * And PF_* is the same as AF_*.
 * \see https://stackoverflow.com/questions/6729366/what-is-the-difference-between-af-inet-and-pf-inet-in-socket-programming.
 * \note Note that rsockets fd's cannot be passed into non-rsocket calls. For
 * applications which must mix rsocket fd's with standard socket fd's or opened
 * files, rpoll and rselect support polling both rsockets and normal fd's.
 *
 * Existing applications can make use of rsockets through the use of a preload
 * library. Because rsockets implements an end-to-end protocol, both sides of a
 * connection must use rsockets. The rdma_cm library provides such a preload
 * library, librspreload. To reduce the chance of the preload library intercepting
 * calls without the user's explicit knowledge, the librspreload library is
 * installed into %libdir%/rsocket subdirectory.
 *
 * The preload library can be used by setting LD_PRELOAD when running. Note
 * that not all applications will work with  rsockets. Support is limited based
 * on the socket options used by the application. Support for fork() is limited,
 * but available. To use rsockets with the preload library for applications that
 * call fork, users must set the environment variable RDMAV_FORK_SAFE=1 on both
 * the client and server side of the connection. In general, fork is supportable
 * for server applications that accept a connection, then fork off a process to
 * handle the new connection.
 */
int rsocket(int domain, int type, int protocol);
int rbind(int socket, const struct sockaddr *addr, socklen_t addrlen);
int rlisten(int socket, int backlog);
int raccept(int socket, struct sockaddr *addr, socklen_t *addrlen);
int rconnect(int socket, const struct sockaddr *addr, socklen_t addrlen);

/**
 * \brief Shut down part of a full-duplex connection.
 *
 * The shutdown() call causes all or part of a full-duplex connection on the
 * socket associated with sockfd to be shut down. If how is SHUT_RD, further
 * receptions will be disallowed. If how is SHUT_WR, further transmissions will
 * be disallowed. If how is HUT_RDWR, further receptions and transmissions will
 * be disallowed.
 *
 * \return On success, zero is returned. On error, -1 is returned, and errno
 * is set appropriately.
 *
 * \see http://man7.org/linux/man-pages/man8/shutdown.8.html
 */
int rshutdown(int socket, int how);
int rclose(int socket);

/**
 * \brief Receive a message from a socket.
 *
 * \param flags is formed by ORing one or more of the following values:
 * - MSG_DONTWAIT enables nonblocking operation; if the operation would block,
 *   the call fails with the error EAGAIN or EWOULDBLOCK. This provides similar
 *   behavior to setting the O_NONBLOCK flag (via the fcntl(2) F_SETFL operation),
 *   but differs in that MSG_DONTWAIT is a percall option, wheras O_NONBLOCK is
 *   a setting on the open file description (see open(2)), which will affect all
 *   threads in the calling process and as well as other processes that hold
 *   file descriptors referring to the same open file description.
 * - MSG_PEEK causes the receive operation to return data from the beginning
 *   of the receive queue without removing that from the queue. Thus, a subsequent
 *   receive call will return the same data.
 */
ssize_t rrecv(int socket, void *buf, size_t len, int flags);

/**
 * \details recvfrom() places the received message into buffer buf. The caller
 * must specify the size of the buffer in len. If src_addr is not NULL, and the
 * underlying protocol provides the source address of the message, that source
 * address is placed in the buffer pointed to by src_addr. In this case, addrlen
 * is a value-result argument. Before the call, it should be initialized to
 * the size of the buffer associated with src_addr. Upon return, addrlen is updated
 * to contain the actual size of the source address. The returned address is
 * truncated if the buffer provided is too small; in the case, addrlen will
 * return a vaule greater than was supplied to the call. If the caller is not
 * interested in the source address, src_addr and addrlen should be specified
 * as NULL.
 *
 * \see http://man7.org/linux/man-pages/man2/recv.2.html
 */
ssize_t rrecvfrom(int socket, void *buf, size_t len, int flags,
		  struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t rrecvmsg(int socket, struct msghdr *msg, int flags);
ssize_t rsend(int socket, const void *buf, size_t len, int flags);
ssize_t rsendto(int socket, const void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t rsendmsg(int socket, const struct msghdr *msg, int flags);
ssize_t rread(int socket, void *buf, size_t count);

/**
 * \brief Read or write data into multiple buffers.
 *
 * The readv() system call reads iovcnt buffers from the file associated with
 * the file descriptor fd into the buffers described by iov ("scatter input").
 * The writev() system call writes iovcnt buffers of data described by iov to
 * the file associated with the file descriptor fd ("gather output"). The pointer
 * iov points to an array of iovec structures, defined in <sys/uio.h> as:
 * \code
 * struct iovec {
 *     void  *iov_base;
 *     size_t iov_len;
 * };
 * \endcode
 *
 * The readv() system call works just like read(2) except that multiple buffers
 * are filled. The writev() system call works just like write(2) except that
 * multiple buffers are written out.
 *
 * Buffers are processed in array order. This means that readv() completely fills
 * iov[0] before proceeding to iov[1], and so on. (If there is insufficient data,
 * then not all buffers pointed to by iov may be filled.) Similarly, writev()
 * writes out the entire contents of iov[0] before proceeding to iov[1], and so on.
 *
 * The data transfers performed by readv() and writev() are atomic: the data
 * written by writev() is written as a single block that is not intermigled with
 * output from writes in other processes; analogously, reav() is guaranteed to
 * read a contiguous block of data from the file, regardless of read operations
 * performed in other threads or processes that hava file descriptors referring
 * to the same open file description.
 *
 * \see http://man7.org/linux/man-pages/man2/readv.2.html
 */
ssize_t rreadv(int socket, const struct iovec *iov, int iovcnt);
ssize_t rwrite(int socket, const void *buf, size_t count);
ssize_t rwritev(int socket, const struct iovec *iov, int iovcnt);

/**
 * \brief Wait for some event on a file descriptor.
 *
 * poll() performs a similar task to select(2): it waits for one of a set of
 * file descriptors to become ready to perform I/O. The set of fds to be
 * monitored is specified in the fds argument, which is an array of the following
 * form:
 * \code
 * struct pollfd {
 *     int   fd;
 *     short events;
 *     short revents;
 * };
 * \endcode
 *
 * \see http://man7.org/linux/man-pages/man2/poll.2.html
 */
int rpoll(struct pollfd *fds, nfds_t nfds, int timeout);

/**
 * \see http://man7.org/linux/man-pages/man2/select.2.html
 */
int rselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    struct timeval *timeout);

int rgetpeername(int socket, struct sockaddr *addr, socklen_t *addrlen);
int rgetsockname(int socket, struct sockaddr *addr, socklen_t *addrlen);

#define SOL_RDMA 0x10000
enum { RDMA_SQSIZE, RDMA_RQSIZE, RDMA_INLINE, RDMA_IOMAPSIZE, RDMA_ROUTE };

/**
 * \brief Set the socket options.
 *
 * \param level specifies the protocol level at which the options resides.
 * e.g. SOL_SOCKET, IPPROTO_TCP, IPPROTO_IPV6
 * \param optname specifies a single option to set,
 * - SOL_SOCKET: SO_ERROR, SO_KEEPLIVE (flag supported, but ignored), SO_LINGER,
 *   SO_OOBINLINE, SO_RCVBUF, SO_REUSEADDR, SO_SNDBUF
 * - IPPROTO_TCP: TCP_NODELAY, TCP_MAXSEG
 * - IPPROTO_IPV6, IPV6_V6ONLY
 * \see http://man7.org/linux/man-pages/man3/setsockopt.3p.html
 *
 * \note The socket options listed below can be set by using setsockopt and
 * read with getsockopt(2) with the socket level set to SOL_SOCKET for all
 * sockets. Unless otherwise noted, optval is a pointer to an int.
 * - SO_ERROR, Get and clear the pending socket error. This socket option is
 *   read-only. Expects an integer.
 * - SO_KEEPLIVE, Enable sending of keep-live messages on connection-oriented
 *   sockets. Expects an integer boolean flag.
 * - SO_LINGER, Sets or gets the SO_LINGER option. The arguments is linger
 *   structure.
 *   \code
 *   struct linger {
 *       int l_onoff;
 *       int l_linger;
 *   };
 *   \endcode
 *   When enabled, a close(2) or shutdown(2) will not return until all queued
 *   messages for the socket have been successfully sent or the linger timeout
 *   has been reached. Otherwise, the call returns immediately and the closing
 *   is done in the background. When the socket is closed as part of exit(2), it
 *   always lingers in the background.
 * - SO_OOBINLINE, If this option is enabled, out-of-band data is directly placed
 *   into the receive data stream.  Otherwise, out-of-band data is passed only
 *   when the MSG_OOB flag is set during receiving.
 * - SO_RCVBUF, Sets or gets the maximum socket receive buffer in bytes.  The
 *   kernel doubles this value (to allow space for bookkeeping overhead) when
 *   it is set using setsockopt(2), and this doubled value is returned by
 *   getsockopt(2).  The default value is set by the /proc/sys/net/core/rmem_default
 *   file, and the maximum allowed value is set by the /proc/sys/net/core/rmem_max
 *   file. The minimum (doubled) value for this option is 256.
 * - SO_REUSEADDR, Indicates that the rules used in validating addresses supplied
 *   in a bind(2) call should allow reuse of local addresses.  For AF_INET
 *   sockets this means that a socket may bind, except when there is an active
 *   listening socket bound to the address. When the listening socket is bound
 *   to INADDR_ANY with a specific port then it is not possible to bind to this
 *   port for any local address. Argument is an integer boolean flag.
 * - SO_SNDBUF, Sets or gets the maximum socket send buffer in bytes.  The
 *   kernel doubles this value (to allow space for bookkeeping overhead) when
 *   it is set using setsockopt(2), and this doubled value is returned by
 *   getsockopt(2).  The default value is set by the /proc/sys/net/core/wmem_default
 *   file and the maximum allowed value is set by the /proc/sys/net/core/wmem_max
 *   file. The minimum (doubled) value for this option is 2048.
 * \see http://man7.org/linux/man-pages/man7/socket.7.html
 *
 * \note To set or get a TCP socket option, call getsockopt(2) to read or
 * setsockopt(2) to write the option with the option level argument set to
 * IPPROTO_TCP. Unless otherwise noted, optval is a pointer to an int. In
 * addition, most IPPROTO_IP socket options are valid on TCP sockets. For
 * more information see ip(7).
 * - TCP_NODELAY, If set, disable the Nagle algorithm.  This means that segments are always
 *   sent as soon as possible, even if there is only a small amount of data.
 *   When not set, data is buffered until there is a sufficient amount to send
 *   out, thereby avoiding the frequent sending of small packets, which results
 *   in poor utilization of the network.  This option is overridden by TCP_CORK;
 *   however, setting this option forces an explicit flush of pending output,
 *   even if TCP_CORK is currently set.
 * - TCP_MAXSEG, The maximum segment size for outgoing TCP packets.  In Linux
 *   2.2 and earlier, and in Linux 2.6.28 and later, if this option is set before
 *   connection establishment, it also changes the MSS value announced to the
 *   other end in the initial packet. Values greater than the (eventual) interface
 *   MTU have no effect.  TCP will also impose its minimum and maximum bounds
 *   over the value provided.
 * - IPV6_V6ONLY, (since Linux 2.4.21 and 2.6) If this flag is set to true
 *   (nonzero), then the socket is restricted to sending and receiving IPv6
 *   packets only.  In this case, an IPv4 and an IPv6 application can bind to a
 *   single port at the same time. If this flag is set to false (zero), then the
 *   socket can be used to send and receive packets to and from an IPv6 address
 *   or an IPv4-mapped IPv6 address. The argument is a pointer to a boolean value
 *   in an integer. The default value for this flag is defined by the contents of
 *   the file /proc/sys/net/ipv6/bindv6only. The default value for that file is
 *   0 (false).
 * \see http://man7.org/linux/man-pages/man7/tcp.7.html
 *
 * \note In  addition to standard socket options, rsockets supports options
 * specific to RDMA devices and protocols. These options are accessible through
 * rsetsockopt using SOL_RDMA option level.
 * - RDMA_SQSIZE, Integer size of the underlying send queue.
 * - RDMA_RQSIZE, Integer size of the underlying receive queue.
 * - RDMA_INLINE, Integer size of inline data.
 * - RDMA_IOMAPSIZE, Integer number of remote IO mappings supported.
 * - RDMA_ROUTE, struct ibv_path_data of path record for connection.
 *
 */
int rsetsockopt(int socket, int level, int optname, const void *optval,
		socklen_t optlen);
int rgetsockopt(int socket, int level, int optname, void *optval,
		socklen_t *optlen);
int rfcntl(int socket, int cmd, ... /* arg */);

/**
 * \details Rsockets provides extensions beyond normal socket routines that allow
 * for direct placement of data into an application's buffer. This is also known
 * as zero-copy support, since data is sent and received directly, bypassing
 * copies into network controlled buffers. The following calls and options
 * support direct data placement.
 *
 * Riomap registers an application buffer with RDMA hardwar associated
 * with an rsocket. The buffer is registered either for local only access (PROT_NONE)
 * or for remote write access (PROT_WRITE). When registered for remote access,
 * the buffer is mapped to a given offset. The offset is either provided by the
 * user, or if the user selects -1 for the oofset, rsockets selects one. The
 * remote peer may access an iomapped buffer directly by specifying the correct
 * offset. The remote peer is not guaranteed to be available until after the remote
 * peer receives a data transfer initiated after riomap has completed.
 *
 * In order to enable the use of remote IO mapping calls on an rsocket, an
 * application must set the number of IO mappings that are available to the remote
 * peer. This may be done using the rsetsockopt RDMA_IOMAPSIZE option. By
 * default, an rsocket does not support remote IO mappings.
 *
 */
off_t riomap(int socket, void *buf, size_t len, int prot, int flags,
	     off_t offset);
/**
 * \brief Riounmap removes the mapping between a buffer and an rsocekt.
 */
int riounmap(int socket, void *buf, size_t len);
/**
 * \details Riowrite allows an application to transfer data over an rsocket
 * directly into a remotely iomapped buffer. The remote buffer is specified
 * through an offset parameter, which corresponds to a remote iomapped buffer.
 * From the sender's perspective, riowrite behaves similar to rwrite. From a
 * receiver's view, riowrite transfers are silently redirected into a pre-determined
 * data buffer. However, iowrite data is still considered part of the data stream,
 * such that iowrite data will be written before a subsequent transfer is received.
 * A message sent immediately after initiating an iowrite may be used to notify
 * the receiver of the iowrite.
 */
size_t riowrite(int socket, const void *buf, size_t count, off_t offset,
		int flags);

#ifdef __cplusplus
}
#endif

#endif /* RSOCKET_H */
