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

size_t readn(int fd, void *buf, size_t size);
int writen(int fd, void *buf, size_t size);
int sendInteger(int fd_sk, unsigned long n);
size_t receiveInteger(int fd_sk);
int sendn(int fd_sk, void* msg, size_t lenght);

int sendfile(int fd_sk, const char* pathname);

/* Ricevi un file inviato precedentemente con la funzione sendFile()
 * o con la sendn(). Se viene usata quest'ultima, i parametri devono
 * essere rispettivamente:
 * sendn("fd socket", "contenuto del file", "grandezza del file").
 *
 * In caso di successo, buff viene allocato
 * e size contiene la grandezza del file.
 * */
void receivefile(int fd_sk, void** buff, size_t* lenght);
void sendStr(int to, char* msg);
char* receiveStr(int from);
#endif /* MY_SOCKET_H */
