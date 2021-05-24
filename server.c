#include <stdio.h>
#include "lib/mysocket.h"
#include "lib/hash_table.h"
#include "lib/config_parser.h"
#include "lib/my_string.h"
#include "lib/queue.h"
#include "lib/sorted_list.h"
#include "lib/myerrno.h"
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#define PRINT_TABLE(x,flags) hash_iterate(x, &print,(void*)flags)

volatile sig_atomic_t server_running = true;

hash_table *storage;            //contiene tutti i file associati al loro path assoluto
hash_table* opened_files;       //memorizza quanti file un determinato client ha aperto
settings config = DEFAULT_SETTINGS;
queue *q;
int pipe_fd[2];

typedef struct{
    char* path;
    void* content;
    size_t size;
    list* pidlist;
}file_s;

static file_s* file_init(char* path, void* content, size_t size){
    if(path==NULL || str_isEmpty(path))
        return NULL;

    file_s* file= malloc(sizeof(file_s));
    file->path= str_create(path);
    file->content=content;
    file->size=size;
    file->pidlist=list_create();

    return file;
}

void file_update(file_s** f, char* newContent, size_t newSize){
    (*f)->content=newContent;
    (*f)->size=newSize;
}

bool file_removeClient(file_s** f, char* cpid){
    return list_remove(&(*f)->pidlist,cpid);
}

bool file_isEmpty(file_s* f){
    return f->content==NULL;
}

bool file_isOpened(file_s* f){
    return list_isEmpty(f->pidlist);
}

bool file_isOpenedBy(file_s* f, char* pid){
    return list_conteinsKey(f->pidlist,pid);
}

int file_appendClient(file_s** f, char* clientPid){
    if(file_isOpenedBy(*f, clientPid)){
        return -1;
    }

    list_insert(&(*f)->pidlist,clientPid,NULL);
    return 0;
}

void print(char* key, void* value, bool* exit, void* argv){
    int n=(int)argv;
    if(n==0){
        printf("key: %s | value: %s\n", key, ((file_s *) value)->path);
    } else if(n==1){
        printf("key: %s | value: NULL\n", key);
    }
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void close_server();

void increase(char* cpid){
    (*(int*)hash_getValue(opened_files, cpid))++;
}

void decrease(char* cpid){
    (*(int*)hash_getValue(opened_files, cpid))--;
}

void init_server() {
    settings_load(&config, "../../config.ini");
    storage = hash_create(config.MAX_STORABLE_FILES);
    opened_files= hash_create(config.MAX_STORABLE_FILES);
    q = queue_create();
    pipe(pipe_fd);
}

void openFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filepath = array[0];
    char *cpid = array[1];

    if (!hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_NOT_FOUND);
        str_clearArray(&array, n);
        return;
    }

    file_s *f = hash_getValue(storage, filepath);
    if (file_isOpenedBy(f, cpid)) {
        sendInteger(client, SFILE_ALREADY_OPENED);
        str_clearArray(&array, n);
        return;
    }

    file_appendClient(&f,cpid);

    hash_updateValue(&storage, filepath, f);
    increase(cpid);
    sendInteger(client, S_SUCCESS);

    str_clearArray(&array, n);
}

void createFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filepath = array[0];
    char* cpid= array[1];
    assert(!str_isEmpty(filepath) && filepath != NULL);

    if (hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_ALREADY_EXIST);
    } else {
        file_s* f= file_init(filepath, NULL, 0);
        file_appendClient(&f,cpid); //apro il file in lettura e scrittura

        hash_insert(&storage, filepath, f);//memorizzo il file
        increase(cpid);
        sendInteger(client, S_SUCCESS);
    }
    str_clearArray(&array, n);
}

void searchFile(){
    //coming soon...
}

void readFile(int client, char *filepath) {
    assert(!str_isEmpty(filepath) && filepath != NULL);

    //controllo che il file sia stato creato
    if(!hash_containsKey(storage,filepath)){
        sendInteger(client, SFILE_NOT_FOUND);
        return;
    }
    sendInteger(client, S_SUCCESS);
    file_s *file = hash_getValue(storage, filepath);
//    if(!file_isOpenedBy(file, cpid)){
//        sendInteger(client, SFILE_NOT_OPENED);
//        str_clearArray(&array, n);
//        return;
//    }

    sendn(client, file->content, file->size);
}

void send_nfiles(char* key, void* value, bool* exit, void* args){
    int client=(int) args;
    file_s* f=(file_s*) value;
    sendStr(client,key);
    sendn(client, f->content, f->size);
}

void readNFile(int client, char *request) {
    if(hash_isEmpty(storage)){
        sendInteger(client, S_STORAGE_EMPTY);
        return;
    }

    int n;
    int res=str_toInteger(&n,request);
    assert(res != -1);

    sendInteger(client, S_SUCCESS);
    hash_iteraten(storage,&send_nfiles,(void*)client,n);
}

