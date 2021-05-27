#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "../fs_client_api.h"
#include "../my_string.h"
#include "../mysocket.h"
#include "../myerrno.h"

bool running = true;
int fd_sk = 0;
char *current_sock = "";

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void exit_function(){
    closeConnection(current_sock);
}

static void *thread_function(void *abs_t) {
    struct timespec *abs = (struct timespec *) abs_t;

    pthread_mutex_lock(&lock);
    pthread_cond_timedwait(&cond, &lock, abs);

    running = false;
    pthread_mutex_unlock(&lock);
    return NULL;
}

int openConnection(const char *sockname, int msec, const struct timespec abstime) {
    errno=0;
    fd_sk = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un sa;
    strcpy(sa.sun_path, sockname);
    sa.sun_family = AF_UNIX;
    int status = connect(fd_sk, (const struct sockaddr *) &sa, sizeof(sa));

    if(status==0){
        current_sock = sockname;
        char* mypid= str_long_toStr(getpid());
        sendn(fd_sk,mypid, str_length(mypid));
        atexit(exit_function);
        return 0;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, &thread_function, (void *) &abstime);

    while (running) {
        status = connect(fd_sk, (const struct sockaddr *) &sa, sizeof(sa));
        if(status==0){
            current_sock = sockname;
            char* mypid= str_long_toStr(getpid());
            sendn(fd_sk,mypid, str_length(mypid));
            atexit(exit_function);
            return 0;
        }
        usleep(msec * 1000);
    }

    errno=CONNECTION_TIMED_OUT;

    pthread_join(tid,NULL);
    return -1;
}

int closeConnection(const char *sockname) {
    errno=0;

    if(sockname==NULL){
        return 0;
    }

    if(fcntl(fd_sk, F_GETFL) < 0){
        errno=SOCKET_ALREADY_CLOSED;
        return -1;
    }

    if (!str_equals(sockname, current_sock)) {
        errno=WRONG_SOCKET;
        return -1;
    }
    char* client_pid=str_long_toStr(getpid());
    char* request= str_concat("e:",client_pid);

    sendn(fd_sk,request, str_length(request));
    int status= (int)receiveInteger(fd_sk);

    if(status != S_SUCCESS){
        errno=status;
        return -1;
    }

    return close(fd_sk);
}

int openFile(const char *pathname, int flags) {
    errno=0;

    if(pathname==NULL){
        return 0;
    }

    int response;
    char* client_pid=str_long_toStr(getpid());

    switch (flags) {
        case O_OPEN : {
            char *cmd = str_concatn("o:",pathname,":", client_pid, NULL);

            //invio il comando al server
            sendn(fd_sk, cmd, str_length(cmd));

            //attendo una sua risposta
            response = (int)receiveInteger(fd_sk);

            if (response != 0) {
                errno=response;
                return -1;
            }
            free(cmd);
            break;
        }
        case O_CREATE : {   //comando: c:filename
            char *abs_path = realpath(pathname, NULL);
            if(abs_path==NULL){
                errno=FILE_NOT_FOUND;
                return -1;
            }

            char *cmd = str_concatn("c:", abs_path, ":", client_pid, NULL);
            //invio il comando al server
            sendn(fd_sk, (char*)cmd, str_length(cmd));

            //attendo una sua risposta
            response = (int)receiveInteger(fd_sk);

            if (response != 0) {
                errno=response;
                return -1;
            }
            free(cmd);
            free(abs_path);
            break;
        }

        case O_LOCK : {
            printf("not implemented yet...\n");
            response = -1;
            break;
        }

        default: {
            fprintf(stderr, "flags argument error, use O_CREATE or O_LOCK\n");
            response = -1;
            break;
        }
    }
    free(client_pid);
    return response;
}

int readFile(const char *pathname, void **buf, size_t *size) {
    errno=0;

    if(pathname==NULL){
        return 0;
    }

    //mando la richiesta al server
    char *request = str_concat("r:", pathname);
    sendn(fd_sk, request, str_length(request));

    //attendo una risposta
    int response = (int)receiveInteger(fd_sk);
    if (response == 0) {    //se il file esiste
        receivefile(fd_sk,buf,size);
        free(request);
        return 0;
    }
    //!settare errno
    free(request);
    errno=response;
    return -1;
}

