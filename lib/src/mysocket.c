#include "../mysocket.h"
#include "../file_reader.h"

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
size_t readn(int fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    void* bufptr = buf;
    while(left>0) {
        if ((r=(int) read(fd ,bufptr,left)) == -1) {
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
        if ((r=(int) write(fd ,bufptr,left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;
        left    -= r;
        bufptr  += r;
    }
    return 1;
}

int sendInteger(int fd_sk, size_t n) {
    if (writen(fd_sk, &n, sizeof(unsigned long)) == -1) {
        fprintf(stderr, "An error occurred on sending msg lenght\n");
        return errno;
    }

    return 0;
}

size_t receiveInteger(int fd_sk) {
    size_t n=0;
    if(readn(fd_sk, &n, sizeof(unsigned long)) == -1){
        fprintf(stderr, "An error occurred reading msg lenght\n");
        return errno;
    }

    return n;
}

int sendn(int fd_sk, void* msg, size_t lenght){
    if(sendInteger(fd_sk,lenght)!=0){
        fprintf(stderr, "Errore nell invio della dimensione del file\n");
        return errno;
    }

    if(writen(fd_sk, msg, lenght) == -1){
        fprintf(stderr, "An error occurred on sending msg\n");
        return errno;
    }
    return 0;
}


/* Riceve un messaggio dal socket fd_sk.
 * In caso di successo, la funzione receiven() ritorna
 * il buffer letto o NULL in caso di errore (errno viene settato a dovere).
 * Il buffer è già pre-allocato*/
void* receiven(int fd_sk, size_t* size){
    size_t lenght = receiveInteger(fd_sk) * sizeof(char);
    if(size!=NULL)
        *size=lenght;

    void* buff= malloc(lenght);
    if(buff == NULL){
        return NULL;
    }
    if(readn(fd_sk, buff, lenght) == -1){
        fprintf(stderr, "An error occurred on reading msg\n");
        return NULL;
    }

    return buff;
}

int sendfile(int fd_sk, char* pathname){
    FILE* file= fopen(pathname,"rb");
    if(file==NULL){
        return -1;
    }

    size_t fsize= file_getsize(file);

    if(sendInteger(fd_sk,fsize)!=0){
        fprintf(stderr, "Errore nell invio della dimensione del file\n");
        return errno;
    }
    void* fcontent= file_readAll(file);

    if(writen(fd_sk, fcontent, fsize) == -1){
        fprintf(stderr, "An error occurred on sending file\n");
        exit(errno);
    }

    return 0;
}

void receivefile(int fd_sk, void** buff, size_t* lenght){
    size_t size= receiveInteger(fd_sk);
    *lenght=size;

    *buff = malloc(size* sizeof(char));
    if(*buff==NULL){
        fprintf(stderr, "Impossibile ricevere un file: memoria finita\n");
        return;
    }
    if(readn(fd_sk, *buff, size) == -1){
        fprintf(stderr, "An error occurred reading file\n");
        exit(errno);
    }
}

void sendStr(int to, char* msg){
    sendn(to,msg, (int)strlen(msg));
}

char* receiveStr(int from){
    size_t lenght;
    char *msg = (char *) receiven(from,&lenght);
    msg[lenght]=0;

    return msg;
}