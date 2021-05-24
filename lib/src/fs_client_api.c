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
    printf("exit...\n");
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
    fd_sk = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un sa;
    strcpy(sa.sun_path, sockname);
    sa.sun_family = AF_UNIX;

    pthread_t tid;
    pthread_create(&tid, NULL, &thread_function, (void *) &abstime);
    int status;

    while (running) {
        status = connect(fd_sk, (const struct sockaddr *) &sa, sizeof(sa));
        if(status==0)
            break;
        usleep(msec * 1000);
    }

    if (status == 0) {
        current_sock = sockname;
        char* mypid= str_long_toStr(getpid());
        sendn(fd_sk,mypid, str_length(mypid));
        atexit(exit_function);
    }

    return status;
}

int closeConnection(const char *sockname) {
    if(fcntl(fd_sk, F_GETFL) < 0){
        return SOCKET_ALREADY_CLOSED;
    }

    if (!str_equals(sockname, current_sock)) {
        fprintf(stderr, "Sock name passed not corrisponding to current sock opend!\n");
        return -1;
    }
    char* client_pid=str_long_toStr(getpid());
    char* request= str_concat("e:",client_pid);

    sendn(fd_sk,request, str_length(request));
    int status= (int)receiveInteger(fd_sk);
    if(status != S_SUCCESS){
        fprintf(stderr, "Non hai chiuso alcuni file, ricordatelo la prossima volta!\n");
    }
    return close(fd_sk);
}

int openFile(const char *pathname, int flags) {
    int response;
    char* client_pid=str_long_toStr(getpid());
    printf("%s\n", client_pid);

    switch (flags) {
        case O_OPEN : {
            char *cmd = str_concatn("o:",pathname,":", client_pid, NULL);

            //invio il comando al server
            sendn(fd_sk, cmd, str_length(cmd));

            //attendo una sua risposta
            response = (int)receiveInteger(fd_sk);

            if (response != 0) {
                return -1;
            }
            free(cmd);
            break;
        }
        case O_CREATE : {   //comando: c:filename
            char *abs_path = realpath(pathname, NULL);
            if(abs_path==NULL){
                return -1;
            }

            char *cmd = str_concatn("c:", abs_path, ":", client_pid, NULL);
            //invio il comando al server
            sendn(fd_sk, (char*)cmd, str_length(cmd));

            //attendo una sua risposta
            response = (int)receiveInteger(fd_sk);

            if (response != 0) {
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
    return -1;
}

int readNFiles(int N, const char *dirname) {
    if (str_endsWith(dirname, "/")) {
        fprintf(stderr, "path not valid, remove the last / char");
        return -1;
    }

    //mando la richiesta al Server
    char *request = str_concat("rn:", str_long_toStr(N));
    sendStr(fd_sk, request);

    //se la risposta è ok, lo storage non è vuoto
    int response = (int)receiveInteger(fd_sk);
    if (dirname != NULL && response==0) {
        size_t size;
        void* buff;

        //ricevo i file espulsi
        for (int i = 0; i < N; i++) {
            char *file_name = receiveStr(fd_sk);     //nome del file
            receivefile(fd_sk,&buff,&size);

            char *path = str_concatn(dirname,"/", file_name,NULL);
            FILE *file = fopen(path, "wb");
            if (file == NULL) {
                fprintf(stderr, "Errore nel salvataggio "
                                "dei file espulsi dal Server nella cartella %s\n", dirname);
                response=-1;
                break;

            }
            fwrite(buff, sizeof(char), size, file);
            fclose(file);
            free(path);
        }
    }
    free(request);
    return response;
}

int writeFile(const char *pathname, const char *dirname) {
	char* client_pid=str_long_toStr(getpid());

    //costruisco la key
    char *abs_path = realpath(pathname, NULL);
    if(abs_path==NULL)
        return -1;

    FILE *file = fopen(pathname, "rb");
    if (file == NULL) {
        return errno;
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

    return status;
}

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname) {
    char *request;
    if (dirname != NULL)
        request = str_concatn("a:", pathname, ":y", NULL);
    else
        request = str_concatn("a:", pathname, ":n" ,NULL);

    //invio la richiesta
    sendStr(fd_sk, request);
    sendn(fd_sk, buf, size);//invio il contenuto da appendere

    if (dirname != NULL) {
        int exp_file = (int)receiveInteger(fd_sk);

        //ricevo i file espulsi
        for (int i = 0; i < exp_file; i++) {
            size_t lenght;
            char *file_name = (char *) receiven(fd_sk,&lenght);
            file_name[lenght]=0;
            int n = (int)receiveInteger(fd_sk);
            char *path = str_concat(dirname, file_name);

            void *buff = receiven(fd_sk,NULL);
            FILE *file = fopen(path, "wb");
            if (file == NULL) {
                fprintf(stderr, "Errore nel salvataggio "
                                "dei file espulsi dal Server nella cartella %s\n", dirname);
                break;

            }
            fwrite(buff, sizeof(char), n, file);
            fclose(file);
        }
    }

    int status = (int)receiveInteger(fd_sk);
    return status;
}

int closeFile(const char *pathname) {
    char* client_pid=str_long_toStr(getpid());
    char *request = str_concatn("cl:", pathname,":",client_pid,NULL);
    sendn(fd_sk, request, str_length(request));

    int status = (int)receiveInteger(fd_sk);
    return status;
}

int removeFile(const char *pathname) {
    char* client_pid=str_long_toStr(getpid());
    char *request = str_concatn("rm:", pathname,":", client_pid, NULL);
    sendn(fd_sk, request, str_length(request));
    int status = (int)receiveInteger(fd_sk);
    return status;
}