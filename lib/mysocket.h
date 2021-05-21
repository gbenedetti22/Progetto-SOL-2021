#if !defined(MY_SOCKET_H)
#define MY_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int unix_socket(char *path);
int socket_bind(int fd_sk, char* path);
int socket_accept(int fd_sk);
int socket_connect(int fd_sk, struct sockaddr sa);
void socket_close(int fd_sk);
int readn(int fd, void *buf, size_t size);
int writen(int fd, void *buf, size_t size);
void sendInteger(int fd_sk, int n);
int receiveInteger(int fd_sk);
void sendn(int fd_sk, void* msg, int lenght);
void* receiven(int fd_sk);
#endif /* MY_SOCKET_H */
