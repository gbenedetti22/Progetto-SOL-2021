#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdbool.h>
#include <sys/socket.h>
#include "../fs_client_api.h"
#include "../my_string.h"
#include "../mysocket.h"
#include "../file_reader.h"

bool running = true;
int fd_sk = 0;
char *current_sock = "";

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

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
    }

    return status;
}

int closeConnection(const char *sockname) {
    if (!str_equals(sockname, current_sock)) {
        fprintf(stderr, "Sock name passed not corrisponding to current sock opend!\n");
        return -1;
    }
    return close(fd_sk);
}

int openFile(const char *pathname, int flags) {
    int response;
    char* client_pid=str_int_toStr(getpid());
    printf("%s\n", client_pid);

    switch (flags) {
        case O_OPEN : {
            char *cmd = str_concatn("o:",pathname,":", client_pid, NULL);

            //invio il comando al server
            sendn(fd_sk, cmd, str_length(cmd));

            //attendo una sua risposta
            response = receiveInteger(fd_sk);

            if (response != 0) {
                fprintf(stderr, "An error occurred");
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
            response = receiveInteger(fd_sk);

            if (response != 0) {
                fprintf(stderr, "An error occurred");
                return -1;
            }
            free(cmd);
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
	char* client_pid=str_int_toStr(getpid());
    //mando la richiesta al server
    char *request = str_concatn("r:", pathname,":",client_pid, NULL);
    sendn(fd_sk, request, str_length(request));

    //attendo una risposta
    int response = receiveInteger(fd_sk);
    if (response >= 0) {    //se il file esiste
        *buf = receiven(fd_sk);
        *size = response;
        free(client_pid);
        free(request);
        return 0;
    }
    //!settare errno
    free(request);
    free(client_pid);
    return -1;
}

int readNFiles(int N, const char *dirname) {
    if (str_endsWith(dirname, "/")) {
        fprintf(stderr, "path not valid, remove the last / char");
        return -1;
    }
    dirname = str_concat(dirname, "/");

    //mando la richiesta al Server
    char *request = str_concat("rn:", str_int_toStr(N));
    sendn(fd_sk, request, str_length(request));

    //se la risposta è ok, il file esiste
    int response = receiveInteger(fd_sk);
    if (response == 0) {
        int n_files = receiveInteger(fd_sk);

        //ricevo i file espulsi
        for (int i = 0; i < n_files; i++) {
            char *file_name = (char *) receiven(fd_sk);     //nome del file
            int n = receiveInteger(fd_sk);                  //quanto è grande
            char *path = str_concat(dirname, file_name);

            void *buff = receiven(fd_sk);                   //contenuto
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
    free(dirname);
    return response;
}

int writeFile(const char *pathname, const char *dirname) {
	char* client_pid=str_int_toStr(getpid());

    //costruisco la key
    char *abs_path = realpath(pathname, NULL);

    FILE *file = fopen(pathname, "rb");
    if (file == NULL) {
        return errno;
    }
    //costruisco il value
    void *file_content = file_readAll(file);

    //mando la richiesta di scrittura al Server -> key: abs_path | value: file_content
    char *request;
    if (dirname != NULL) {
        request = str_concatn("w:", abs_path, ":", client_pid,"?",file_content, ";y", NULL);
    } else {
        request = str_concatn("w:", abs_path, ":", client_pid,"?",file_content, ";n", NULL);
    }

    sendn(fd_sk, request, str_length(request));
    int status = receiveInteger(fd_sk);
    if (dirname != NULL) {
        char *file_exp = receiven(fd_sk);
        str_startTokenizer(file_exp, ";");

        while (str_hasToken()) {
            char **array = NULL;// array[0] -> nome file ; array[1] -> contenuto del file
            str_split(&array, str_getToken(), ":");

            //scrivo il file nella directory
            char *path = str_concat(dirname, array[0]);
            FILE *file = fopen(path, "w");
            fwrite(array[1], sizeof(char), str_length(array[1]), file);

            fclose(file);
            str_clearArray(&array, 2);
        }
    }

    return status;
}

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname) {
    char *request;
    if (dirname != NULL)
        request = str_concatn("a:", pathname, ":g", NULL);
    else
        request = str_concatn("a:", pathname, NULL);

    //invio la richiesta
    sendn(fd_sk, request, str_length(request));
    writen(fd_sk, buf, size);//invio il contenuto da appendere

    if (dirname != NULL) {
        int exp_file = receiveInteger(fd_sk);

        //ricevo i file espulsi
        for (int i = 0; i < exp_file; i++) {
            char *file_name = (char *) receiven(fd_sk);
            int n = receiveInteger(fd_sk);
            char *path = str_concat(dirname, file_name);

            void *buff = receiven(fd_sk);
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

    int status = receiveInteger(fd_sk);
    return status;
}

int closeFile(const char *pathname) {
    char *request = str_concat("c:", pathname);
    sendn(fd_sk, request, str_length(request));
    int status = receiveInteger(fd_sk);
    return status;
}

int removeFile(const char *pathname) {
    char *request = str_concat("r:", pathname);
    sendn(fd_sk, request, str_length(request));
    int status = receiveInteger(fd_sk);
    return status;
}