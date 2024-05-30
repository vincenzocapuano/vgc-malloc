//
// Copyright (C) 2015-2024 by Vincenzo Capuano
//
#if defined(VGC_MALLOC_MPROTECT) || defined(VGC_MALLOC_PKEYMPROTECT)

#include <errno.h>
#include <unistd.h>
#include <sys/un.h>

#include "vgc_common.h"
#include "vgc_message.h"
#include "vgc_network.h"


static const char *moduleName = "VGC-NETWORK";


bool do_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen, const char *socketName)
{
	if (bind(sockfd, addr, addrlen) == 0) return true;

	switch(errno) {
		// 1. The address is protected, and the user is not the superuser.
		// 2. Search permission is denied on a component of the path prefix. (See also path_resolution(7).)
		//
		case EACCES:

		// 1. The given address is already in use.
		// 2. (Internet domain sockets) The port number was specified as zero in the socket address structure, but, upon attempting to bind to an ephemeral port,
		//    it was determined that all port numbers in the ephemeral port range are currently in use.
		//    See the discussion of /proc/sys/net/ipv4/ip_local_port_range ip(7).
		//
		case EADDRINUSE:

		// sockfd is not a valid file descriptor.
		//
		case EBADF:

		// 1. The socket is already bound to an address.
		// 2. addrlen is wrong, or addr is not a valid address for this socket's domain.
		//
		case EINVAL:

		// The file descriptor sockfd does not refer to a socket.
		//
		case ENOTSOCK:

		// A nonexistent interface was requested or the requested address was not local.
		//
		case EADDRNOTAVAIL:

		// addr points outside the user's accessible address space.
		//
		case EFAULT:

		// Too many symbolic links were encountered in resolving addr.
		//
		case ELOOP:

		// addr is too long.
		//
		case ENAMETOOLONG:

		// A component in the directory prefix of the socket pathname does not exist.
		//
		case ENOENT:

		// Insufficient kernel memory was available.
		//
		case ENOMEM:

		// A component of the path prefix is not a directory.
		//
		case ENOTDIR:

		// The socket inode would reside on a read-only filesystem.
		//
		case EROFS:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "bind", 0, "%d - %s on socket: %s", errno, strerror(errno), socketName);
			break;

		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", "bind", 0, "%d - %s on socket: %s", errno, strerror(errno), socketName);
			break;
	}

	return false;
}


bool do_listen(int sockfd, int backlog)
{
	if (listen(sockfd, backlog) == 0) return true;

	switch(errno) {
		// 1. Another socket is already listening on the same port.
		// 2. (Internet domain sockets) The socket referred to by sockfd had not previously been bound to an address and, upon attempting to bind it to an ephemeral port,
		//    it was determined that all port numbers in the ephemeral port range are currently in use.
		//    See the discussion of /proc/sys/net/ipv4/ip_local_port_range in ip(7).
		//
		case EADDRINUSE:

		// The argument sockfd is not a valid file descriptor.
		//
		case EBADF:

		// The file descriptor sockfd does not refer to a socket.
		//
		case ENOTSOCK:

		// The socket is not of a type that supports the listen() operation.
		//
		case EOPNOTSUPP:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "listen", 0, "%d - %s", errno, strerror(errno));
			break;

		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", "listen", 0, "%d - %s", errno, strerror(errno));
			break;
 	}

	return false;
}


bool do_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int *fd)
{
	*fd = accept(sockfd, addr, addrlen);
	if (*fd != -1) return true;

	switch(errno) {
		// The socket is marked nonblocking and no connections are present to be accepted. POSIX.1-2001 and POSIX.1-2008 allow either error to be returned for this case,
		// and do not require these constants to have the same value, so a portable application should check for both possibilities.
		//
#if EAGAIN != EWOULDBLOCK
		case EAGAIN:
#endif
		case EWOULDBLOCK:

		// sockfd is not an open file descriptor.
		//
		case EBADF:

		// A connection has been aborted.
		//
		case ECONNABORTED:

		// The addr argument is not in a writable part of the user address space.
		//
		case EFAULT:

		// The system call was interrupted by a signal that was caught before a valid connection arrived; see signal(7).
		//
		case EINTR:

		// Socket is not listening for connections, or addrlen is invalid (e.g., is negative).
		//
		case EINVAL:

		// The per-process limit on the number of open file descriptors has been reached.
		//
		case EMFILE:

		// The system-wide limit on the total number of open files has been reached.
		//
		case ENFILE:

		// Not enough free memory. This often means that the memory allocation is limited by the socket buffer limits, not by the system memory.
		//
		case ENOBUFS:
		case ENOMEM:

		// The file descriptor sockfd does not refer to a socket.
		//
		case ENOTSOCK:

		// The referenced socket is not of type SOCK_STREAM.
		//
		case EOPNOTSUPP:

		// Protocol error.
		//
		case EPROTO:

		// Firewall rules forbid connection.
		//
		case EPERM:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "accept", 0, "%d - %s", errno, strerror(errno));
			break;

		// In  addition, network errors for the new socket and as defined for the protocol may be returned.
		// Various Linux kernels can return other errors such as ENOSR, ESOCKTNOSUPPORT, EPROTONOSUPPORT, ETIMEDOUT.
		// The value ERESTARTSYS may be seen during a trace.
		//
		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", "accept", 0, "%d - %s", errno, strerror(errno));
			break;
	}

	return false;
}