void appendFile(int client, char* request){
    char** array=NULL;
    str_split(&array,request,":");
    char* filepath=array[0];
    void* buff;
    size_t size;
    receivefile(client,&buff,&size);
    if(!hash_getValue(storage,filepath)){
        free(buff);
        sendInteger(client, SFILE_NOT_FOUND);
        return;
    }

    file_s* f= hash_getValue(storage,filepath);

    size_t newSize=f->size + size;
    void* newContent= malloc(newSize);

    memcpy(newContent, f->content, f->size);
    memcpy(newContent + f->size, buff, size);

    free(f->content);
    f->content=newContent;
    f->size=newSize;
    sendInteger(client,S_SUCCESS);
}

void writeFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":?");
    assert(n == 3);
    char *filepath = array[0];
    char* cpid=array[1];

    //!DA USARE POI
    char option = (array[2])[0];
    size_t fsize;

//    printf("%c\n", option);
    void* fcontent=NULL;
    receivefile(client,&fcontent,&fsize);

    if (!hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_NOT_FOUND);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    file_s* f = hash_getValue(storage,filepath);
    if(!file_isOpenedBy(f, cpid)){
        sendInteger(client, SFILE_NOT_OPENED);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    } else if(!file_isEmpty(f)){
        sendInteger(client, SFILE_NOT_EMPTY);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }
    assert(f->path != NULL && !str_isEmpty(f->path));
    file_update(&f,fcontent,fsize);

    sendInteger(client, S_SUCCESS);
    str_clearArray(&array, n);
}

void clear_openedFiles(char* key, void* value, bool* exit, void* cpid){
    file_s* f=(file_s*)value;
    if(file_isOpenedBy(f, (char *) cpid)){
        file_removeClient(&f,(char*)cpid);
    }
}
void closeConnection(int client, char *cpid) {
    int nfiles= *((int*)hash_getValue(opened_files,cpid));

    if(nfiles==0){
        sendInteger(client,S_SUCCESS);
        hash_deleteKey(&opened_files,cpid);
    } else{
        sendInteger(client, SFILES_FOUND_ON_EXIT);
        hash_iterate(storage,&clear_openedFiles,(void*)cpid);
    }

}

void closeFile(int client, char *request) {
    char** array=NULL;
    int n=str_split(&array,request,":");
    assert(n==2);

    char* filepath=array[0];
    char* cpid=array[1];
    
    if(!hash_containsKey(storage,filepath)){
        sendInteger(client, SFILE_NOT_FOUND);
        str_clearArray(&array, n);
        return;
    }
    file_s* f= hash_getValue(storage,filepath);
    
    if(!file_isOpenedBy(f, cpid)){
        sendInteger(client, SFILE_NOT_OPENED);
        str_clearArray(&array, n);
        return;
    }

    file_removeClient(&f,cpid);
    decrease(cpid);
    sendInteger(client,S_SUCCESS);
    str_clearArray(&array,n);
}

void removeFile(int client, char *request) {
    //il file non esiste
    if(!hash_containsKey(storage,request)){
        sendInteger(client, SFILE_NOT_FOUND);
        return;
    }

    char** array=NULL;
    int n=str_split(&array,request,":");
    char* filepath=array[0];
    char* cpid=array[1];

    file_s* f= hash_getValue(storage,filepath);

    //chiudo un file che ho aperto
    if(file_isOpenedBy(f, cpid)){
        hash_deleteKey(&storage,filepath);
        decrease(cpid);
        sendInteger(client, S_SUCCESS);
    }else{
        sendInteger(client,SFILE_NOT_OPENED);
    }

    str_clearArray(&array,n);
}

void *worker_function(void *argv) {
    while (true) {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond, &lock);
        if (!server_running) {
            break;
        }

        int client = queue_get(&q);

        char *request = receiveStr(client);

        assert(request!=NULL && !str_isEmpty(request));
        switch (request[0]) {
            case 'a': {
                char *cmd = str_cut(request, 2, str_length(request) - 2);
                appendFile(client, cmd);
                free(cmd);
                break;
            }
            case 'o': {
                char *cmd = str_cut(request, 2, str_length(request) - 2);
                openFile(client, cmd);
                free(cmd);
                break;
            }
            case 'c': {
                char *cmd;
                if(request[1]=='l'){
                    cmd = str_cut(request, 3, str_length(request) - 3);
                    closeFile(client, cmd);
                }else {
                    cmd = str_cut(request, 2, str_length(request) - 2);
                    createFile(client, cmd);
                    free(cmd);
                }
                break;
            }

            case 'r': {
                char *cmd;
                if (request[1] == 'n') {
                    cmd = str_cut(request, 3, str_length(request) - 3);
                    readNFile(client, cmd);
                } else if(request[1] == 'm'){
                    cmd = str_cut(request, 3, str_length(request) - 3);
                    removeFile(client,cmd);
                }else {
                    cmd = str_cut(request, 2, str_length(request) - 2);
                    readFile(client, cmd);
                }
                free(cmd);

                break;
            }

            case 'w': {
                char *cmd = str_cut(request, 2, str_length(request) - 2);
                writeFile(client, cmd);
                free(cmd);
                break;
            }

            case 'e':{
                char *cmd = str_cut(request, 2, str_length(request) - 2);
                closeConnection(client,cmd);
                close(client);
                break;
            }

            default: {
                assert(1 == 0);
            }
        }

        if(request[0] != 'e') {
            if (writen(pipe_fd[1], &client, sizeof(int)) == -1) {
                fprintf(stderr, "An error occurred on write back client to the pipe\n");
                exit(errno);
            }
        }
        free(request);
        pthread_mutex_unlock(&lock);
    }
    return argv;
}