int readNFiles(int N, const char *dirname) {
    errno=0;

    if (!str_endsWith(dirname, "/") && dirname != NULL) {
        dirname = str_concat(dirname,"/");
    }

    //mando la richiesta al Server
    char *request = str_concat("rn:", str_long_toStr(N));
    sendStr(fd_sk, request);

    //se la risposta è ok, lo storage non è vuoto
    int response = (int)receiveInteger(fd_sk);
    if(response != 0) {
        errno = response;
        return -1;
    }

    size_t size;
    void* buff;
    if (dirname != NULL) {
        //ricevo i file espulsi
        while((int) receiveInteger(fd_sk)!=EOS_F) {
            char *file_name = receiveStr(fd_sk);     //nome del file
            receivefile(fd_sk,&buff,&size);

            char *path = str_concat(dirname, file_name);
            FILE *file = fopen(path, "wb");
            if (file == NULL) { //se dirname è invalido, viene visto subito
                errno=INVALID_ARG;
                return -1;
            }
            fwrite(buff, sizeof(char), size, file);
            fclose(file);
            free(path);
        }
    } else{
        while((int) receiveInteger(fd_sk) != EOS_F) {
            char* filepath=receiveStr(fd_sk);
            free(filepath);
            receivefile(fd_sk,&buff,&size);
            free(buff);
        }
    }
    free(request);
    return 0;
}

int writeFile(const char *pathname, const char *dirname) {
    errno=0;

    if(pathname==NULL && dirname != NULL){
        errno=INVALID_ARG;
        return -1;
    }

    if(pathname==NULL){
        return 0;
    }

	char* client_pid=str_long_toStr(getpid());

    //costruisco la key
    char *abs_path = realpath(pathname, NULL);
    if(abs_path==NULL) {
        errno=FILE_NOT_FOUND;
        return -1;
    }

    //mando la richiesta di scrittura al Server -> key: abs_path | value: file_content
    char *request;
    if (dirname != NULL) {
        request = str_concatn("w:", abs_path, ":", client_pid,"?", "y", NULL);
    } else {
        request = str_concatn("w:", abs_path, ":", client_pid,"?", "n", NULL);
    }

    sendn(fd_sk, request, str_length(request));
    sendfile(fd_sk,pathname);

    int status = (int)receiveInteger(fd_sk);

    if(status==S_SUCCESS){
        return 0;
    }

    if(status==S_STORAGE_FULL && dirname != NULL){
        while(((int)receiveInteger(fd_sk))!=EOS_F){
            char* filepath=receiveStr(fd_sk);
            FILE* file=fopen(filepath,"wb");
            void* buff;
            size_t n;
            receivefile(fd_sk,&buff,&n);
            fwrite(buff,sizeof(char), n, file);
            fclose(file);
        }

        return 0;
    }

    if(status != S_STORAGE_FULL){
        errno=status;
        return -1;
    }

    return 0;
}

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname) {
    errno=0;

    char* client_pid=str_long_toStr(getpid());
    char *request;
    if (dirname != NULL)
        request = str_concatn("a:", pathname,":", client_pid, ":y", NULL);
    else
        request = str_concatn("a:", pathname,":", client_pid, ":n" ,NULL);

    //invio la richiesta
    sendStr(fd_sk, request);
    sendn(fd_sk, buf, size);//invio il contenuto da appendere
    int status = (int)receiveInteger(fd_sk);

    if(status != 0 && status != S_STORAGE_FULL){
        errno=status;
        return -1;
    }

    if (dirname != NULL && status==S_STORAGE_FULL) {
        //ricevo i file espulsi
        while(((int)receiveInteger(fd_sk))!=EOS_F) {
            char *filepath = receiveStr(fd_sk);

            char* filename=strrchr(filepath,'/')+1;
            char *path = str_concat(dirname, filename);

            size_t n;
            void* buff;
            receivefile(fd_sk,&buff,&n);
            FILE *file = fopen(path, "wb");
            if (file == NULL) {
                errno=INVALID_ARG;
                return -1;
            }
            fwrite(buff, sizeof(char), n, file);
            fclose(file);
        }
    }

    status = (int)receiveInteger(fd_sk);
    if(status != 0){
        errno=status;
        return -1;
    }

    return 0;
}

int closeFile(const char *pathname) {
    errno=0;

    if(pathname==NULL){
        return 0;
    }

    char* client_pid=str_long_toStr(getpid());
    char *request = str_concatn("cl:", pathname,":",client_pid,NULL);
    sendn(fd_sk, request, str_length(request));

    int status = (int)receiveInteger(fd_sk);
    if(status != 0){
        errno=status;
        return -1;
    }
    return 0;
}

int removeFile(const char *pathname) {
    errno=0;

    if(pathname==NULL)
        return 0;

    char* client_pid=str_long_toStr(getpid());
    char *request = str_concatn("rm:", pathname,":", client_pid, NULL);
    sendn(fd_sk, request, str_length(request));
    int status = (int)receiveInteger(fd_sk);

    if(status != 0){
        errno=status;
        return -1;
    }
    return 0;
}