bool do_read(int fd, void *buf, size_t count, size_t *readBytes)
{
	memset(buf, 0, count);
	*readBytes = read(fd, buf, count);
	if (*readBytes >= 0) return true;

	switch(errno) {
		// 1. The file descriptor fd refers to a file other than a socket and has been marked nonblocking (O_NONBLOCK), and the read would block.
		//    See open(2) for further details on the O_NONBLOCK flag.
		// 2. The file descriptor fd refers to a socket and has been marked nonblocking (O_NONBLOCK), and the read would block.
		//    POSIX.1-2001 allows either error to be returned for this case, and does not require these constants to have the same value,
		//    so a portable application should check for both possibilities.
		//
#if EAGAIN != EWOULDBLOCK
		case EAGAIN:
#endif
		case EWOULDBLOCK:

		// fd is not a valid file descriptor or is not open for reading.
		//
		case EBADF:

		// buf is outside your accessible address space.
		//
		case EFAULT:

 		// The call was interrupted by a signal before any data was read; see signal(7).
		//
		case EINTR:

		// 1. fd is attached to an object which is unsuitable for reading; or the file was opened with the O_DIRECT flag, and either the address specified in buf,
		//    the value specified in count, or the file offset is not suitably aligned.
		// 2. fd was created via a call to timerfd_create(2) and the wrong size buffer was given to read(); see timerfd_create(2) for further information.
		// 
		case EINVAL:

		// I/O error. This will happen for example when the process is in a background process group, tries to read from its controlling terminal,
		// and either it is ignoring or blocking  SIGTTIN or its process group is orphaned. It may also occur when there is a low-level I/O error
		// while reading from a disk or tape. A further possible cause of EIO on networked filesystems is when an advisory lock had been taken out
		// on the file descriptor and this lock has been lost.
		// See the Lost locks section of fcntl(2) for further details.
		//
		case EIO:

		// fd refers to a directory.
		//
		case EISDIR:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "read", 0, "%d - %s", errno, strerror(errno));
			break;

		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", "read", 0, "%d - %s", errno, strerror(errno));
			break;
	}

	return false;
}


bool do_write(int fd, const void *buf, size_t count)
{
	if (write(fd, buf, count) != -1) return true;

	switch(errno) {
		// 1. The file descriptor fd refers to a file other than a socket and has been marked nonblocking (O_NONBLOCK), and the write would block.
		//    See open(2) for further details on  the  O_NONBLOCK flag.
		// 2. The file descriptor fd refers to a socket and has been marked nonblocking (O_NONBLOCK), and the write would block.
		//    POSIX.1-2001 allows either error to be returned for this case, and does not require these constants to have the same value,
		//    so a portable application should check for both possibilities.
		//
#if EAGAIN != EWOULDBLOCK
		case EAGAIN:
#endif
		case EWOULDBLOCK:

		// fd is not a valid file descriptor or is not open for writing.
		//
		case EBADF:

		// fd refers to a datagram socket for which a peer address has not been set using connect(2).
		//
		case EDESTADDRREQ:

		// The user's quota of disk blocks on the filesystem containing the file referred to by fd has been exhausted.
		//
		case EDQUOT:

		// buf is outside your accessible address space.
		//
		case EFAULT:

		// An attempt was made to write a file that exceeds the implementation-defined maximum file size or the process's file size limit,
		// or to write at a position past the maximum allowed offset.
		//
		case EFBIG:

		// The call was interrupted by a signal before any data was written; see signal(7).
		//
		case EINTR:

		// fd is attached to an object which is unsuitable for writing; or the file was opened with the O_DIRECT flag, and either the address specified in buf,
		// the value specified in count, or the file offset is not suitably aligned.
		//
		case EINVAL:

		// A low-level I/O error occurred while modifying the inode. This error may relate to the write-back of data written by an earlier write(2),
		// which may have been issued to a different file  descriptor on the same file. Since Linux 4.13, errors from write-back come with a promise
		// that they may be reported by subsequent. write(2) requests, and will be reported by a subsequent fsync(2) (whether or not they were also
		// reported by write(2)). An alternate cause of EIO on networked filesystems is when an advisory lock had been taken out on the file descriptor
		// and this lock has been lost. See the Lost locks section of fcntl(2) for further details.
		//
		case EIO:

		// The device containing the file referred to by fd has no room for the data.
		//
		case ENOSPC:

		// The operation was prevented by a file seal; see fcntl(2).
		//
		case EPERM:

		// fd is connected to a pipe or socket whose reading end is closed. When this happens the writing process will also receive a SIGPIPE signal.
		// (Thus, the write return value is seen only if the program catches, blocks or ignores this signal.)
		//
		case EPIPE:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "write", 0, "%d - %s", errno, strerror(errno));
			break;

		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", "write", 0, "%d - %s", errno, strerror(errno));
			break;
	}

	return false;
}


