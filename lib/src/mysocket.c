#include "../mysocket.h"

int unix_socket(char *path) {
    int fd_sk = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_sk == -1) {
        fprintf(stderr, "Socket creation error\n");
        return errno;
    }
    unlink(path);
    return fd_sk;
}

int socket_bind(int fd_sk, char* path){

    struct sockaddr_un sa;
    strcpy(sa.sun_path, path);
    sa.sun_family=AF_UNIX;

    if((bind(fd_sk, (const struct sockaddr *) &sa, sizeof(sa)))==-1){
        fprintf(stderr, "Cannot bind\n");
        if(errno==EADDRINUSE){
            fprintf(stderr, "Address already in use\n");
        }
        return errno;
    }
    listen(fd_sk,SOMAXCONN);
    return 0;
}

int socket_accept(int fd_sk){
    int fd_c= accept(fd_sk,NULL, 0);
    return fd_c;
}

int socket_connect(int fd_sk, struct sockaddr sa){
    if((connect(fd_sk, (const struct sockaddr *) &sa, sizeof(sa))) == -1){
        fprintf(stderr, "Socket non trovato\n");
        return -1;
    }

    return 0;
}

void socket_close(int fd_sk){
    if(close(fd_sk)==-1){
        fprintf(stderr, "Errore nella chiusura del Socket");
        return;
    }
}

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
int readn(int fd, void *buf, size_t size) {
    size_t left = size;
    int r=0;
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
int writen(int fd, void *buf, size_t size) {
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
    if (writen(fd_sk, &n, sizeof(int)) == -1) {
        fprintf(stderr, "An error occurred reading msg lenght\n");
        exit(errno);
    }
}

void sendn(int fd_sk, void* msg, int lenght){
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