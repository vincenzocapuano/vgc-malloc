//
// Copyright (C) 2020 by Vincenzo Capuano
//
#pragma once

#include <stdbool.h>
#include <sys/socket.h>


bool do_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen, const char *socketName);
bool do_listen(int sockfd, int backlog);
bool do_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int *fd);
bool do_read(int fd, void *buf, size_t count, size_t *readBytes);
bool do_write(int fd, const void *buf, size_t count);
bool do_socket(int domain, int type, int protocol, int *sockfd);
bool do_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