bool do_socket(int domain, int type, int protocol, int *sockfd)
{
	*sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*sockfd != -1) return true;

	switch(errno) {
		// Permission to create a socket of the specified type and/or protocol is denied.
		//
		case EACCES:

		// The implementation does not support the specified address family.
		//
		case EAFNOSUPPORT:

		// 1. Unknown protocol, or protocol family not available.
		// 2. Invalid flags in type.
		//
		case EINVAL:

		// The per-process limit on the number of open file descriptors has been reached.
		//
		case EMFILE:

		// The system-wide limit on the total number of open files has been reached.
		//
		case ENFILE:

		// Insufficient memory is available. The socket cannot be created until sufficient resources are freed.
		//
		case ENOBUFS:
		case ENOMEM:

		// The protocol type or the specified protocol is not supported within this domain.
		//
		case EPROTONOSUPPORT:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "socket", 0, "%d - %s", errno, strerror(errno));
			break;

		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", "socket", 0, "%d - %s", errno, strerror(errno));
			break;
	}

	return false;
}


bool do_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if (connect(sockfd, addr, addrlen) == 0) return true;

	switch(errno) {
		// 1. For UNIX domain sockets, which are identified by pathname: Write permission is denied on the socket file, or search permission is denied
		//    for one of the directories in the path  pre‐ fix. (See also path_resolution(7).)
		// 2. The user tried to connect to a broadcast address without having the socket broadcast flag enabled or the connection request failed because of a local firewall rule.
		//
		case EACCES:

		// The user tried to connect to a broadcast address without having the socket broadcast flag enabled or the connection request failed because of a local firewall rule.
		//
		case EPERM:

		// Local address is already in use.
		//
		case EADDRINUSE:

		// (Internet domain sockets) The socket referred to by sockfd had not previously been bound to an address and, upon attempting to bind it to an ephemeral port,
		//  it was determined that all port numbers in the ephemeral port range are currently in use. See the discussion of /proc/sys/net/ipv4/ip_local_port_range in ip(7).
		//
		case EADDRNOTAVAIL:

		// The passed address didn't have the correct address family in its sa_family field.
		//
		case EAFNOSUPPORT:

		// Insufficient entries in the routing cache.
		//
		case EAGAIN:

		// The socket is nonblocking and a previous connection attempt has not yet been completed.
		//
		case EALREADY:

		// sockfd is not a valid open file descriptor.
		//
		case EBADF:

		// A connect() on a stream socket found no one listening on the remote address.
		//
		case ECONNREFUSED:

		// The socket structure address is outside the user's address space.
		//
		case EFAULT:

		// The socket is nonblocking and the connection cannot be completed immediately. It is possible to select(2) or poll(2) for completion by selecting
		// the socket for writing. After select(2) indicates writability, use getsockopt(2) to read the SO_ERROR option at level SOL_SOCKET to determine
		// whether connect() completed successfully (SO_ERROR is zero) or unsuccessfully (SO_ERROR is one of the usual error codes listed here,
		// explaining the reason for the failure).
		//
		case EINPROGRESS:

		// The system call was interrupted by a signal that was caught; see signal(7).
		//
		case EINTR:

		// The socket is already connected.
		//
		case EISCONN:

		// Network is unreachable.
		//
		case ENETUNREACH:

		// The file descriptor sockfd does not refer to a socket.
		//
		case ENOTSOCK:

		// The socket type does not support the requested communications protocol. This error can occur, for example, on an attempt to connect a
		// UNIX domain datagram socket to a stream socket.
		//
		case EPROTOTYPE:

		// Timeout while attempting connection. The server may be too busy to accept new connections. Note that for IP sockets the timeout may be
		// very long when syncookies are enabled on the server.
		//
		case ETIMEDOUT:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Error", "connect", 0, "%d - %s", errno, strerror(errno));
			break;

		default:
			vgc_message(ERROR_LEVEL, __FILE__, __LINE__, moduleName, __func__, "Unknown error", "connect", 0, "%d - %s", errno, strerror(errno));
			break;
	}

	return false;
}
#endif
