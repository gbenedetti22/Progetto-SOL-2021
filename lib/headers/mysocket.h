#if !defined(CONN_H)
#define CONN_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    void* bufptr = buf;
    while(left>0) {
        if ((r=(int) read((int)fd ,bufptr,left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;   // EOF
        left    -= r;
        bufptr  += r;
    }
    return size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    void* bufptr = buf;
    while(left>0) {
        if ((r=(int) write((int)fd ,bufptr,left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;
        left    -= r;
        bufptr  += r;
    }
    return 1;
}

void sendInteger(int fd_sk, int n) {
    if(writen(fd_sk, &n, sizeof(int)) == -1){
        fprintf(stderr, "An error occurred reading msg lenght\n");
        exit(errno);
    }
}

//void sendn(int fd_sk, char* msg){
//    int lenght=(int) strlen(msg);
//
//    sendInteger(fd_sk, lenght);
//
//    if(writen(fd_sk, (char*)msg, lenght) == -1){
//        fprintf(stderr, "An error occurred on reading msg\n");
//        exit(errno);
//    }
//
//}

void sendn(int fd_sk, void* msg, int lenght){
//    int lenght=(int) strlen(msg);

    sendInteger(fd_sk, lenght);

    if(writen(fd_sk, msg, lenght) == -1){
        fprintf(stderr, "An error occurred on reading msg\n");
        exit(errno);
    }

}

int receiveInteger(int fd_sk) {
    int n=0;
    if(readn(fd_sk, &n, sizeof(int)) == -1){
        fprintf(stderr, "An error occurred reading msg lenght\n");
        exit(errno);
    }
    return n;
}

/* Riceve un messaggio dal socket fd_sk.
 * In caso di successo, la funzione receiven() ritorna
 * la stringa letta o NULL in caso di errore.
 * La stringa è già pre-allocata*/
void* receiven(int fd_sk){
    int lenght = receiveInteger(fd_sk);

    void* buff= malloc(lenght * sizeof(char));
    if(buff == NULL){
        return buff;
    }

    if(readn(fd_sk, buff, lenght * sizeof(char)) == -1){
        fprintf(stderr, "An error occurred on reading msg\n");
        exit(errno);
    }

    return buff;
}


#endif /* CONN_H */