//int update_fdmax(fd_set set, int fdmax) {
//    for(int i=(fdmax-1);i>=0;--i)
//        if (FD_ISSET(i, &set)) return i;
//    assert(1==0);
//}


int main(void) {
    init_server();
    sorted_list *fd_list = sortedlist_create();

    int fd_sk = unix_socket(config.SOCK_PATH);
    if (socket_bind(fd_sk, config.SOCK_PATH) != 0) {
        return errno;
    }
    pthread_t tid;
    pthread_t thread_pool[config.N_THREAD_WORKERS];
    for (int i = 0; i < config.N_THREAD_WORKERS; i++) {
        pthread_create(&tid, NULL, &worker_function, NULL);
        thread_pool[i] = tid;
    }
    printf("[Server in Ascolto]\n");

    fd_set current_sockets;

    FD_ZERO(&current_sockets);
    FD_SET(fd_sk, &current_sockets);
    FD_SET(pipe_fd[0], &current_sockets);
    sortedlist_insert(&fd_list, fd_sk);
    sortedlist_insert(&fd_list, pipe_fd[0]);

//    int max_socket;
//    if (fd_sk > pipe_fd[0]) {
//        max_socket =fd_sk;
//    } else {
//        max_socket = pipe_fd[0];
//    }

    while (server_running) {
        fd_set ready_sockets = current_sockets;
        assert((sortedlist_getMax(fd_list) + 1) < FD_SETSIZE);

        if (select(sortedlist_getMax(fd_list) + 1, &ready_sockets, NULL, NULL, NULL) < 0) {
            if (errno != EINTR)
                fprintf(stderr, "Error on select\n");
            break;
        }
        sortedlist_iterate();
        for (int i = 0; i <= sortedlist_getMax(fd_list); i++) {
            int setted_fd = sortedlist_getNext(fd_list);
            if (FD_ISSET(setted_fd, &ready_sockets)) {
                if (setted_fd == fd_sk) {
                    int client = socket_accept(fd_sk);
                    if(client != -1) {
                        char *cpid = receiveStr(client);

                        int *n = malloc(sizeof(int));
                        if(n==NULL){
                            fprintf(stderr, "Impossibile allocare per nuovo client\n");
                            return errno;
                        }
                        *n = 0;
                        hash_insert(&opened_files, cpid, n);
                    }
                    FD_SET(client, &current_sockets);
                    sortedlist_insert(&fd_list, client);
//                    if (client->fd_sk > max_socket)
//                        max_socket = client->fd_sk;
                    break;

                } else if (setted_fd == pipe_fd[0]) {
                    int old_fd_c;
                    readn(pipe_fd[0], &old_fd_c, sizeof(int));
                    FD_SET(old_fd_c, &current_sockets);
                    sortedlist_insert(&fd_list, old_fd_c);

//                    if (old_fd_c > max_socket)
//                        max_socket = old_fd_c;
                    break;
                } else {
                    FD_CLR(setted_fd, &current_sockets);
                    sortedlist_remove(&fd_list, setted_fd);
                    queue_insert(&q, setted_fd);
//                    if (i == max_socket)
//                        max_socket = update_fdmax(current_sockets, max_socket);

                    pthread_cond_signal(&cond);
                    break;
                }
            }
        }
    }

    return 0;
}


//int main(int argc, char *argv[]) {
//    hash_table* t= hash_create(10);
//    int* n= malloc(sizeof(int));
//    *n=0;
//    hash_insert(&t, "ciao", n);
//    int val= 1 + *((int*)hash_getValue(t,"ciao"));
//    printf("%d\n", val);
//    hash_destroy(&t);
//    return 0;
//}
void close_server() {
    settings_free(&config);
    hash_destroy(&storage);
    hash_destroy(&opened_files);
}
